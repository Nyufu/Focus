#pragma once

#include <assert.hxx>
#include <algorithm.hxx>
#include <enum_helper.hxx>
#include <Macros.h>
#include <Platform.h>
#include <Core/EngineConfig.hpp>
#include <Core/Concurrency/TaskParameters.hpp>
#include <Core/Memory/Allocator/Heap.hpp>
#include "Fiber.hpp"

namespace Focus::Concurrency::_impl {

namespace Scheduler {
class FiberBased;
}

WARNING_SCOPE_BEGIN
MSVC_WARNING_DISABLE(4324) // structure was padded due to alignment specifier

class alignas(platform::cacheline_size) FiberQueue {
	ONLY_CONSTRUCT(FiberQueue);

	friend class Scheduler::FiberBased;

public:
	using DataType = Fiber*;

protected:
	using DataTypePtr = DataType*;
	using FreeSetDataType = Fiber*;

	static_assert(platform::register_size % sizeof(FreeSetDataType) == 0);

protected:
	struct FreeSetDescriptor {
		const unsigned __int64 size;
		FreeSetDataType* const storage = nullptr;

		FreeSetDescriptor(unsigned __int64 size, void* const memory)
			: size{ size * sizeof(FreeSetDataType) }
			, storage{ reinterpret_cast<FreeSetDataType*>(STDEX align(reinterpret_cast<uintptr_t>(memory), platform::cacheline_size, STDEX align_way::up)) } {
		}

		FreeSetDescriptor(unsigned __int64 size, const FreeSetDescriptor& desc)
			: size{ size * sizeof(FreeSetDataType) }
			, storage{ reinterpret_cast<FreeSetDataType*>(STDEX align(reinterpret_cast<uintptr_t>(desc.storage) + desc.size, platform::cacheline_size, STDEX align_way::up)) } {
		}
	};

	struct QueueDescriptor {
		const unsigned __int64 capacityMask;
		union {
			const unsigned __int64 redBlackBit;
			const unsigned __int64 maxSize;
		};
		Fiber** const storage = nullptr;


		QueueDescriptor(unsigned __int64 size, const FreeSetDescriptor& desc)
			: capacityMask{ size - 1 }
			, maxSize{ size }
			, storage{ reinterpret_cast<DataTypePtr>(STDEX align(reinterpret_cast<uintptr_t>(desc.storage) + desc.size, platform::cacheline_size, STDEX align_way::up)) } {
		}

		QueueDescriptor(const QueueDescriptor& desc)
			: capacityMask{ desc.capacityMask }
			, maxSize{ desc.maxSize }
			, storage{ reinterpret_cast<DataTypePtr>(
				  STDEX align(reinterpret_cast<uintptr_t>(desc.storage) + (desc.maxSize * platform::register_size), platform::cacheline_size, STDEX align_way::up)) } {
		}
	};

	struct alignas(platform::cacheline_size) AlignedValue {
		unsigned __int64 value;
	};

public:
	FiberQueue(const Config::TaskQueue& initDesc);
	~FiberQueue();

	void Enqueue(Priority priority, DataType value);
	STD pair<DataType, bool> Dequeue(Priority priority);

	void Release(DataType value);
	DataType Acquire(StackSize stackSize);

protected:
	void EmplaceFreeFiber(void* address, register_t stackLimit_, StackSize stackSize_);

protected:
	void* const memory;

	FreeSetDescriptor freeDesc[3];
	QueueDescriptor queueDesc[3];

	AlignedValue head[3];
	AlignedValue tail[3];
};

WARNING_SCOPE_END

// clang-format off

inline FiberQueue::FiberQueue(const Config::TaskQueue& config)
	: memory{ Memory::Allocator::Heap<uint8_t>{}.allocate(
		platform::cacheline_size - platform::default_align // The default_align is the default align by NT allocator on x64.
		+ STDEX align(config.large.taskCount  * sizeof(FreeSetDataType), platform::cacheline_size, STDEX align_way::up)
		+ STDEX align(config.medium.taskCount * sizeof(FreeSetDataType), platform::cacheline_size, STDEX align_way::up)
		+ STDEX align(config.small.taskCount  * sizeof(FreeSetDataType), platform::cacheline_size, STDEX align_way::up)
		+ STD bit_ceil(STD max(config.large.taskCount + config.medium.taskCount + config.small.taskCount, platform::registers_in_cacheline * platform::registers_in_cacheline)) * 3 * platform::register_size) }
	, freeDesc{ { config.large.taskCount, memory },
		{ config.medium.taskCount, freeDesc[0] },
		{ config.small.taskCount, freeDesc[1] } } 
	, queueDesc{ { STD bit_ceil(STD max(config.large.taskCount + config.medium.taskCount + config.small.taskCount, platform::registers_in_cacheline * platform::registers_in_cacheline)), freeDesc[2] },
		{ queueDesc[0] },
		{ queueDesc[1] } } {
	STD memset(freeDesc[0].storage, 0, freeDesc[0].size);
	STD memset(freeDesc[1].storage, 0, freeDesc[1].size);
	STD memset(freeDesc[2].storage, 0, freeDesc[2].size);
}

// clang-format on

inline FiberQueue::~FiberQueue() {
	Memory::Allocator::Heap<uint8_t>{}.deallocate(reinterpret_cast<uint8_t*>(memory), 1);
}

inline void FiberQueue::Enqueue([[maybe_unused]] Priority priority, [[maybe_unused]] DataType value) {
}
inline STD pair<FiberQueue::DataType, bool> FiberQueue::Dequeue([[maybe_unused]] Priority priority) {
	return { nullptr, false };
}

inline void FiberQueue::Release(DataType value) {
	ASSERT(!value->freeSetPlace);
	*value->freeSetPlace = value;
}
inline FiberQueue::DataType FiberQueue::Acquire([[maybe_unused]] StackSize stackSize) {
	return nullptr;
}

inline void FiberQueue::EmplaceFreeFiber(void* address, register_t stackLimit, StackSize stackSize) {
	auto& desc = freeDesc[STDEX to_num(stackSize)];
	for (Fiber **it = desc.storage, **end = desc.storage + desc.size; it != end; it++) {
		if (*it == nullptr) {
			*it = ::new (address) Fiber(stackLimit, stackSize, it);
			return;
		}
	}
}


}