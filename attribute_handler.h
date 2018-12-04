#ifndef ATTRIBUTE_HANDLER_H
#define ATTRIBUTE_HANDLER_H

#include "debugger_common.h"
#include "debugger_shared.h"

/**
 *  a "tracker" is a function that print out the required infomation of a struct which has a signature like tracker(struct foo* ptr, struct context_info* info);
 */

static tree handle_tracker_attribute(tree *node, tree name, tree args __unused, int flags __unused, bool *__unused);
static tree handle_track_value_attribute(tree *node, tree name, tree args __unused, int flags __unused, bool *__unused);
static tree handle_debugger_print_func_attribute(tree *node, tree name, tree args __unused, int flags __unused, bool *__unused);
static tree handle_debugger_jmp_buf_attribute(tree *node, tree name, tree args __unused, int flags __unused, bool *__unused);
static tree handle_debugger_setjmp_attribute(tree *node, tree name, tree args __unused, int flags __unused, bool *__unused);
static tree handle_debugger_entering_risk_attribute(tree *node, tree name, tree args __unused, int flags __unused, bool *__unused);
static tree handle_debugger_exiting_risk_attribute(tree *node, tree name, tree args __unused, int flags __unused, bool *__unused);

static struct attribute_spec tracker = {
    .name               = "tracker",
    .min_length         = 0,
    .max_length         = 0,
    .decl_required          = true,
    .type_required          = false,
    .function_type_required     = false,
    .affects_type_identity      = false,
    .handler            = handle_tracker_attribute
};

/**
 *  "type_to_print_func" stores the "tracker" to a type given its name.
 *  notice that 'typedef struct foo bar' may require two trackers as their keys are 'foo' and 'bar' respectively.
 *  however, if one also defines 'struct bar', then the code may crash, which is a bug to be fixed.
 */

static std::unordered_map<const char*, tree> type_to_print_func;

static std::unordered_set<tree> stored_print_func;

static std::vector<const char*> name_registered;

/**
 *  find out the name in 'const char*' given a tree(RECORD_TYPE)
 */

static const char* type_name_from_type(tree type)
{
	gcc_assert(TREE_CODE(type) == RECORD_TYPE);

	tree name = TYPE_NAME(type);
	// TYPE_NAME(RECORD_TYPE) may get either TYPE_DECL or IDENTIFIER_NODE
	if (TREE_CODE(name) == TYPE_DECL) 
	{
		name = DECL_NAME(name);
	}

	gcc_assert(TREE_CODE(name) == IDENTIFIER_NODE);
	return IDENTIFIER_POINTER(name);
}

/**
 *  push the tracker into the map if 'tracker' for the type of identical name hadn't been pushed yet.
 */

const char* push_print_func(tree func_decl)
{
	if (stored_print_func.count(func_decl) == 0) return NULL; // not a "tracker" function
	// TODO: add signature checking for func_decl here
	gcc_assert(TREE_CODE(func_decl) == FUNCTION_DECL);

	tree arg = DECL_ARGUMENTS(func_decl);
	gcc_assert(TREE_CODE(arg) == PARM_DECL);

	/*tree type_to_print_ptr = TREE_TYPE(arg);  // the first argument should be a POINTER to the type to print
	gcc_assert(TREE_CODE(type_to_print_ptr) == POINTER_TYPE);

	tree type_to_print = TREE_TYPE(type_to_print_ptr);  // getting the type the pointer points to*/

	tree type_to_print = TREE_TYPE(arg);

	const char* name = type_name_from_type(type_to_print);
	for (const char* registered: name_registered)
	{
		if (strcmp(name, registered) == 0)
		{
			debugger_info_printf("printer for < %s > is registered more than once.\n", name);
			// return registered;
		}
	}
	name_registered.push_back(name);
	type_to_print_func[name] = func_decl;
	debugger_info_printf("tracker < %s > has been registered to type < %s >.\n", IDENTIFIER_POINTER(DECL_NAME(func_decl)), name);
	return name;
}

tree retrieve_print_function(tree type)
{
	const char* type_name = type_name_from_type(type);
	for (const char* registered: name_registered)
	{
		if (strcmp(registered, type_name) == 0)
		{
			debugger_info_printf("< %s > uses %p against default print.\n", type_name, type_to_print_func[registered]);
			return type_to_print_func[registered];
		}
	}
	debugger_info_printf("< %s > uses default print.\n", type_name);
	return NULL_TREE;
}

/**
 *  invokes when a function decl with attribute "tracker" was found.
 *  as info like parm_list are not available at this stage, only the decl of the func is stored.
 *  the type that the func prints had not been recorded yet.
 */

static void detected_data_print_func(tree func_decl)
{
	gcc_assert(TREE_CODE(func_decl) == FUNCTION_DECL);
	stored_print_func.emplace(func_decl);
}

static tree handle_tracker_attribute(tree *node, tree name, tree args __unused, int flags __unused, bool *__unused)
{
	detected_data_print_func(*node);
	debugger_info_printf("tracker < %s > has been pushed.\n", IDENTIFIER_POINTER(DECL_NAME(*node)));
	return NULL_TREE;
}

/**
 *  a "track_value" is a variable that should be printed every time its value is used
 *  it hadn't been decided in which cases the value is "used"
 */

static struct attribute_spec track_value = {
    .name               = "track_value",
    .min_length         = 0,
    .max_length         = 2,
    .decl_required          = true,
    .type_required          = false,
    .function_type_required     = false,
    .affects_type_identity      = false,
    .handler            = handle_track_value_attribute
};

/**
 *  "track_value_decls" stores the var_decls to track
 */

struct print_option
{
	bool has_range = false;
	tree range_start = NULL_TREE;
	tree range_end = NULL_TREE;
} EMPTY_PRINT_OPTION;

std::unordered_set<tree> track_value_decls;

bool is_var_marked_track(tree decl)
{
	return track_value_decls.count(decl) > 0;
}

static void retrieve_print_option(tree decl, print_option& option)
{
	if (TREE_CODE(decl) != VAR_DECL || TREE_CODE(decl) != PARM_DECL) return;
	tree attr_list = DECL_ATTRIBUTES(decl);
    if (attr_list == NULL_TREE) return;
    tree bound_attr = TREE_VALUE(attr_list);
    if (bound_attr == NULL_TREE) return;
    tree lb = TREE_VALUE(bound_attr);
    if (lb == NULL_TREE) return;
    bound_attr = TREE_CHAIN(bound_attr);
    if (bound_attr == NULL_TREE) return;
    tree ub = TREE_VALUE(bound_attr);
    if (ub == NULL_TREE) return;

    option.has_range = true;
    option.range_start = lb;
    option.range_end = ub;
}

static void detected_var_to_track(tree decl)
{
	gcc_assert(TREE_CODE(decl) == VAR_DECL || TREE_CODE(decl) == PARM_DECL);
	track_value_decls.emplace(decl);
}

static tree handle_track_value_attribute(tree *node, tree name, tree args __unused, int flags __unused, bool *__unused)
{
    detected_var_to_track(*node);
    debugger_info_printf("var < %s > has been pushed for tracking.\n", IDENTIFIER_POINTER(DECL_NAME(*node)));
    return NULL_TREE;
}

/**
 *  a "debugger_print_func" is a function that handles communication with debugger or print base types like int, char, etc
 *  this kind of function is defined in "debugger.h" which is to be included in the source file to debug.
 */

static struct attribute_spec debugger_print_func = {
    .name               = "debugger_print_func",
    .min_length         = 1,
    .max_length         = 1,
    .decl_required          = true,
    .type_required          = false,
    .function_type_required     = false,
    .affects_type_identity      = false,
    .handler            = handle_debugger_print_func_attribute
};

/**
 *  "debugger_print_func" stores the functions to print base type
 */

tree convert_to_char_pointer(tree char_array_or_pointer)
{
	tree type = TREE_TYPE(char_array_or_pointer);
	gcc_assert(TREE_CODE(type) == ARRAY_TYPE || TREE_CODE(type) == POINTER_TYPE);
	tree element_type = TREE_TYPE(type);
	gcc_assert(element_type == char_type_node);
	if (TREE_CODE(type) == POINTER_TYPE) return char_array_or_pointer;
	return build1(ADDR_EXPR, build_pointer_type(element_type), char_array_or_pointer);
}

bool is_char_type(tree type_tree)
{
	return TREE_CODE(type_tree) == INTEGER_TYPE && !TYPE_UNSIGNED(type_tree) && TYPE_PRECISION(type_tree) == CHAR_PRECISION;
}

bool is_char_pointer_like(tree type_tree)
{
	if (TREE_CODE(type_tree) != POINTER_TYPE && TREE_CODE(type_tree) != ARRAY_TYPE) return false;
	tree element_type = TREE_TYPE(type_tree);
	if (!is_char_type(element_type)) return false;
	return true;
}

static enum base_type get_base_type_from_type_tree(tree type_tree)
{
	if (is_char_pointer_like(type_tree)) return CHAR_POINTER;
	tree_code code = TREE_CODE(type_tree);
	if (code == REAL_TYPE)
	{
		switch (TYPE_PRECISION(type_tree))
		{
			case FLOAT_PRECISION:
				return REAL_FLOAT;
			case DOUBLE_PRECISION:
				return REAL_DOUBLE;
		}
	}
	else if (code == INTEGER_TYPE)
	{
		switch (TYPE_PRECISION(type_tree))
		{
			case CHAR_PRECISION:
				return TYPE_UNSIGNED(type_tree) ? UNSIGNED_CHAR  : SIGNED_CHAR;
			case SHORT_PRECISION:
				return TYPE_UNSIGNED(type_tree) ? UNSIGNED_SHORT : SIGNED_SHORT;
			case INT_PRECISION:
				return TYPE_UNSIGNED(type_tree) ? UNSIGNED_INT   : SIGNED_INT;
			case LONG_PRECISION:
				return TYPE_UNSIGNED(type_tree) ? UNSIGNED_LONG  : SIGNED_LONG;
		}
	}
	else if (code == POINTER_TYPE)
	{
		return POINTER;
	}
	return ERR_BASE_TYPE;
}

bool is_base_type(tree type_tree)
{
	return get_base_type_from_type_tree(type_tree) != ERR_BASE_TYPE;
}

static std::unordered_map<enum base_type, tree> debugger_print_func_decls;

tree get_debugger_print_for_base_type(tree type_tree)
{
	if (!is_base_type(type_tree) || !debugger_print_func_decls.count(get_base_type_from_type_tree(type_tree))) return NULL_TREE;
	return debugger_print_func_decls[get_base_type_from_type_tree(type_tree)];
}

tree get_string_literal_print()
{
	return debugger_print_func_decls[CHAR_POINTER];
}

tree get_debug_context_print()
{
	return debugger_print_func_decls[DEBUG_CONTEXT];
}

static tree handle_debugger_print_func_attribute(tree *node, tree name, tree args, int flags __unused, bool *__unused)
{
	gcc_assert(TREE_CODE(*node) == FUNCTION_DECL);
	gcc_assert(args != NULL_TREE && TREE_CODE(args) == TREE_LIST);
	tree attribute_value = TREE_VALUE(args);
	gcc_assert(TREE_CODE(attribute_value) == INTEGER_CST);
	HOST_WIDE_INT value = TREE_INT_CST_LOW(attribute_value);
    debugger_print_func_decls[(enum base_type) value] = *node;
    debugger_info_printf("func < %-20s > has been pushed for base type < %2ld > printing.\n", IDENTIFIER_POINTER(DECL_NAME(*node)), value);
    return NULL_TREE;
}

/**
 *  stores the jmp_buf that longjmp requires to escape from segfault during local var expansion
 */

static struct attribute_spec debugger_jmp_buf = {
    .name               = "debugger_jmp_buf",
    .min_length         = 0,
    .max_length         = 0,
    .decl_required          = true,
    .type_required          = false,
    .function_type_required     = false,
    .affects_type_identity      = false,
    .handler            = handle_debugger_jmp_buf_attribute
};

tree segf_jmp_buf_decl;

static tree handle_debugger_jmp_buf_attribute(tree *node, tree name, tree args __unused, int flags __unused, bool *__unused)
{
    segf_jmp_buf_decl = *node;
    debugger_info_printf("var < %s > has been pushed as jmp_buf for segfault handling.\n", IDENTIFIER_POINTER(DECL_NAME(*node)));
    return NULL_TREE;
}

/**
 *  stores the decl of setjmp(jmp_buf) that handles segfault during local var expansion
 */

static struct attribute_spec debugger_setjmp = {
    .name               = "debugger_setjmp",
    .min_length         = 0,
    .max_length         = 0,
    .decl_required          = true,
    .type_required          = false,
    .function_type_required     = false,
    .affects_type_identity      = false,
    .handler            = handle_debugger_setjmp_attribute
};

tree setjmp_decl;

static tree handle_debugger_setjmp_attribute(tree *node, tree name, tree args __unused, int flags __unused, bool *__unused)
{
    setjmp_decl = *node;
    debugger_info_printf("func < %s > has been pushed as setjmp for segfault handling.\n", IDENTIFIER_POINTER(DECL_NAME(*node)));
    return NULL_TREE;
}

/**
 *  stores the decl of entering_risk that invokes before expanding local vars
 */

static struct attribute_spec debugger_entering_risk = {
    .name               = "debugger_entering_risk",
    .min_length         = 0,
    .max_length         = 0,
    .decl_required          = true,
    .type_required          = false,
    .function_type_required     = false,
    .affects_type_identity      = false,
    .handler            = handle_debugger_entering_risk_attribute
};

tree entering_risk_decl;

static tree handle_debugger_entering_risk_attribute(tree *node, tree name, tree args __unused, int flags __unused, bool *__unused)
{
    entering_risk_decl = *node;
    debugger_info_printf("func < %s > has been pushed as jmp_buf for risk entering.\n", IDENTIFIER_POINTER(DECL_NAME(*node)));
    return NULL_TREE;
}

/**
 *  stores the decl of exiting_risk that invokes after expanding local vars
 */

static struct attribute_spec debugger_exiting_risk = {
    .name               = "debugger_exiting_risk",
    .min_length         = 0,
    .max_length         = 0,
    .decl_required          = true,
    .type_required          = false,
    .function_type_required     = false,
    .affects_type_identity      = false,
    .handler            = handle_debugger_exiting_risk_attribute
};

tree exiting_risk_decl;

static tree handle_debugger_exiting_risk_attribute(tree *node, tree name, tree args __unused, int flags __unused, bool *__unused)
{
    exiting_risk_decl = *node;
    debugger_info_printf("func < %s > has been pushed as jmp_buf for risk exiting.\n", IDENTIFIER_POINTER(DECL_NAME(*node)));
    return NULL_TREE;
}

// TODO: attribute declaration of the final items are highly replicated and required reconstruction

void register_attributes(void *event_data __unused, void *data __unused)
{
    register_attribute(&tracker);
    register_attribute(&track_value);
    register_attribute(&debugger_print_func);
    register_attribute(&debugger_jmp_buf);
    register_attribute(&debugger_setjmp);
    register_attribute(&debugger_entering_risk);
    register_attribute(&debugger_exiting_risk);
}

#endif
