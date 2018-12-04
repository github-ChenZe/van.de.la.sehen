#ifndef PRINT_INJECTOR_H
#define PRINT_INJECTOR_H

#include "debugger_common.h"
#include "analyzer_context.h"

tree inject_seg_protector(tree_stmt_iterator& it, analyzer_context* context)
{
	tree entering_risk_call = build_call_expr(entering_risk_decl, 0);
	tsi_link_after(&it, entering_risk_call, TSI_CONTINUE_LINKING);

	tree break_label_decl = build_decl(UNKNOWN_LOCATION, LABEL_DECL, NULL_TREE, void_type_node);
	DECL_CONTEXT(break_label_decl) = context->context_func_decl;
	tree break_label_expr = build1(LABEL_EXPR, void_type_node, break_label_decl);

	tree expansion_body_label_decl = build_decl(UNKNOWN_LOCATION, LABEL_DECL, NULL_TREE, void_type_node);
	DECL_CONTEXT(expansion_body_label_decl) = context->context_func_decl;
	tree expansion_body_label_expr = build1(LABEL_EXPR, void_type_node, expansion_body_label_decl);

	tree addr_of_jmp_buf = build1(ADDR_EXPR, build_pointer_type(integer_type_node), segf_jmp_buf_decl);
	tree setjmp_expr = build_call_expr(setjmp_decl, 1, addr_of_jmp_buf);
	tree compare_expr = build2(EQ_EXPR, integer_type_node, setjmp_expr, to_int_cst(0));
	tree continue_expr = build1(GOTO_EXPR, void_type_node, expansion_body_label_decl);
	tree break_expr = build1(GOTO_EXPR, void_type_node, break_label_decl);
	tree cond_expr = build3(COND_EXPR, void_type_node, compare_expr, continue_expr, break_expr);

	tsi_link_after(&it, cond_expr, TSI_CONTINUE_LINKING);
	tsi_link_after(&it, expansion_body_label_expr, TSI_CONTINUE_LINKING);
	return break_label_expr;
}

void escape_seg_protector(tree_stmt_iterator& it, tree break_label_expr)
{
	tsi_link_after(&it, break_label_expr, TSI_CONTINUE_LINKING);

	tree exiting_risk_call = build_call_expr(exiting_risk_decl, 0);
	tsi_link_after(&it, exiting_risk_call, TSI_CONTINUE_LINKING);
}

static void inject_print_string_literal(tree_stmt_iterator& it, int padding_incr, analyzer_context* context, const char* fmt, ...)
{
	char buff[1024];

	static int current_padding = 0;

	if (padding_incr < 0) current_padding += padding_incr;

	for (int i = 0; i < current_padding; i++)
    {
    	buff[i] = ' ';
    }

	va_list myargs;
    va_start(myargs, fmt);
    vsprintf(buff + current_padding, fmt, myargs);
    va_end(myargs);

	tsi_link_after(
		&it, 
        build_call_expr(
        	get_string_literal_print(),
        	1,
            to_str_cst(buff)),
        TSI_CONTINUE_LINKING);

	if (padding_incr > 0) current_padding += padding_incr;
}

#define IN_DISPLAY(fmt, ...) inject_print_string_literal(it, 4, context, fmt, ##__VA_ARGS__)
#define PADDING_DISPLAY() inject_print_string_literal(it, 0, context, "")
#define NEWLINE_DISPLAY() inject_print_string_literal(it, 0, context, "\n")
#define FLAT_DISPLAY(fmt, ...) inject_print_string_literal(it, 0, context, fmt, ##__VA_ARGS__)
#define OUT_DISPLAY(fmt, ...) inject_print_string_literal(it, -4, context, fmt, ##__VA_ARGS__)

/**
 *  an "injector" is a method of signature(tree_stmt_iterator&, tree expr_or_decl) 
 *  which inject printing method according to the tree type by the iterator,
 *  then (if necessary) perfrom further injection on inner content of the tree by calling proper function.
 */

typedef void (*injector)(tree_stmt_iterator& it, analyzer_context* context, tree expr);

static injector injector_from_tree_type(tree type);

static void inject_print_on_generic(tree_stmt_iterator& it, analyzer_context* context, tree generic_expr)
{
	injector injector_for_expr = injector_from_tree_type(TREE_TYPE(generic_expr));
	if (injector_for_expr == NULL)
	{
		debugger_err_printf("NULL injector encountered for tree < %s > of tree_type < %s >.\n",
			get_tree_code_name(TREE_CODE(generic_expr)),
			get_tree_code_name(TREE_CODE(TREE_TYPE(generic_expr))));
		return;
	}
	tree break_label_expr = inject_seg_protector(it, context);
	injector_for_expr(it, context, generic_expr);
	escape_seg_protector(it, break_label_expr);
}

static void inject_print_on_base_type(tree_stmt_iterator& it, analyzer_context* context, tree base_type_expr)
{
	PADDING_DISPLAY();
	tsi_link_after(
		&it, 
        build_call_expr(
        	get_debugger_print_for_base_type(TREE_TYPE(base_type_expr)),
        	1, 
            base_type_expr), 
        TSI_CONTINUE_LINKING);
	NEWLINE_DISPLAY();
}

static void build_for_loop(tree_stmt_iterator& it, analyzer_context* context, tree start, tree end, tree ptr, 
							   void (*build_ref)(tree_stmt_iterator&, analyzer_context*, tree, tree))
{
	tree decl = build_decl(UNKNOWN_LOCATION, VAR_DECL, get_identifier ("__intrinsic__"), integer_type_node);
	DECL_CONTEXT(decl) = context->context_func_decl;
	DECL_SEEN_IN_BIND_EXPR_P(decl) = 1; // cheat the gimplfy checker
	// decl->decl_with_vis.seen_in_bind_expr = 1;
	tree decl_expr = build1(DECL_EXPR, integer_type_node, decl);
	tsi_link_after(
		&it, 
        decl_expr, 
        TSI_CONTINUE_LINKING);

	tree ass_expr = build2(MODIFY_EXPR, integer_type_node, decl, start);
	tsi_link_after(
		&it, 
        ass_expr, 
        TSI_CONTINUE_LINKING);

	// insert at the end of the loop. built here as init_goto uses it.
	tree cond_label_decl = build_decl(UNKNOWN_LOCATION, LABEL_DECL, NULL_TREE, void_type_node);
	DECL_CONTEXT(cond_label_decl) = context->context_func_decl;
	tree cond_label_expr = build1(LABEL_EXPR, void_type_node, cond_label_decl);

	tree init_goto = build1(GOTO_EXPR, void_type_node, cond_label_decl);
	tsi_link_after(
		&it, 
        init_goto, 
        TSI_CONTINUE_LINKING);

	tree loop_execution_label_decl = build_decl(UNKNOWN_LOCATION, LABEL_DECL, NULL_TREE, void_type_node);
	DECL_CONTEXT(loop_execution_label_decl) = context->context_func_decl;
	tree loop_execution_label_expr = build1(LABEL_EXPR, void_type_node, loop_execution_label_decl);
	tsi_link_after(
		&it, 
        loop_execution_label_expr,
        TSI_CONTINUE_LINKING);

	IN_DISPLAY("<item>\n");
	build_ref(it, context, ptr, decl);
	OUT_DISPLAY("</item>\n");

	tree increment_expr = build2(POSTINCREMENT_EXPR, integer_type_node, decl, to_int_cst(1));
	tsi_link_after(
		&it, 
        increment_expr, 
        TSI_CONTINUE_LINKING);

	tree break_label_decl = build_decl(UNKNOWN_LOCATION, LABEL_DECL, NULL_TREE, void_type_node);
	DECL_CONTEXT(break_label_decl) = context->context_func_decl;
	tree break_label_expr = build1(LABEL_EXPR, void_type_node, break_label_decl);

	tsi_link_after(
		&it, 
        cond_label_expr, 
        TSI_CONTINUE_LINKING);

	tree compare_expr = build2(LE_EXPR, integer_type_node, decl, end);
	tree continue_expr = build1(GOTO_EXPR, void_type_node, loop_execution_label_decl);
	tree break_expr = build1(GOTO_EXPR, void_type_node, break_label_decl);
	tree cond_expr = build3(COND_EXPR, void_type_node, compare_expr, continue_expr, break_expr);

	tsi_link_after(
		&it, 
        cond_expr, 
        TSI_CONTINUE_LINKING);

	tsi_link_after(
		&it, 
        break_label_expr, 
        TSI_CONTINUE_LINKING);
}

static void build_ptr_ref(tree_stmt_iterator& it, analyzer_context* context, tree ptr, tree index)
{
	tree type_size = TYPE_SIZE(TREE_TYPE(TREE_TYPE(ptr)));  // Double TREE_TYPE: the first one gets POINTER_TYPE, the second one get the TYPE be pointed to
	unsigned long int size_byte = TREE_INT_CST_LOW(type_size) / 8;
	// NOPs must be build here, otherwise an internal error of 'internal compiler error: in emit_move_insn, at expr.c:3722' may occur for unknwon reason
	tree pointer_index = build2(MULT_EXPR, long_unsigned_type_node, build1(NOP_EXPR, long_unsigned_type_node, index), to_ptr_off_cst(size_byte));
	tree pointer_plus = build2(POINTER_PLUS_EXPR, TREE_TYPE(ptr), build1(NOP_EXPR, TREE_TYPE(ptr), ptr), pointer_index);
	tree element_on_index = build1(INDIRECT_REF, TREE_TYPE(TREE_TYPE(ptr)), pointer_plus);
	inject_print_on_generic(it, context, element_on_index);
}

static void inject_print_on_pointer(tree_stmt_iterator& it, analyzer_context* context, tree pointer_type_expr)
{
	print_option option = print_option();
	retrieve_print_option(pointer_type_expr, option);
	IN_DISPLAY("<pointer>\n");
	PADDING_DISPLAY();
	tree break_label_expr = inject_seg_protector(it, context);
	tsi_link_after(
		&it, 
        build_call_expr(
        	get_debugger_print_for_base_type(TREE_TYPE(pointer_type_expr)),
        	1, 
            pointer_type_expr), 
        TSI_CONTINUE_LINKING);
	NEWLINE_DISPLAY();
	// printf("Print_option: %d %p %p\n", option.has_range, option.range_start, option.range_end);
	if (option.has_range)
	{
		tree end_index = build2(MINUS_EXPR, integer_type_node, option.range_end, to_int_cst(1)); // for loop uses LE (<=)
		build_for_loop(it, context, option.range_start, end_index, pointer_type_expr, build_ptr_ref);
		escape_seg_protector(it, break_label_expr);
		OUT_DISPLAY("</pointer>\n");
		return;
	}
	
	// The first TREE_TYPE results in a POINTER_TYPE, thus append a second TREE_TYPE to get the type that the pointer points to 
	tree after_dereference = build1(INDIRECT_REF, TREE_TYPE(TREE_TYPE(pointer_type_expr)), pointer_type_expr);
	IN_DISPLAY("<dereference>\n");
	inject_print_on_generic(it, context, after_dereference);
	OUT_DISPLAY("</dereference>\n");
	escape_seg_protector(it, break_label_expr);
	OUT_DISPLAY("</pointer>\n");
}

long int get_array_lower_bound(tree array_type)
{
    return TREE_INT_CST_LOW(TYPE_MIN_VALUE(TYPE_DOMAIN(array_type)));
}

long int get_array_upper_bound(tree array_type)
{
    return TREE_INT_CST_LOW(TYPE_MAX_VALUE(TYPE_DOMAIN(array_type)));
}

static void build_array_ref(tree_stmt_iterator& it, analyzer_context* context, tree array, tree index)
{
	tree element_on_index = build4(ARRAY_REF, TREE_TYPE(TREE_TYPE(array)), array, index, NULL_TREE, NULL_TREE);
	inject_print_on_generic(it, context, element_on_index);
}

static void inject_print_on_array(tree_stmt_iterator& it, analyzer_context* context, tree array_type_expr)
{
	if (is_char_pointer_like(TREE_TYPE(array_type_expr))) // is array of char, thus print as a string
	{
		inject_print_on_generic(it, context, convert_to_char_pointer(array_type_expr));
		return;
	}
	tree lb = to_int_cst(get_array_lower_bound(TREE_TYPE(array_type_expr)));
    tree ub = to_int_cst(get_array_upper_bound(TREE_TYPE(array_type_expr)));
    IN_DISPLAY("<array>\n");
    build_for_loop(it, context, lb, ub, array_type_expr, build_array_ref);
    OUT_DISPLAY("</array>\n");
}

static void inject_print_on_record(tree_stmt_iterator& it, analyzer_context* context, tree record_type_expr)
{
	tree record_type = TREE_TYPE(record_type_expr);
	tree print_function;
	if ((print_function = retrieve_print_function(record_type)) != NULL)
	{
		tree break_label_expr = inject_seg_protector(it, context);
		tsi_link_after(
			&it, 
        	build_call_expr(
        		print_function,
        		1, 
            	record_type_expr),
        	TSI_CONTINUE_LINKING);
		escape_seg_protector(it, break_label_expr);
		return;
	}
	if (context->has_expanded(record_type))
	{
		FLAT_DISPLAY("<__RECURSION__/>\n");
		return;
	}
	context->push_expanded(record_type);
	tree element = TYPE_FIELDS(record_type);	
    while (element != NULL_TREE)
    {
    	IN_DISPLAY("<FIELD_%s>\n", IDENTIFIER_POINTER(DECL_NAME(element)));
        tree member_ref = build3(COMPONENT_REF, TREE_TYPE(element), record_type_expr, element, NULL_TREE);
        inject_print_on_generic(it, context, member_ref);
        OUT_DISPLAY("</FIELD_%s>\n", IDENTIFIER_POINTER(DECL_NAME(element)));
        element = TREE_CHAIN(element);
    }
}

/**
 *  "injector_from_tree_type" decides the proper function to perform injection for the tree
 */

static injector injector_from_tree_type(tree type)
{
	switch (TREE_CODE(type))
	{
		case POINTER_TYPE:
			return inject_print_on_pointer;
		case ARRAY_TYPE:
			return inject_print_on_array;
		case RECORD_TYPE:
			return inject_print_on_record;
		default:
			break;
	}
	if (is_base_type(type)) return inject_print_on_base_type;
	return NULL;
}

static void inject_print_context(tree_stmt_iterator& it, analyzer_context* context, const char* file_path, int line_no)
{
	tsi_link_after(
		&it, 
        build_call_expr(
        	get_debug_context_print(),
        	2,
            build_string_literal_of_source_file_path(file_path),
            to_int_cst(line_no)), 
        TSI_CONTINUE_LINKING);
}

void inject_print(tree_stmt_iterator& it, analyzer_context* context, std::deque<tree> vars_to_track)
{
	context->clear_expanded();
	if (vars_to_track.size() == 0) return;

	IN_DISPLAY("<vars_info>\n");
	// TODO: abstract the code snap for label generation
	// segfault handling during var expansion

	PADDING_DISPLAY();
	inject_print_context(it, context, context->file_name, context->line_no);
	NEWLINE_DISPLAY();
	for (tree var_decl: vars_to_track)
	{
		IN_DISPLAY("<IDENTIFIER_%s>\n", IDENTIFIER_POINTER(DECL_NAME(var_decl)));

		tree break_label_expr = inject_seg_protector(it, context);

		gcc_assert(TREE_CODE(var_decl) == VAR_DECL || TREE_CODE(var_decl) == PARM_DECL);
		inject_print_on_generic(it, context, var_decl);

		escape_seg_protector(it, break_label_expr);

		OUT_DISPLAY("</IDENTIFIER_%s>\n", IDENTIFIER_POINTER(DECL_NAME(var_decl)));
	}
	OUT_DISPLAY("</vars_info>\n");

}

#endif