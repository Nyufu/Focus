#include <Core/EngineConfig.hpp>
#include <Core/Concurrency/Task.hpp>
#include <thread>

namespace Focus::Config {
const Engine_t Engine = { .numberOfThreads = 2, .taskQueue = { .small = { 2, 4_KB } } };
}

using namespace Focus::Concurrency;

int foo([[maybe_unused]] int i) {
	__debugbreak();

	return 42;
}

int foo1([[maybe_unused]] const std::vector<int>& i) {
	__debugbreak();

	return 42;
}

int foo2([[maybe_unused]] void* ptr) {
	__debugbreak();

	return 42;
}

int main() noexcept {
	// auto task = CreateTask(foo, 3, Priority::Low, StackSize::Small);
	auto task = CreateTask(foo1, std::vector<int>(4), Priority::Low, StackSize::Small);
	// auto task = CreateTask(foo2, &foo, Priority::Low, StackSize::Small);

	Sleep(INFINITE);
	// std::this_thread::sleep_for(60s);
	return 0;
}
