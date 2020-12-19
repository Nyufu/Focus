#pragma once

#include "../type_traits.hxx"

// clang-format off

namespace abi::x64 {

template <class _Ty>
INLINE constexpr bool is_definable_return_policy_v =
!STD conjunction_v <
	STD is_class< _Ty >,
	STDEX is_register_sized< _Ty >,
	STD negation< STD is_aggregate< _Ty > >,
	STD negation< STD is_polymorphic< _Ty > >,
	STD is_trivially_default_constructible< _Ty >,
	STD is_trivially_copy_constructible< _Ty >,
	STD is_trivially_copy_assignable< _Ty >,
	STD is_trivially_move_constructible< _Ty >,
	STD is_trivially_destructible< _Ty >
>;

template <class _Ty>
struct is_definable_return_policy : STD bool_constant<is_definable_return_policy_v<_Ty>> {};

template <class _Ty>
INLINE constexpr bool is_return_by_register_v =
STD disjunction_v<
	STD is_void< _Ty >,
	STD is_scalar< _Ty >,
	STD is_reference< _Ty >,
	STD conjunction<
		STDEX is_register_sized< _Ty >,
		STD is_aggregate< _Ty >,
		STD negation< STDEX is_aggregate_with_base< _Ty> >,
		STD negation< STDEX is_aggregate_with_reference< _Ty> >
	>,
	STDEX is_xmm< _Ty >
>;

template <class _Ty>
struct is_return_by_register : STD bool_constant<is_return_by_register_v<_Ty>> {};

template <class _Ty>
INLINE constexpr bool is_passed_by_register_v =
STD conjunction_v<
	STD negation< STD is_reference< _Ty > >,
	STDEX is_register_sized< _Ty >
>;

template <class _Ty>
struct is_passed_by_register : STD bool_constant<is_passed_by_register_v<_Ty>> {};

}
