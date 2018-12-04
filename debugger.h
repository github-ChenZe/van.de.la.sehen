#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "debugger_network.h"
#include "debugger_shared.h"
#include "debugger_exception_handler.h"

/**
 *  this file should be included in the source code to debug
 *  not in the source code of the debugger plugin.
 *  this file should be a c-compatible file.
 */

/**
 *  "debug_context" stores the info to be printed
 *  when data-printing function is invoked
 */

struct debug_context
{
	int line_no;
	const char* file_name;
};

__attribute__((debugger_print_func(SIGNED_CHAR)))
void print_char(char v)
{
	fprintf(stderr, "%c", v);
}

__attribute__((debugger_print_func(UNSIGNED_CHAR)))
void print_uchar(unsigned char v)
{
	fprintf(stderr, "%c", v);
}

__attribute__((debugger_print_func(SIGNED_SHORT)))
void print_short(short v)
{
	fprintf(stderr, "%hd", v);
}

__attribute__((debugger_print_func(UNSIGNED_SHORT)))
void print_ushort(unsigned short v)
{
	fprintf(stderr, "%hu", v);
}

__attribute__((debugger_print_func(SIGNED_INT)))
void print_int(int v)
{
	fprintf(stderr, "%d", v);
}

__attribute__((debugger_print_func(UNSIGNED_INT)))
void print_uint(unsigned int v)
{
	fprintf(stderr, "%u", v);
}

__attribute__((debugger_print_func(SIGNED_LONG)))
void print_long(long int v)
{
	fprintf(stderr, "%ld", v);
}

__attribute__((debugger_print_func(UNSIGNED_LONG)))
void print_ulong(unsigned long int v)
{
	fprintf(stderr, "%lu", v);
}

__attribute__((debugger_print_func(REAL_FLOAT)))
void print_float(float v)
{
	fprintf(stderr, "%f", v);
}

__attribute__((debugger_print_func(REAL_DOUBLE)))
void print_double(double v)
{
	fprintf(stderr, "%lf", v);
}

__attribute__((debugger_print_func(POINTER)))
void print_pointer(void* v)
{
	fprintf(stderr, "%p", v);
}

__attribute__((debugger_print_func(CHAR_POINTER)))
void print_char_pointer(const char* v)
{
	fprintf(stderr, "%s", v);
}

__attribute__((debugger_print_func(DEBUG_CONTEXT)))
struct debug_context build_debug_context(const char* file_name, int line_no)
{
	struct debug_context result;
	result.line_no = line_no;
	result.file_name = file_name;
	fprintf(stderr, "%s:%d:", file_name, line_no);
	return result;
}

#define track_var __attribute__((track_value))
#define track_range(a, b) __attribute__((track_value(a, b)))

#endif