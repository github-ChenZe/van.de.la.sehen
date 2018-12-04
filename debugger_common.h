#ifndef DEBUGGER_COMMON_H
#define DEBUGGER_COMMON_H

#include "gcc-plugin.h"
#include "plugin-version.h"
#include "tree-pass.h"
#include "context.h"
#include "tree.h"
#include "tree-core.h"
#include <unistd.h>
#include <stdio.h>
#include <limits.h>

#include "tree-iterator.h"
#include <unordered_set>
#include "stringpool.h"
#include "langhooks.h"
#include "cgraph.h"
#include "gimple.h"
#include "gimple-iterator.h"
#include "gimple-ssa.h"
#include "cp/cp-tree.h"
#include "c-family/c-common.h"
#include <stdarg.h>
#include <unordered_map>
#include <vector>
#include <deque>
#include <stack>
#include <string.h>

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <setjmp.h>

// #define DEBUGGING
#define ENABLE_TREE_CHECKING

int plugin_is_GPL_compatible;

tree to_int_cst(int val)
{
    return build_int_cst(integer_type_node, val);
}

tree to_ptr_off_cst(int val)
{
    return build_int_cst(long_unsigned_type_node, val);
}

tree to_str_cst(const char* str)
{
    return build_string_literal(strlen(str) + 1, str);
}

tree build_string_literal_of_source_file_path(const char* source_file_name)
{
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    size_t len = strlen(cwd);
    cwd[len] = '/';
    cwd[len + 1] = 0;
    strcpy(cwd + strlen(cwd), source_file_name);
    return build_string_literal(strlen(cwd) + 1, cwd);
}

bool is_record_type(tree type)
{
	return TREE_CODE(type) == RECORD_TYPE || TREE_CODE(type) == UNION_TYPE;
}

void segfault_handler(int sig)
{
    void* array[10];
    size_t size;

    size = backtrace(array, 10);
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

int debugger_info_printf(const char* fmt, ...)
{
    int ret;
    va_list myargs;
    va_start(myargs, fmt);
    ret = vfprintf(stdout, fmt, myargs);
    va_end(myargs);
    return ret;
}

int debugger_err_printf(const char* fmt, ...)
{
    int ret;
    va_list myargs;
    va_start(myargs, fmt);
    ret = vfprintf(stderr, fmt, myargs);
    va_end(myargs);
    return ret;
}

void survive_with_count()
{
	static int count = 0;
	printf("survived %d\n", count++);
}

#define SUR survive_with_count()

#endif
