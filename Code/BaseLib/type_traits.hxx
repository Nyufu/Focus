#pragma once

#include <Macros.h>

namespace stdex {

// false value attached to a dependent name (for static_assert)
template <class>
INLINE constexpr bool always_false_v = false;

template <class _Ty>
INLINE constexpr bool is_register_sized_v = (sizeof(_Ty) == 1Ui64) || (sizeof(_Ty) == 2Ui64) || (sizeof(_Ty) == 4Ui64) || (sizeof(_Ty) == 8Ui64);

template <>
INLINE constexpr bool is_register_sized_v<void> = false;

template <>
INLINE constexpr bool is_register_sized_v<const void> = false;

template <class _Ty>
struct is_register_sized : STD bool_constant<is_register_sized_v<_Ty>> {};

template <size_t _Align>
struct _Alignment_error {
#if !defined(__clang_major__) || (defined(__clang_major__) && __clang_major__ < 12) // Clang 12 template instantiation issue workaround
	static_assert(always_false_v<_Align>, "alignment must be power of two");
#endif
};

template <size_t _Align>
INLINE constexpr size_t _Alignment_validate = _Alignment_error<_Align>::value;

template <>
INLINE constexpr size_t _Alignment_validate<1> = 1;

template <>
INLINE constexpr size_t _Alignment_validate<2> = 2;

template <>
INLINE constexpr size_t _Alignment_validate<4> = 4;

template <>
INLINE constexpr size_t _Alignment_validate<8> = 8;

template <>
INLINE constexpr size_t _Alignment_validate<16> = 16;

template <size_t _Val, size_t _Align>
INLINE constexpr size_t aligned_v = (_Val + _Alignment_validate<_Align> - 1) & ~(_Alignment_validate<_Align> - 1);

template <size_t _Val, size_t _Align>
INLINE constexpr bool is_aligned_v = (_Val & _Alignment_validate<_Align> - 1) == 0;

// https://stackoverflow.com/a/46196936/2471710
template <class _Ty>
struct _Any_base {
	operator _Ty() = delete;

	template <class _Base, class = STD enable_if_t<STD is_base_of_v<_Base, _Ty>>>
	operator _Base();
};

template <class _Ty, class = void>
struct is_aggregate_with_base : STD false_type {
	static_assert(STD is_aggregate_v<_Ty>, "This trait works only with aggregate types");
};

template <class _Ty>
struct is_aggregate_with_base<_Ty, STD void_t<decltype(_Ty{ _Any_base<_Ty>{} })>> : STD true_type {
	static_assert(STD is_aggregate_v<_Ty>, "This trait works only with aggregate types");
};

template <class _Ty>
INLINE constexpr bool is_aggregate_with_base_v = is_aggregate_with_base<_Ty>::value;

template <class _Ty, class = void>
struct is_aggregate_with_reference : STD true_type {
	static_assert(STD is_aggregate_v<_Ty>, "This trait works only with aggregate types");
};

template <class _Ty>
struct is_aggregate_with_reference<_Ty, STD enable_if_t<!STD is_empty_v<_Ty>, STD void_t<decltype(_Ty{ 0 })>>> : STD false_type {
	static_assert(STD is_aggregate_v<_Ty>, "This trait works only with aggregate types");
};

template <class _Ty>
struct is_aggregate_with_reference<_Ty, STD enable_if_t<STD is_empty_v<_Ty>>> : STD false_type {
	static_assert(STD is_aggregate_v<_Ty>, "This trait works only with aggregate types");
};

template <class _Ty>
INLINE constexpr bool is_aggregate_with_reference_v = is_aggregate_with_reference<_Ty>::value;

// STRUCT TEMPLATE is_floating_point
template <class _Ty>
INLINE constexpr bool is_xmm_v = STD _Is_any_of_v<STD remove_cv_t<_Ty>, __m128, __m128i, __m128d>;

template <class _Ty>
struct is_xmm : STD bool_constant<is_xmm_v<_Ty>> {};


template <class _Ty, template <class...> class _Template>
INLINE constexpr bool is_specialization_v = STD _Is_specialization_v<_Ty, _Template>;


template <class>
INLINE constexpr bool is_lvalue_reference_v = false;

template <class _Ty>
INLINE constexpr bool is_lvalue_reference_v<_Ty&> = !STD is_const_v<_Ty>;

template <class _Ty>
struct is_lvalue_reference : STD bool_constant<is_lvalue_reference_v<_Ty>> {};


template <class>
INLINE constexpr bool is_const_lvalue_reference_v = false;

template <class _Ty>
INLINE constexpr bool is_const_lvalue_reference_v<const _Ty&> = true;

template <class _Ty>
struct is_const_lvalue_reference : STD bool_constant<is_const_lvalue_reference_v<_Ty>> {};


template <class _Functor, class = void>
INLINE constexpr bool is_trivial_functor_v = false;

template <class _Functor>
INLINE constexpr bool is_trivial_functor_v<_Functor, STD void_t<decltype(&_Functor::operator())>> = true;

template <class...>
struct type_list;

template <class, class = void>
struct _ConstTemplateFunctorTrait {
	inline static constexpr bool value = false;
};

template <class _ReturnTy, class _Functor, class... _Args>
struct _ConstTemplateFunctorTrait<STDEX type_list<_ReturnTy, _Functor, _Args...>, STD void_t<decltype(static_cast<_ReturnTy (_Functor::*)(_Args...) const>(&_Functor::operator()))>> {
	inline static constexpr bool value = true;
};

template <class _ReturnTy, class _Functor, class... _Args>
INLINE constexpr bool is_const_template_functor_v = _ConstTemplateFunctorTrait<STDEX type_list<_ReturnTy, _Functor, _Args...>>::value;

template <class _ReturnTy, class _Functor, class... _Args>
using const_template_operator_t = _ReturnTy (_Functor::*)(_Args...) const;

template <class _ReturnTy, class _Functor, class... _Args>
using template_operator_t = _ReturnTy (_Functor::*)(_Args...);

}