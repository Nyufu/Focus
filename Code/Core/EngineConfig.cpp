#include "EngineConfig.hpp"
#include <Macros.h>

MSVC_WARNING_DISABLE(4075)	   // supress an init_seg warning which works in this case
#pragma init_seg(".CRT$XCAAA") // First after Startup C++ Initializer

namespace Focus::Config {

System_t SystemDefault_v;
Engine_t Engine_v = []() -> Engine_t {
	SYSTEM_INFO system_info_ SUPPRESS_INITIALIZE({});
	GetSystemInfo(&system_info_);

	SystemDefault_v = { system_info_.dwPageSize, system_info_.dwNumberOfProcessors };

	return {
		.numberOfThreads = system_info_.dwNumberOfProcessors,
	};
}();

#pragma comment(linker, "/alternatename:?SystemDefault@Config@Focus@@3USystem_t@12@B=?SystemDefault_v@Config@Focus@@3USystem_t@12@A")
#pragma comment(linker, "/alternatename:?Engine@Config@Focus@@3UEngine_t@12@B=?Engine_v@Config@Focus@@3UEngine_t@12@A")

}
