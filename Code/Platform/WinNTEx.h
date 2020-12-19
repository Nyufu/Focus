#pragma once

#include <Macros.h>

WARNING_SCOPE_BEGIN
MSVC_WARNING_DISABLE(4201)
#include <ntpebteb.h>
WARNING_SCOPE_END

#define GetPeb_()				(reinterpret_cast<PPEB >(__readgsqword(FIELD_OFFSET(TEB, ProcessEnvironmentBlock))))

#if (defined (FAST_WINAPI_CALL) && FAST_WINAPI_CALL)

#define GetCurrentFiber_()		(reinterpret_cast<PVOID>(__readgsqword(FIELD_OFFSET(NT_TIB, FiberData))))
#define GetCurrentThreadId_()	(static_cast<HANDLE	   >(reinterpret_cast<CLIENT_ID>(__readgsqword(FIELD_OFFSET(TEB, ClientId))).UniqueThread))
#define GetProcessHeap_()		(static_cast<HANDLE	   >(GetPeb_()->ProcessHeap))
#define GetTeb_()				(reinterpret_cast<PTEB >(__readgsqword(FIELD_OFFSET(NT_TIB, Self))))

#else

#define GetCurrentFiber_()		(::GetCurrentFiber())
#define GetCurrentThreadId_()	(::GetCurrentThreadId())
#define GetProcessHeap_()		(::GetProcessHeap())
#define GetTeb_()				(reinterpret_cast<PTEB>(::NtCurrentTeb()))

#endif