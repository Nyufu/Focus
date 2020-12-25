#pragma once

namespace config {

inline constexpr bool debug =
#if defined(NDEBUG)
#if defined(ASAN)
	true;
#else
	false;
#endif
#else
	true;
#endif

}

#define FAST_WINAPI_CALL 1

// vvv Windows.h config vvv

#define WIN32_LEAN_AND_MEAN
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCOMM
#define NOCTLMGR
#define NODEFERWINDOWPOS
#define NODRAWTEXT
//#define NOGDI
//#define NOGDICAPMASKS
#define NOHELP
#define NOICONS
#define NOIME
#define NOKANJI
#define NOKERNEL
#define NOKEYSTATES
#define NOMB
#define NOMCX
#define NOMEMMGR
#define NOMENUS
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NONLS
#define NOOPENFILE
#define NOPROFILER
#define NORASTEROPS
#define NOSCROLL
#define NOSERVICE
#define NOSHOWWINDOW
#define NOSOUND
#define NOSYSCOMMANDS
#define NOSYSMETRICS
#define NOTEXTMETRIC
#define NOUSER
#define NOVIRTUALKEYCODES
#define NOWH
#define NOWINMESSAGES
#define NOWINOFFSETS
#define NOWINSTYLES
#define OEMRESOURCE

// vvv STL config vvv

#define _STL_EXTRA_DISABLED_WARNINGS
