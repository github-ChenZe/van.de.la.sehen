#ifndef ANALYZER_CONTEXT_H
#define ANALYZER_CONTEXT_H

#include "debugger_common.h"

static std::unordered_set<tree> visited;

struct analyzer_context
{
	std::deque<tree> vars_to_track;

	/** 
	 *  "expanded" stores the type_decls that had been expanded for printing
	 *  to avoid recursion, types that had benn expanded would not be expanded again
	 *  "expanded" is cleared everytime a new turn of adding printing exprssions is started
	 */
	
	std::unordered_set<tree> expanded;
	const char* file_name;
	int line_no;
	int writing_expr_entering_count = 0;
	bool writing_if_has_init = false;
	tree context_func_decl;
public:
	analyzer_context(tree context_func)
	{
		this->context_func_decl = context_func;
	}
	analyzer_context* new_instance()
	{
		return new analyzer_context(context_func_decl);
	}
	analyzer_context* enter_writing()
	{
		writing_expr_entering_count++;
		return this;
	}
	void exit_writing()
	{
		if (writing_expr_entering_count <= 0)
		{
			debugger_err_printf("Exiting from writing but writing_count < 0.\n");
			return;
		} 
		writing_expr_entering_count--;
	}
	bool in_writing()
	{
		return writing_expr_entering_count > 0;
	}
	bool has_visited(tree node)
	{
		return visited.count(node) > 0;
	}
	void push_visited(tree node)
	{
		visited.emplace(node);
	}
	bool has_expanded(tree type_decl)
	{
		return expanded.count(type_decl) > 0;
	}
	void push_expanded(tree type_decl)
	{
		expanded.emplace(type_decl);
	}
	void clear_expanded()
	{
		expanded.clear();
	}
	void push_var_to_track(tree decl)
	{
		for (tree pushed: vars_to_track)
		{
			if (decl == pushed) return;
		}
		vars_to_track.push_back(decl);
	}
	tree pop_var_to_track()
	{
		tree var_decl = vars_to_track.front();
		vars_to_track.pop_front();
		return var_decl;
	}
	analyzer_context* set_writing_if_has_init()
	{
		writing_if_has_init = true;
		return this;
	}
	analyzer_context* unset_writing_if_has_init()
	{
		writing_if_has_init = false;
		return this;
	}
	bool in_writing_if_has_init()
	{
		return writing_if_has_init;
	}
	analyzer_context* set_location(const char* file_name, int line_no)
	{
		this->file_name = file_name;
		this->line_no   = line_no;
		return this;
	}
};


#endif