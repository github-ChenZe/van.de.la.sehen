#ifndef DEBUGGER_SHARED_H
#define DEBUGGER_SHARED_H

/**
 *  this files includes the declarations that both <debugger.h> and the debugger plugin would include.
 */

#define CHAR_PRECISION    8
#define SHORT_PRECISION  16
#define INT_PRECISION    32
#define LONG_PRECISION   64
#define FLOAT_PRECISION  32
#define DOUBLE_PRECISION 64

enum base_type
{
	SIGNED_CHAR, UNSIGNED_CHAR, SIGNED_SHORT, UNSIGNED_SHORT, SIGNED_INT, UNSIGNED_INT, SIGNED_LONG, UNSIGNED_LONG,
	REAL_FLOAT, REAL_DOUBLE,
	POINTER,
	CHAR_POINTER,
	DEBUG_CONTEXT,
	ERR_BASE_TYPE
};

#endif