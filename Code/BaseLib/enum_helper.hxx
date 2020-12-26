#pragma once

#include "type_traits.hxx"

template <class _Enum>
struct _Enum_Count_Error {
	static_assert(STDEX always_false_v<_Enum>, "Please specialize the enum_count variable with <_Enum> to use!");
};

template <class _Enum>
INLINE constexpr size_t enum_count = _Enum_Count_Error<_Enum>::value;

namespace stdex {

template <class _Enum>
constexpr auto to_num(_Enum value) noexcept -> STD enable_if_t<STD is_enum_v<_Enum>, STD underlying_type_t<_Enum>> {
	return static_cast<STD underlying_type_t<_Enum>>(value);
}

template <typename _Key, typename _Value, size_t _Size = enum_count<_Key>>
struct static_enum_map : STD array<_Value, _Size> {
	using base = STD array<_Value, _Size>;

	static_assert(STD is_enum_v<_Key>, "This compile time container designed only for enum keys");

public:
	constexpr typename base::reference operator[](const _Key key) noexcept {
		return base::operator[](to_num(key));
	}

	constexpr typename base::const_reference operator[](const _Key key) const noexcept {
		return base::operator[](to_num(key));
	}

	template <size_t _N>
	constexpr static_enum_map(const STD pair<const _Key, const _Value> (&items)[_N]) noexcept
		: static_enum_map{ items, STD make_index_sequence<_N>() } {
	}

private:
	template <size_t _N, size_t... _I>
	constexpr static_enum_map(const STD pair<const _Key, const _Value> (&items)[_N], STD index_sequence<_I...>) noexcept
		: base{ get<_I>(items, STD make_index_sequence<_N>())... } {
	}

	template <size_t _ArrayIndex, size_t _N, size_t _Inext, size_t... _Indices, typename = STD enable_if_t<sizeof...(_Indices) != 0>>
	constexpr const _Value& get(const STD pair<const _Key, const _Value> (&items)[_N], STD index_sequence<_Inext, _Indices...>) noexcept {
		return to_num(items[_Inext].first) == _ArrayIndex ? items[_Inext].second : get<_ArrayIndex>(items, STD index_sequence<_Indices...>());
	}

	template <size_t _ArrayIndex, size_t _N, size_t _Inext>
	constexpr const _Value& get(const STD pair<const _Key, const _Value> (&items)[_N], STD index_sequence<_Inext>) noexcept {
		return to_num(items[_Inext].first) == _ArrayIndex ? items[_Inext].second : KeyIsMissingError<const _Value&>();
	}

	template <class _Ty>
	_Ty KeyIsMissingError() noexcept; // This cause compile time error if an element is missing from the initializer list
};

}
