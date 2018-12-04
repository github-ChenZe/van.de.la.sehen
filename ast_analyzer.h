#ifndef AST_ANALYZER_H
#define AST_ANALYZER_H

#include "attribute_handler.h"
#include "print_injector.h"
#include "analyzer_context.h"

/**
 *  function that performs proper DFS on AST.
 */

typedef void (*analyzer)(tree, analyzer_context*);

static analyzer analyzer_from_tree_code(tree_code code);

static void analyze_tree(tree generic_tree, analyzer_context* context)
{
    if (generic_tree == NULL_TREE) return;
    analyzer tree_analyzer = analyzer_from_tree_code(TREE_CODE(generic_tree));
    if (tree_analyzer == NULL) return;
    tree_analyzer(generic_tree, context);
    context->push_visited(generic_tree);
}

void add_print_for_var(tree func_decl)
{
	analyzer_context* context = new analyzer_context(func_decl);
	analyze_tree(func_decl, context);
	delete context;
}

#define ANALYZE(x, context) analyze_tree((x), context)

static void analyze_tree_list(tree tree_list, analyzer_context* context)
{
	tree element = tree_list;
    while (element != NULL_TREE)
    {
        ANALYZE(TREE_VALUE(element), context);
        ANALYZE(TREE_PURPOSE(element), context);
        element = TREE_CHAIN(element);
    }
}

static void analyze_statement_list(tree stmt_list_tree, analyzer_context* context)
{
	tree_stmt_iterator new_it = tsi_start(stmt_list_tree);
    while (!tsi_end_p(new_it))
    {
    	analyzer_context* new_context = context->new_instance();
    	tree stmt = tsi_stmt(new_it);
        ANALYZE(stmt, new_context);
        // TODO: add code to handle marked var in new_context for printing.
        inject_print(new_it, new_context->set_location(EXPR_FILENAME(stmt), EXPR_LINENO(stmt)), new_context->vars_to_track);
        for (tree var_decl: new_context->vars_to_track)
        {
        	debugger_info_printf("var < %s > is registered for printing.\n", IDENTIFIER_POINTER(DECL_NAME(var_decl)));
        }
        delete new_context;
        tsi_next(&new_it);
    }
}

static void analyze_bind_expr(tree bind_expr, analyzer_context* context)
{
	ANALYZE(BIND_EXPR_BODY(bind_expr), context);
}

static void analyze_var_decl(tree var_decl, analyzer_context* context)
{
	ANALYZE(DECL_NAME(var_decl), context);
	if (is_var_marked_track(var_decl)
		&& (context->in_writing()
		    || (DECL_INITIAL(var_decl) != NULL_TREE && context->in_writing_if_has_init())))
    {
        context->push_var_to_track(var_decl);
        context->unset_writing_if_has_init();
    }
	if (context->has_visited(var_decl)) return;
    tree init = DECL_INITIAL(var_decl);
    if (init != NULL_TREE && init != error_mark_node)
    {
        ANALYZE(init, context);
    }
}

static void analyze_function_decl(tree function_decl, analyzer_context* context)
{
	if (context->has_visited(function_decl)) return;
    ANALYZE(DECL_SAVED_TREE(function_decl), context);
}

static void analyze_ternary_expr(tree cond_expr, analyzer_context* context)
{
	ANALYZE(TREE_OPERAND(cond_expr, 0), context);
    ANALYZE(TREE_OPERAND(cond_expr, 1), context);
    ANALYZE(TREE_OPERAND(cond_expr, 2), context);
}

static void analyze_call_expr(tree call_expr, analyzer_context* context)
{
    tree arg;
    call_expr_arg_iterator cit;
    FOR_EACH_CALL_EXPR_ARG(arg, cit, call_expr)
    {
        ANALYZE(arg, context);
    }
}

static void analyze_unary_expr(tree unary_expr, analyzer_context* context)
{
	ANALYZE(TREE_OPERAND(unary_expr, 0), context);
}

static void analyze_unary_writing_expr(tree unary_expr, analyzer_context* context)
{
	ANALYZE(TREE_OPERAND(unary_expr, 0), context->enter_writing());
	context->exit_writing();
}

static void analyze_binary_expr(tree binary_expr, analyzer_context* context)
{
	ANALYZE(TREE_OPERAND(binary_expr, 0), context);
	ANALYZE(TREE_OPERAND(binary_expr, 1), context);
}

static void analyze_modify_expr(tree binary_expr, analyzer_context* context)
{
	ANALYZE(TREE_OPERAND(binary_expr, 0), context->enter_writing());
	context->exit_writing();
	ANALYZE(TREE_OPERAND(binary_expr, 1), context);
}

static void analyze_compound_literal_expr(tree compound_literal_expr, analyzer_context* context)
{
	ANALYZE(COMPOUND_LITERAL_EXPR_DECL_EXPR(compound_literal_expr), context);
}

static void analyze_decl_expr(tree decl_expr, analyzer_context* context)
{
	ANALYZE(DECL_EXPR_DECL(decl_expr), context->set_writing_if_has_init());
}

/**
 *  find out the proper function to perform DFS given the tree_code.
 */

static analyzer analyzer_from_tree_code(tree_code code)
{
	switch (code)
    {
        case TREE_LIST: return analyze_tree_list;
        case STATEMENT_LIST: return analyze_statement_list;
        case BIND_EXPR: return analyze_bind_expr;
        case PARM_DECL:
        case VAR_DECL: return analyze_var_decl;
        // case TYPE_DECL: return analyze_type_decl;
        case FUNCTION_DECL: return analyze_function_decl; 
        // case RESULT_DECL: return analyze_result_decl;
        // case LABEL_DECL: return analyze_label_decl;
        // case FIELD_DECL: return analyze_field_decl;
        // case METHOD_TYPE:
        // case FUNCTION_TYPE: return analyze_function_type;
        // case ENUMERAL_TYPE: return analyze_enum;
        // case RECORD_TYPE:
        // case UNION_TYPE: return analyze_type;
        // case POINTER_TYPE: return analyze_pointer_type;
        // case INTEGER_TYPE: return analyze_int_type;
        // case ARRAY_TYPE: return analyze_array_type;
        case COND_EXPR:
        case ANNOTATE_EXPR: return analyze_ternary_expr;
        case CALL_EXPR: return analyze_call_expr;
        // case LABEL_EXPR: return analyze_label_expr;
        case NEGATE_EXPR:
        case ABS_EXPR:
        case BIT_NOT_EXPR:
        case TRUTH_NOT_EXPR:
        case FIX_TRUNC_EXPR:
        case FLOAT_EXPR:
        case CONJ_EXPR:
        case REALPART_EXPR:
        case IMAGPART_EXPR:
        case NON_LVALUE_EXPR:
        case CONVERT_EXPR:
        case FIXED_CONVERT_EXPR:
        case CLEANUP_POINT_EXPR:
        case SAVE_EXPR:
        case INDIRECT_REF:
        case RETURN_EXPR:
        case NOP_EXPR: return analyze_unary_expr;
        case ADDR_EXPR:
        case PREDECREMENT_EXPR:
        case PREINCREMENT_EXPR:
        case POSTDECREMENT_EXPR:
        case POSTINCREMENT_EXPR: return analyze_unary_writing_expr;
        case SWITCH_EXPR:
        case COMPLEX_EXPR:
        case LSHIFT_EXPR:
        case RSHIFT_EXPR:
        case BIT_IOR_EXPR:
        case BIT_XOR_EXPR:
        case BIT_AND_EXPR:
        case TRUTH_ANDIF_EXPR:
        case TRUTH_ORIF_EXPR:
        case TRUTH_AND_EXPR:
        case TRUTH_OR_EXPR:
        case TRUTH_XOR_EXPR:
        case POINTER_PLUS_EXPR:
        case POINTER_DIFF_EXPR:
        case PLUS_EXPR:
        case MINUS_EXPR:
        case MULT_EXPR:
        case MULT_HIGHPART_EXPR:
        case RDIV_EXPR:
        case TRUNC_DIV_EXPR:
        case FLOOR_DIV_EXPR:
        case CEIL_DIV_EXPR:
        case ROUND_DIV_EXPR:
        case TRUNC_MOD_EXPR:
        case FLOOR_MOD_EXPR:
        case CEIL_MOD_EXPR:
        case ROUND_MOD_EXPR:
        case EXACT_DIV_EXPR:
        case LT_EXPR:
        case LE_EXPR:
        case GT_EXPR:
        case GE_EXPR:
        case EQ_EXPR:
        case NE_EXPR:
        case ORDERED_EXPR:
        case UNORDERED_EXPR:
        case UNLT_EXPR:
        case UNLE_EXPR:
        case UNGT_EXPR:
        case UNGE_EXPR:
        case UNEQ_EXPR:
        case LTGT_EXPR:
        case INIT_EXPR:
        case COMPOUND_EXPR:
        case TARGET_EXPR:
        case ARRAY_REF:
        case ARRAY_RANGE_REF:
        case MEM_REF:
        case COMPONENT_REF: return analyze_binary_expr;
        case MODIFY_EXPR: return analyze_modify_expr;
        // case CASE_LABEL_EXPR: return analyze_case_expr;
        // case GOTO_EXPR: return analyze_goto_expr;
        case COMPOUND_LITERAL_EXPR: return analyze_compound_literal_expr;
        // case CONSTRUCTOR: return analyze_constructor;
        // case VA_ARG_EXPR: return analyze_va_arg_expr;
        case DECL_EXPR: return analyze_decl_expr;
        // case STRING_CST: return analyze_string_cst;
        // case INTEGER_CST: return analyze_int_cst;
        // case IDENTIFIER_NODE: return analyze_identifier;
        // case TARGET_MEM_REF:
        default: return NULL;
    }
    return NULL;
}

#endif