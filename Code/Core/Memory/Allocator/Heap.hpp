#pragma once

#include <Platform.h>
#include <Platform/WinNTEx.h>

namespace Focus::Memory::Allocator {

template <class _Ty>
class Heap {
public:
	static_assert(!STD is_const_v<_Ty>, "The C++ Standard forbids containers of const elements "
		"because Heap<const T> is ill-formed.");

	static_assert(alignof(_Ty) <= platform::default_align, "The Heap allocator can't allocate memory for overaligned types.");

	using _From_primary = Heap;

	using value_type = _Ty;

	using size_type = size_t;
	using difference_type = ptrdiff_t;

	using propagate_on_container_move_assignment = STD true_type;
	using is_always_equal = STD true_type;

	constexpr Heap() noexcept {}

	constexpr Heap(const Heap&) noexcept = default;
	template <class _Other>
	constexpr Heap(const Heap<_Other>&) noexcept {}

	void deallocate(_Ty* const ptr, [[maybe_unused]] const size_t count) noexcept {
		if constexpr (config::debug)
			::operator delete (ptr, sizeof(_Ty) * count, STD align_val_t{ platform::default_align });
		else
			::HeapFree(GetProcessHeap_(), 0Ul, ptr);
	}

	[[nodiscard]] __declspec(allocator) _Ty* allocate(_CRT_GUARDOVERFLOW const size_t count) noexcept {
		if constexpr (config::debug)
			return static_cast<_Ty*>(::operator new(sizeof(_Ty) * count, STD align_val_t{ platform::default_align }, STD nothrow));
		else
			return static_cast<_Ty*>(::HeapAlloc(GetProcessHeap_(), 0Ul, sizeof(_Ty) * count));
	}
};

template <class _Ty, class _Other>
[[nodiscard]] bool operator==(const Heap<_Ty>&, const Heap<_Other>&) noexcept {
	return true;
}

template <class _Ty, class _Other>
[[nodiscard]] bool operator!=(const Heap<_Ty>&, const Heap<_Other>&) noexcept {
	return false;
}

}