#pragma once

#include <types.hxx>

/*
// For set explicit the EngineConfig at init time use this code for example in main.cpp
namespace Focus::Config {
const Engine_t Engine = { .numberOfThreads = SystemDefault.numberOfThreads - 1, .taskQueue = { Default.taskQueue.large, {64, 16_KB} } };
}
*/

namespace Focus::Config {

struct TaskQueue {
	struct Entry {
		size_t taskCount = 0;
		size_t stackSize = 4_KB;
	};

	Entry large = { 16, 512_KB };
	Entry medium = { 32, 32_KB };
	Entry small = { 128, 4_KB };
};

struct Engine_t {
	uint32_t numberOfThreads;
	TaskQueue taskQueue;
};

extern const Engine_t Engine;

inline constexpr Engine_t Default = {};

struct System_t {
	uint32_t pageSize;
	uint32_t numberOfThreads;
};

extern const System_t SystemDefault;

}
