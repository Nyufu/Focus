#pragma once

#include <Macros.h>
#include "assert.hxx"

namespace stdex {

template <class _Ty, class _Alloc = STD allocator<_Ty>>
class managed_object final {
	static_assert(_Alloc::is_always_equal::value, "The managed_object doesn't store the allocator, therefore the allocator can be only stateless!");
	static_assert(!STD is_array_v<_Ty>, "The managed_object doesn't support array as template parameter!");

private:
	class object {
	protected:
		constexpr object() = default;
		~object() = default;
		ONLY_CONSTRUCT(object);
	};

public:
	using type = _Ty;
	using alloc = _Alloc;
	using pointer = type*;
	using object_pointer = object*;

	struct ref_count {
		constexpr explicit operator long() const noexcept {
			return (value);
		}

		long value;
	};

private:
	template <long _InitialRefCount>
	class object_imp : object {
		using alloc = typename STD allocator_traits<_Alloc>::template rebind_alloc<object_imp>;

		friend managed_object;

		template <class, class = void>
		struct wrapper {
			template <class... _Args>
			constexpr wrapper(_Args&&... args) noexcept(STD is_nothrow_constructible_v<type, _Args...>)
				: data(STD forward<_Args>(args)...) {
			}

			type data;
		};

		template <class T>
		struct wrapper<T, STD enable_if_t<STD is_aggregate_v<T>>> {
			template <class... _Args>
			constexpr wrapper(_Args&&... args) noexcept
				: data{ STD forward<_Args>(args)... } {
			}

			type data;
		};

		ONLY_CONSTRUCT(object_imp);
		~object_imp() = delete;

		template <class... _Args>
		constexpr object_imp(ref_count initialRefCount, _Args&&... args) noexcept(STD is_nothrow_constructible_v<type, _Args...>)
			: uses{ initialRefCount }
			, wrap(STD forward<_Args>(args)...) {
		}

		template <class... _Args>
		constexpr object_imp(_Args&&... args) noexcept(STD is_nothrow_constructible_v<type, _Args...>)
			: wrap(STD forward<_Args>(args)...) {
		}

		void delete_this() {
#if !defined(NDEBUG)
			ASSERT(debug_uses == 0l);
#endif

			wrap.data.~_Ty();
			alloc{}.deallocate(this, 1);
		}

		void inc_ref() noexcept {
			_InterlockedIncrement(reinterpret_cast<volatile long*>(&uses));

#if !defined(NDEBUG)
			_InterlockedIncrement(reinterpret_cast<volatile long*>(&debug_uses));
#endif
		}

		void dec_ref() {
#if !defined(NDEBUG)
			_InterlockedDecrement(reinterpret_cast<volatile long*>(&debug_uses));
#endif
			ASSERT(uses != 3722304989l); // 0xdddddddd, The memory is freed.
			ASSERT(uses > 0l);

			if (_InterlockedDecrement(reinterpret_cast<volatile long*>(&uses)) == 0l)
				delete_this();
		}

		long uses = _InitialRefCount;
#if !defined(NDEBUG)
		long debug_uses = 0l;
#endif
		wrapper<type> wrap;
	};

	using object_imp_type = object_imp<0l>;
	using object_imp_ptr = object_imp_type*;

	inline static constexpr object_imp_ptr invalid_ptr = TO_CONSTANT(object_imp_ptr, 0xdddddddddddddddd);

public:
	template <long _InitialRefCount = 1l, class... _Args>
	[[nodiscard]] static object_pointer construct(_Args&&... args) {
		static_assert(_InitialRefCount > 0l, "Invalid Initial Reference Count!");

		const auto ptr = typename object_imp<_InitialRefCount>::alloc{}.allocate(1);
		return ::new (const_cast<void*>(static_cast<const volatile void*>(ptr))) object_imp<_InitialRefCount>(STD forward<_Args>(args)...);
	}

	template <class... _Args>
	[[nodiscard]] static object_pointer construct(ref_count initialRefCount, _Args&&... args) {
		ASSERT(initialRefCount.value > 0l);

		const auto ptr = typename object_imp<0l>::alloc{}.allocate(1);
		return ::new (const_cast<void*>(static_cast<const volatile void*>(ptr))) object_imp<0l>(initialRefCount, STD forward<_Args>(args)...);
	}

	constexpr managed_object() noexcept = default;

	constexpr managed_object(STD nullptr_t) noexcept {
	}

	managed_object(object_pointer ptr_) noexcept
		: ptr{ static_cast<object_imp_ptr>(ptr_) } {
#if !defined(NDEBUG)
		if (ptr) {
			_InterlockedIncrement(reinterpret_cast<volatile long*>(&ptr->debug_uses));

			ASSERT(ptr->uses >= ptr->debug_uses); // Overused the object_pointers.
		}
#endif
	}

	template <class _Arg0, class... _Args, class = STD enable_if_t<!STD is_same_v<STD decay_t<_Arg0>, managed_object>>>
	explicit managed_object(_Arg0&& arg0, _Args&&... args)
		: managed_object(construct(STD forward<_Arg0>(arg0), STD forward<_Args>(args)...)) {
	}

	template <class... _Args>
	explicit managed_object(ref_count initialRefCount, _Args&&... args)
		: managed_object(construct(initialRefCount, STD forward<_Args>(args)...)) {
	}

	managed_object(const managed_object& other) noexcept
		: ptr{ other.ptr } {
		add_ref();
	}

	constexpr managed_object(managed_object&& right) noexcept
		: ptr{ right.ptr } {
		right.ptr = nullptr;
	}

	managed_object& operator=(const managed_object& other) {
		if (ptr != other.ptr) {
			remove_ref();
			ptr = other.ptr;
			add_ref();
		}

		return (*this);
	}

	constexpr managed_object& operator=(managed_object&& right) noexcept {
		STD swap(ptr, right.ptr);
		return (*this);
	};

	~managed_object() noexcept(false) {
		remove_ref();

		if constexpr (config::debug)
			ptr = invalid_ptr;
	}

	constexpr pointer operator->() noexcept {
		return (STD addressof(ptr->wrap.data));
	}

	constexpr const pointer operator->() const noexcept {
		return (STD addressof(ptr->wrap.data));
	}

	constexpr STD add_lvalue_reference_t<type> operator*() noexcept {
		return (ptr->wrap.data);
	}

	constexpr STD add_lvalue_reference_t<const type> operator*() const noexcept {
		return (ptr->wrap.data);
	}

	constexpr explicit operator bool() const noexcept {
		return (!!ptr); // (ptr != nullptr);
	}

	[[nodiscard]] constexpr object_pointer get_ptr() const noexcept {
		return (ptr);
	}

private:
	void add_ref() noexcept {
		if (ptr)
			ptr->inc_ref();
	}

	void remove_ref() {
		if (ptr)
			ptr->dec_ref();
	}

	object_imp_ptr ptr = nullptr;
};

}
