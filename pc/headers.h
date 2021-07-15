//	$Id: headers.h,v 1.1.1.1 2000/01/04 14:48:30 cisc Exp $

#ifndef WIN_HEADERS_H
#define WIN_HEADERS_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE		0x200

//#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
//#include <process.h>
#include <assert.h>

#ifdef _MSC_VER
	#undef max
	#define max _MAX
	#undef min
	#define min _MIN
#endif

#endif	// WIN_HEADERS_H
