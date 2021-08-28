//	$Id: headers.h,v 1.1.1.1 2000/01/04 14:48:30 cisc Exp $

#ifndef WIN_HEADERS_H
#define WIN_HEADERS_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE		0x200
#define MAX_PATH 260

//#include <stdio.h>
#include <ctype.h>
//#include <string.h>
#include <unistd.h>     // unix
#include <fcntl.h>      // file control
#include <errno.h>      // error number definitions
#include <termios.h>    // posix terminal stuff
#include <sys/ioctl.h>

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

#define ASSERT_NO_ERROR(result) if(result < 0) { fprintf(stderr, "%s:%i: Fatal error in last operation: %s\n", __FILE__, __LINE__, strerror(errno)); assert(result >= 0); }

#endif	// WIN_HEADERS_H
