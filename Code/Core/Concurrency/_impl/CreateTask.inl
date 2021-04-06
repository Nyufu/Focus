#pragma once

#include "TaskParameterSeparator.hpp"
#include "Scheduler/Interface.hpp"
#include "Fiber.hpp"
#include <abi/x64.hxx>
#include <Core/Concurrency/Task.hpp>
#include <assert.hxx>
#include <algorithm.hxx>
#include <types.hxx>
#include <type_traits.hxx>
#include <utility.hxx>


namespace Focus::Concurrency {
namespace _impl {

struct TaskCommon {
	register_t refCount = 0;
};

template <class Ty>
struct Task;

template <>
struct Task<void> : TaskCommon, Fiber {
	inline static constexpr size_t size = STDEX sizeof_pack<TaskCommon, Fiber>;
};

template <class Ty>
struct ResultStorage {
	char result[STDEX sizeof_pack<Ty, Task<void>> - sizeof(Task<void>)];
};

template <class Ty>
struct alignas(STD max(alignof(Ty), alignof(Task<void>))) Task : ResultStorage<Ty>, Task<void> {
	inline static constexpr size_t size = STDEX sizeof_pack<ResultStorage<Ty>, Task<void>>;
};

struct ReturnTypeMarker {};

namespace CDecl {

template <
	class FTy, class ITy,
	bool = !STD disjunction_v<
		abi::x64::is_passed_by_register<FTy>, STDEX is_lvalue_reference<FTy>,
		STD conjunction<
			STDEX is_const_lvalue_reference<FTy>, STD is_lvalue_reference<ITy>,
			STD disjunction<STD is_same<STD remove_cvref_t<FTy>, STD remove_cvref_t<ITy>>, STD is_base_of<STD remove_cvref_t<FTy>, STD remove_cvref_t<ITy>>>>>,
	bool = STD conjunction_v<STD disjunction<STDEX is_const_lvalue_reference<FTy>, STD is_rvalue_reference<FTy>>, STD is_base_of<STD decay_t<FTy>, STD decay_t<ITy>>>>
struct CallingConventionTraits;

template <class FTy, class ITy>
struct CallingConventionTraits<FTy, ITy, false, false> {
	inline static constexpr bool StorageRequired = false;
};

template <class FTy, class ITy>
struct CallingConventionTraits<FTy, ITy, false, true> : CallingConventionTraits<FTy, ITy, false, false> {};

template <class FTy, class ITy>
struct CallingConventionTraits<FTy, ITy, true, false> {
	inline static constexpr bool StorageRequired = true;
	inline static constexpr bool CleanupRequired = STD is_reference_v<FTy> && !STD is_trivially_destructible_v<STD remove_reference_t<FTy>>;
	using Target = STD decay_t<FTy>;
};

template <class FTy, class ITy>
struct CallingConventionTraits<FTy, ITy, true, true> {
	inline static constexpr bool StorageRequired = true;
	inline static constexpr bool CleanupRequired = STD is_reference_v<FTy> && !STD is_trivially_destructible_v<STD remove_reference_t<FTy>>;
	using Target = STD decay_t<ITy>;
};

template <class ReturnTy>
struct CallingConventionTraits<ReturnTy, ReturnTypeMarker, true, true> {
	inline static constexpr bool StorageRequired = true;
	inline static constexpr bool CleanupRequired = !STD is_trivially_destructible_v<STD remove_reference_t<ReturnTy>>;
	using Target = ReturnTy;
};

template <class ReturnTy>
struct CallingConventionTraits<ReturnTy, ReturnTypeMarker, false, false> : CallingConventionTraits<ReturnTy, ReturnTypeMarker, true, true> {};

template <class ReturnTy>
struct CallingConventionTraits<ReturnTy, ReturnTypeMarker, false, true> : CallingConventionTraits<ReturnTy, ReturnTypeMarker, true, true> {};

template <class ReturnTy>
struct CallingConventionTraits<ReturnTy, ReturnTypeMarker, true, false> : CallingConventionTraits<ReturnTy, ReturnTypeMarker, true, true> {};

template <class, class, class = void>
struct ArgumentsStorage {
	constexpr void Cleanup() noexcept {
	}
};

template <class FTy, class ITy>
struct ArgumentsStorage<STDEX type_list<FTy>, STDEX type_list<ITy>, STD enable_if_t<CallingConventionTraits<FTy, ITy>::StorageRequired>>
	: STDEX storage<typename CallingConventionTraits<FTy, ITy>::Target> {
	constexpr void Cleanup() {
		using Target = typename CallingConventionTraits<FTy, ITy>::Target;
		if constexpr (CallingConventionTraits<FTy, ITy>::CleanupRequired)
			reinterpret_cast<Target*>(this)->~Target();
	}
};

WARNING_SCOPE_BEGIN
MSVC_WARNING_DISABLE(4584) // 'class1' : base-class 'class2' is already a base-class of 'class3'
CLANG_WARNING_DISABLE("-Winaccessible-base")

template <class FTy, class ITy, class... FRest, class... IRest>
struct ArgumentsStorage<STDEX type_list<FTy, FRest...>, STDEX type_list<ITy, IRest...>, STD enable_if_t<CallingConventionTraits<FTy, ITy>::StorageRequired>>
	: STDEX storage<typename CallingConventionTraits<FTy, ITy>::Target>, ArgumentsStorage<STDEX type_list<FRest...>, STDEX type_list<IRest...>> {
	using Next = ArgumentsStorage<STDEX type_list<FRest...>, STDEX type_list<IRest...>>;

	constexpr void Cleanup() {
		using Target = typename CallingConventionTraits<FTy, ITy>::Target;
		if constexpr (CallingConventionTraits<FTy, ITy>::CleanupRequired)
			reinterpret_cast<Target*>(this)->~Target();
		if constexpr (sizeof...(IRest) > 0)
			Next::Cleanup();
	}
};

template <class FTy, class ITy, class... FRest, class... IRest>
struct ArgumentsStorage<STDEX type_list<FTy, FRest...>, STDEX type_list<ITy, IRest...>, STD enable_if_t<!CallingConventionTraits<FTy, ITy>::StorageRequired>>
	: ArgumentsStorage<STDEX type_list<FRest...>, STDEX type_list<IRest...>> {
	using Next = ArgumentsStorage<STDEX type_list<FRest...>, STDEX type_list<IRest...>>;

	constexpr void Cleanup() {
		if constexpr (sizeof...(IRest) > 0)
			Next::Cleanup();
	}
};

MSVC_WARNING_DISABLE(4324) // structure was padded due to alignment specifier

template <size_t Size>
struct alignas(platform::default_align) Homespace : STD array<register_t, STD max(Size, 4ull)> {}; // The 4 is RCX, RDX, R8, R9

WARNING_SCOPE_END

template <class, class>
struct TaskCaller;

template <class... FTypes, class... ITypes>
struct TaskCaller<STDEX type_list<FTypes...>, STDEX type_list<ITypes...>> : Homespace<sizeof...(FTypes)>, ArgumentsStorage<STDEX type_list<FTypes...>, STDEX type_list<ITypes...>> {};

template <class, class, size_t = 0>
struct TaskCallerInitializer;

template <size_t Index>
struct TaskCallerInitializer<STDEX type_list<>, STDEX type_list<>, Index> {
	template <class FrameTy>
	static constexpr void Initialize(FrameTy* const) noexcept {
	}
};

template <class FTy, class ITy, class... FRest, class... IRest, size_t Index>
struct TaskCallerInitializer<STDEX type_list<FTy, FRest...>, STDEX type_list<ITy, IRest...>, Index> {
	using Next = TaskCallerInitializer<STDEX type_list<FRest...>, STDEX type_list<IRest...>, Index + 1>;
	using ArgumentsStorage = ArgumentsStorage<STDEX type_list<FTy, FRest...>, STDEX type_list<ITy, IRest...>>;
	using CallingConventionTraits = CallingConventionTraits<FTy, ITy>;
	using Ref = STD reference_wrapper<STD remove_reference_t<FTy>>;

	template <class FrameTy>
	static constexpr void Initialize(FrameTy* const frame, ITy&& input0, IRest&&... input) {
		void* const destArgAddress = STD addressof(frame->at(Index));

		if constexpr (CallingConventionTraits::StorageRequired) {
			void* const storage = static_cast<ArgumentsStorage*>(frame);

			if constexpr (STD is_same_v<ReturnTypeMarker, ITy>) {
				::new (destArgAddress) ArgumentsStorage* { storage };
			} else {
				if constexpr (STD is_aggregate_v<ITy>)
					::new (destArgAddress) Ref(*::new (storage) typename CallingConventionTraits::Target{ STD forward<ITy>(input0) });
				else
					::new (destArgAddress) Ref(*::new (storage) typename CallingConventionTraits::Target(STD forward<ITy>(input0)));
			}
		} else {
			if constexpr (STD is_lvalue_reference_v<FTy>) {
				::new (destArgAddress) Ref(STD forward<ITy>(input0));
			} else {
				if constexpr (STD is_aggregate_v<FTy>)
					::new (destArgAddress) FTy{ STD forward<ITy>(input0) };
				else
					::new (destArgAddress) FTy(STD forward<ITy>(input0));
			}
		}

		if constexpr (sizeof...(IRest) > 0)
			Next::Initialize(frame, STD forward<IRest>(input)...);
	}
};

template <class ReturnTy, class FTy, class ITy>
INLINE constexpr size_t PaddingSize = (platform::cacheline_size - (Task<ReturnTy>::size % platform::cacheline_size)) +
									  ((STD is_empty_v<ArgumentsStorage<FTy, ITy>> || STDEX is_aligned_v<sizeof(ArgumentsStorage<FTy, ITy>), platform::default_align>)&&(
										   platform::cacheline_size - Task<ReturnTy>::size % platform::cacheline_size < platform::default_align)
										   ? platform::default_align
										   : 0);

template <class ReturnTy, class FTy, class ITy, class = void>
struct TaskCallerDomain : TaskCaller<FTy, ITy>, STDEX padding<PaddingSize<ReturnTy, FTy, ITy>>, Task<ReturnTy> {
	using TaskCaller = TaskCaller<FTy, ITy>;
	void Finish() {
		static_cast<ArgumentsStorage<FTy, ITy>*>(this)->Cleanup();
	}

	constexpr register_t GetLastInstructionPointer() const noexcept {
		auto stackBottom = reinterpret_cast<register_t>(static_cast<const STDEX padding<PaddingSize<ReturnTy, FTy, ITy>>*>(this));
		GSL_ASSUME(stackBottom);

		if constexpr (!STD is_empty_v<ArgumentsStorage<FTy, ITy>>) {
			if constexpr (STDEX is_aligned_v<sizeof(ArgumentsStorage<FTy, ITy>), platform::default_align>) {
				return stackBottom + platform::default_align;
			} else {
				return stackBottom - platform::register_size;
			}
		} else {
			return stackBottom + platform::register_size;
		}
	}
};

template <class ReturnTy, class FTy, class ITy>
struct TaskCallerDomain<ReturnTy, FTy, ITy, STD enable_if_t<abi::x64::is_return_by_register_v<ReturnTy> && !STD is_void_v<ReturnTy>>>
	: TaskCaller<FTy, ITy>, STDEX padding<PaddingSize<ReturnTy, FTy, ITy>>, Task<ReturnTy> {
	using TaskCaller = TaskCaller<FTy, ITy>;
	void Finish(ReturnTy returnValue) {
		STD memcpy(this->result, &returnValue, sizeof(ReturnTy));
		static_cast<ArgumentsStorage<FTy, ITy>*>(this)->Cleanup();
	}

	constexpr register_t GetLastInstructionPointer() const noexcept {
		auto stackBottom = reinterpret_cast<register_t>(static_cast<const STDEX padding<PaddingSize<ReturnTy, FTy, ITy>>*>(this));
		GSL_ASSUME(stackBottom);

		if constexpr (!STD is_empty_v<ArgumentsStorage<FTy, ITy>>) {
			if constexpr (STDEX is_aligned_v<sizeof(ArgumentsStorage<FTy, ITy>), platform::default_align>) {
				return stackBottom + platform::default_align;
			} else {
				return stackBottom - platform::register_size;
			}
		} else {
			return stackBottom + platform::register_size;
		}
	}
};

}

template <long long offset, class StackTy, class Ty>
void SetOnStack(StackTy stackTop, Ty&& value) {
	void* place = reinterpret_cast<register_t*>(stackTop) + offset;
	GSL_ASSUME(place);

	using RawTy = STD remove_reference_t<Ty>;

	if constexpr (STD is_aggregate_v<RawTy>)
		::new (place) RawTy{ STD forward<Ty>(value) };
	else
		::new (place) RawTy(STD forward<RawTy>(value));
}

template <class...>
struct TasksCallStack;

template <class ReturnTy, class... FunctionArgs, class... InputArgs>
struct TasksCallStack<ReturnTy __cdecl(FunctionArgs...), InputArgs...> {
	static_assert(sizeof...(FunctionArgs) == sizeof...(InputArgs));

	template <class FunctionPtrTy>
	static constexpr void Create(Fiber* const fiber, FunctionPtrTy functionPtr, InputArgs&&... input) {
		using namespace CDecl;

		static_assert(sizeof(FunctionPtrTy) == platform::register_size);

		using ReturnRef = STD add_lvalue_reference_t<ReturnTy>;

		using TaskCallerDomain = STD conditional_t<
			abi::x64::is_return_by_register_v<ReturnTy>, TaskCallerDomain<ReturnTy, STDEX type_list<FunctionArgs...>, STDEX type_list<InputArgs...>>,
			TaskCallerDomain<ReturnTy, STDEX type_list<ReturnRef, FunctionArgs...>, STDEX type_list<ReturnRef, InputArgs...>>>;

		using TaskCaller = typename TaskCallerDomain::TaskCaller;

		auto* taskCallerDomain = static_cast<TaskCallerDomain*>(fiber);
		GSL_ASSUME(taskCallerDomain);
		ASSERT((reinterpret_cast<uintptr_t>(taskCallerDomain) & 0x0F) == 0); // stack must be aligned on a 16-byte boundary

		auto lastInstructionPtrAddress = taskCallerDomain->GetLastInstructionPointer();

		SetOnStack<-4>(taskCallerDomain, &TaskCallerDomain::Finish);
		SetOnStack<-3>(taskCallerDomain, functionPtr);
		SetOnStack<-2>(taskCallerDomain, lastInstructionPtrAddress);
		SetOnStack<-1>(taskCallerDomain, &FiberInitializer);

		fiber->stackPointer = reinterpret_cast<register_t>(taskCallerDomain) - platform::register_size;

		auto* taskCaller = static_cast<TaskCaller*>(taskCallerDomain);
		GSL_ASSUME(taskCaller);

		auto* task = static_cast<Task<ReturnTy>*>(taskCallerDomain);
		GSL_ASSUME(task);

		if constexpr (abi::x64::is_return_by_register_v<ReturnTy>) {
			TaskCallerInitializer<STDEX type_list<FunctionArgs...>, STDEX type_list<InputArgs...>>::Initialize(taskCaller, STD forward<InputArgs>(input)...);
		} else {
			auto& result = *reinterpret_cast<ReturnTy*>(task->result);

			TaskCallerInitializer<STDEX type_list<ReturnRef, FunctionArgs...>, STDEX type_list<ReturnRef, InputArgs...>>::Initialize(taskCaller, result, STD forward<InputArgs>(input)...);
		}

		::new (reinterpret_cast<void*>(lastInstructionPtrAddress)) register_t{ 0 };

		auto* taskCommon = static_cast<TaskCommon*>(taskCallerDomain);
		GSL_ASSUME(taskCommon);
		::new (taskCommon) TaskCommon{ 1 };
	}
};


template <class ReturnTy, class... FunctionArgs, class... InputArgs>
struct TasksCallStack<ReturnTy __cdecl(FunctionArgs...) noexcept, InputArgs...> : TasksCallStack<ReturnTy __cdecl(FunctionArgs...), InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {
	template <class FunctionPtrTy>
	static constexpr void Create(Fiber* const fiber, FunctionPtrTy functionPtr, InputArg0&& input0, InputArgs&&... input) {
		using namespace CDecl;

		static_assert(sizeof(FunctionPtrTy) == platform::register_size);

		using ReturnRef = STD add_lvalue_reference_t<ReturnTy>;

		using TaskCallerDomain = STD conditional_t<
			abi::x64::is_return_by_register_v<ReturnTy>, TaskCallerDomain<ReturnTy, STDEX type_list<InputArg0, FunctionArgs...>, STDEX type_list<InputArg0, InputArgs...>>,
			TaskCallerDomain<ReturnTy, STDEX type_list<InputArg0, ReturnRef, FunctionArgs...>, STDEX type_list<InputArg0, ReturnRef, InputArgs...>>>;

		using TaskCaller = typename TaskCallerDomain::TaskCaller;

		auto* taskCallerDomain = static_cast<TaskCallerDomain*>(fiber);
		GSL_ASSUME(taskCallerDomain);
		ASSERT((reinterpret_cast<uintptr_t>(taskCallerDomain) & 0x0F) == 0); // stack must be aligned on a 16-byte boundary

		auto lastInstructionPtrAddress = taskCallerDomain->GetLastInstructionPointer();

		SetOnStack<-4>(taskCallerDomain, &TaskCallerDomain::Finish);
		SetOnStack<-3>(taskCallerDomain, functionPtr);
		SetOnStack<-2>(taskCallerDomain, lastInstructionPtrAddress);
		SetOnStack<-1>(taskCallerDomain, &FiberInitializer);

		fiber->stackPointer = reinterpret_cast<register_t>(taskCallerDomain) - platform::register_size;

		auto* taskCaller = static_cast<TaskCaller*>(taskCallerDomain);
		GSL_ASSUME(taskCaller);

		auto* task = static_cast<Task<ReturnTy>*>(taskCallerDomain);
		GSL_ASSUME(task);

		if constexpr (abi::x64::is_return_by_register_v<ReturnTy>) {
			TaskCallerInitializer<STDEX type_list<InputArg0, FunctionArgs...>, STDEX type_list<InputArg0, InputArgs...>>::Initialize(
				taskCaller, STD forward<InputArg0>(input0), STD forward<InputArgs>(input)...);
		} else {
			auto& result = *reinterpret_cast<ReturnTy*>(task->result);

			TaskCallerInitializer<STDEX type_list<InputArg0, ReturnRef, FunctionArgs...>, STDEX type_list<InputArg0, ReturnRef, InputArgs...>>::Initialize(
				taskCaller, STD forward<InputArg0>(input0), result, STD forward<InputArgs>(input)...);
		}

		::new (reinterpret_cast<void*>(lastInstructionPtrAddress)) register_t{ 0 };

		auto* taskCommon = static_cast<TaskCommon*>(taskCallerDomain);
		GSL_ASSUME(taskCommon);
		::new (taskCommon) TaskCommon{ 1 };
	}
};

// clang-format off

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...) noexcept, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...) const, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...) const noexcept, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...)&, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...)& noexcept, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...) const&, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...) const& noexcept, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...)&&, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...)&& noexcept, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...) const&&, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...) const&& noexcept, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__cdecl ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class... FunctionArgs, class... InputArgs>
struct TasksCallStack<ReturnTy __vectorcall(FunctionArgs...), InputArgs...> {
	static_assert(sizeof...(FunctionArgs) == sizeof...(InputArgs));

	template <class FunctionPtrTy>
	static constexpr void Create(Fiber* const, FunctionPtrTy, InputArgs&&...) {
		static_assert(STDEX always_false_v<FunctionPtrTy>, "The __vectorcall calling convention hasn't supported yet!");
	}
};

template <class ReturnTy, class... FunctionArgs, class... InputArgs>
struct TasksCallStack<ReturnTy __vectorcall(FunctionArgs...) noexcept, InputArgs...>
    : TasksCallStack<ReturnTy __vectorcall(FunctionArgs...), InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {
	template <class FunctionPtrTy>
	static constexpr void Create(Fiber* const, FunctionPtrTy, InputArg0&&, InputArgs&&...) {
		static_assert(STDEX always_false_v<FunctionPtrTy>, "The __vectorcall calling convention hasn't supported yet!");
	}
};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...) noexcept, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...) const, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...) const noexcept, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...)&, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...)& noexcept, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...) const&, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...) const& noexcept, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...)&&, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...)&& noexcept, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...) const&&, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

template <class ReturnTy, class ObjectTy, class... FunctionArgs, class InputArg0, class... InputArgs>
struct TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...) const&& noexcept, InputArg0, InputArgs...>
    : TasksCallStack<ReturnTy (__vectorcall ObjectTy::*)(FunctionArgs...), InputArg0, InputArgs...> {};

// clang-format on

template <class Callable, class... Args>
struct TaskTraits : TaskTraits<Callable, TaskParameterSeparator<Args...>> {};

template <class Callable, class... Args>
struct TaskTraits<Callable, STDEX type_list<Args...>> {
	static_assert(STD is_invocable_v<Callable, Args...>, "Cannot call the CreateTask's callable argument with the rest arguments of the CreateTask");

	using ReturnTy = STD invoke_result_t<Callable, Args...>;

	static_assert(abi::x64::is_definable_return_policy_v<ReturnTy>, "");

	static constexpr Focus::Concurrency::Task<ReturnTy> CreateTask(
		Callable&& callable, Args&&... args, const StackSize stackSize = Default::StackSize, [[maybe_unused]] const Priority priority = Default::Priority) {
		auto* const fiber = Scheduler::GetEmptyFiber(stackSize);
		ASSERT(fiber);

		if constexpr (STDEX is_specialization_v<STD remove_cvref_t<Callable>, STD reference_wrapper>) {
			using RawCallable = STD remove_cvref_t<typename STD remove_cvref_t<Callable>::type>;

			if constexpr (STD is_member_function_pointer_v<RawCallable>) {
				TasksCallStack<RawCallable, Args...>::Create(fiber, callable.get(), STD forward<Args>(args)...);
			} else if constexpr (STD is_function_v<STD remove_pointer_t<RawCallable>>) {
				TasksCallStack<STD remove_pointer_t<RawCallable>, Args...>::Create(fiber, callable.get(), STD forward<Args>(args)...);
			}
		} else {
			using RawCallable = STD remove_cvref_t<Callable>;

			if constexpr (STD is_member_function_pointer_v<RawCallable>) {
				TasksCallStack<RawCallable, Args...>::Create(fiber, callable, STD forward<Args>(args)...);
			} else if constexpr (STD is_function_v<STD remove_pointer_t<RawCallable>>) {
				TasksCallStack<STD remove_pointer_t<RawCallable>, Args...>::Create(fiber, callable, STD forward<Args>(args)...);
			} else if constexpr (STDEX is_trivial_functor_v<RawCallable>) {
				auto callOperatorPtr = &RawCallable::operator();

				TasksCallStack<decltype(callOperatorPtr), Callable, Args...>::Create(fiber, callOperatorPtr, STD forward<Callable>(callable), STD forward<Args>(args)...);
			} else if constexpr (STDEX is_const_template_functor_v<ReturnTy, RawCallable, Args...>) {
				STDEX const_template_operator_t<ReturnTy, RawCallable, Args...> callOperatorPtr = &RawCallable::operator();

				TasksCallStack<decltype(callOperatorPtr), Callable, Args...>::Create(fiber, callOperatorPtr, STD forward<Callable>(callable), STD forward<Args>(args)...);
			} else {
				STDEX template_operator_t<ReturnTy, RawCallable, Args...> callOperatorPtr = &RawCallable::operator();

				TasksCallStack<decltype(callOperatorPtr), Callable, Args...>::Create(fiber, callOperatorPtr, STD forward<Callable>(callable), STD forward<Args>(args)...);
			}
		}

		Scheduler::ScheduleFiber(fiber, priority);
		return {};
	}

	static constexpr auto CreateTask(Callable&& callable, Args&&... args, const Priority priority, const StackSize stackSize = Default::StackSize) {
		return CreateTask(STD forward<Callable>(callable), STD forward<Args>(args)..., stackSize, priority);
	}
};

}

template <class Callable, class... Args>
[[nodiscard]] auto CreateTask(Callable&& callable, Args&&... args) {
	return _impl::TaskTraits<Callable, Args...>::CreateTask(STD forward<Callable>(callable), STD forward<Args>(args)...);
}

template <class Callable, class... Args>
void Async([[maybe_unused]] Callable&& callable, [[maybe_unused]] Args&&... args) {
}

}
