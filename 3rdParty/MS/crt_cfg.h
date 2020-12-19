#include <time.h>

#define WIN32_LEAN_AND_MEAN 1
#define CRTDLL 1
#define CRTDLL2 1
#define CONCRTDLL 1
#define _CRTBLD 1
#define _CORECRT_BUILD 1
#define _VCRT_BUILD 1
#define _VCRT_ALLOW_INTERNALS 1
#define _CRT_DECLARE_GLOBAL_VARIABLES_DIRECTLY 1
#define _CRT_GLOBAL_STATE_ISOLATION 1
#define _STL_WIN32_WINNT _WIN32_WINNT_WINXP
#define _VCRT_WIN32_WINNT _WIN32_WINNT_WINXP
#define _RTC 1
#define _HAS_EXCEPTIONS 0
#define DECLSPEC_GUARDNOCF
#define DECLSPEC_GUARD_SUPPRESS

typedef void (__cdecl* new_handler) ();