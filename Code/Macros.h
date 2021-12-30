#pragma once

// Copy the changes to cpp.hint too

#define ONLY_CONSTRUCT(T)            \
	T(const T&) = delete;            \
	T(T&&) = delete;                 \
									 \
	T& operator=(const T&) = delete; \
	T& operator=(T&&) = delete;

#if !defined(__INTELLISENSE__)

#if defined(__CLANG__)

#define WARNING_SCOPE_BEGIN _Pragma("clang diagnostic push")

#define WARNING_SCOPE_END _Pragma("clang diagnostic pop")

#define _CLANG_WARNING_DISABLE_HELPER_0(x) #x
#define _CLANG_WARNING_DISABLE_HELPER_1(x) _CLANG_WARNING_DISABLE_HELPER_0(clang diagnostic ignored x)
#define _CLANG_WARNING_DISABLE_HELPER_2(y) _CLANG_WARNING_DISABLE_HELPER_1(y)
#define CLANG_WARNING_DISABLE(x) _Pragma(_CLANG_WARNING_DISABLE_HELPER_2(x))

#define MSVC_WARNING_DISABLE(...)

#define TO_CONSTANT(Type, Value) (__builtin_constant_p((Type)Value) ? (Type)Value : (Type)Value)

#elif defined(__MSVC__)

#define WARNING_SCOPE_BEGIN __pragma(warning(push))

#define WARNING_SCOPE_END __pragma(warning(pop))

#define MSVC_WARNING_DISABLE(...) __pragma(warning(disable : __VA_ARGS__))

#define CLANG_WARNING_DISABLE(x)

#define TO_CONSTANT(Type, Value) (reinterpret_cast<Type>(Value))

#else
#error "Something went wrong in compiler configuration"
#endif

#define INLINE inline

#else

#define WARNING_SCOPE_BEGIN
#define WARNING_SCOPE_END

#define CLANG_WARNING_DISABLE(x)
#define MSVC_WARNING_DISABLE(...)

#define INLINE

#define TO_CONSTANT(Type, Value) (reinterpret_cast<Type>(Value))

#endif

#define _CONCATENATE_(a, b) a##b
#define CONCATENATE(a, b) _CONCATENATE_(a, b)

#define _STRINGIZE_(x) #x
#define STRINGIZE(x) _STRINGIZE_(x)

#define _WIDE_(s) L##s
#define WIDE(s) _WIDE_(s)


#if !defined(NDEBUG)
#define SUPPRESS_INITIALIZE(x) = x
#else
#define SUPPRESS_INITIALIZE(x)
#endif

#if _MSC_VER >= 1929 // VS2019 v16.10 and later (_MSC_FULL_VER >= 192829913 for VS 2019 v16.9)
// Works with /std:c++14 and /std:c++17, and performs optimization

#define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]

#else

// no-op in MSVC v14x ABI
#define NO_UNIQUE_ADDRESS /* [[no_unique_address]] */

#endif