#pragma once

#include <Macros.h>

namespace stdex {

template <class...>
struct type_list;

using empty_type_list = type_list<>;

template <size_t... _Indicies>
using index_list = STD index_sequence<_Indicies...>;

using empty_index_list = index_list<>;

template <class...>
struct _Type_list_append;

template <class _Ty, class... _Types>
struct _Type_list_append<_Ty, type_list<_Types...>> {
	using type = type_list<_Ty, _Types...>;
};

template <class _Ty, class _TypeList>
using append_type_list = typename _Type_list_append<_Ty, _TypeList>::type;

template <size_t, class>
struct _Index_list_append;

template <size_t _Index, size_t... _Indicies>
struct _Index_list_append<_Index, index_list<_Indicies...>> {
	using type = index_list<_Index, _Indicies...>;
};

template <size_t _Index, class _Index_list>
using append_index_list = typename _Index_list_append<_Index, _Index_list>::type;

template <class _Ty>
struct storage {
	alignas(alignof(_Ty)) char _Space[sizeof(_Ty)];
};

template <size_t _Size>
struct padding {
	uint8_t _Space[_Size];
};

template <>
struct padding<0> {};

struct empty {};

template <class... _Types>
struct _SizeofPack;

template <>
struct _SizeofPack<> {};

template <class _Ty>
struct _SizeofPack<_Ty> {
	alignas(alignof(_Ty)) char _Space[sizeof(_Ty)];
};

template <class _Ty, class... _Types>
struct _SizeofPack<_Ty, _Types...> {
	alignas(alignof(_Ty)) char _Space[sizeof(_Ty)];
	_SizeofPack<_Types...> _Rest;
};

template <class... _Types>
INLINE constexpr size_t sizeof_pack = sizeof(_SizeofPack<_Types...>);

}