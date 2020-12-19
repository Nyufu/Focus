#pragma once

#include <Platform.h>

namespace stdex {

enum class align_way
{
	up,
	down
};

template <class _Ty, STD enable_if_t<STD is_pointer_v<_Ty>, void*> = nullptr>
constexpr _Ty align(const _Ty value, const uintptr_t align = platform::default_align, const align_way alignWay = align_way::down) noexcept {
	return (reinterpret_cast<_Ty>((reinterpret_cast<uintptr_t>(value) + (alignWay == align_way::up ? (align - 1) : 0)) & ~(align - 1)));
}

template <class _Ty, STD enable_if_t<!STD is_pointer_v<_Ty>, void*> = nullptr>
constexpr _Ty align(const _Ty value, const STD make_unsigned_t<_Ty> align = platform::default_align, const align_way alignWay = align_way::up) noexcept {
	return ((value + (alignWay == align_way::up ? (align - 1) : 0)) & ~(align - 1));
}

}
