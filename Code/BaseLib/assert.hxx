#pragma once

#include <Macros.h>
#include <gsl/assert>

#ifndef NDEBUG // FIX implement an own report window, because the _CrtDbgReportW does not link with release crt.

#undef ASSERT

#define ASSERT(expression)                                                                                                 \
	{                                                                                                                      \
		if (!!!(expression)) {                                                                                             \
			if (::_CrtDbgReportW(_CRT_ASSERT, WIDE(__FILE__), (__LINE__), NULL, L"%ls", WIDE(STRINGIZE(expression))) == 1) \
				__debugbreak();                                                                                            \
			__fastfail(FAST_FAIL_FATAL_APP_EXIT);                                                                          \
		}                                                                                                                  \
	}

#else

#define ASSERT(expression) GSL_ASSUME(expression)

#endif
