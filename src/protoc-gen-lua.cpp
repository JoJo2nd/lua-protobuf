#include <string>
#include <stdlib.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/stubs/strutil.h>
#include "lua_code_generator.h"

int main(int argc, char* argv[]) {
    static bool wait_for_debugger = true;
    while (wait_for_debugger) {}

#ifdef _MSC_VER
	// Don't print a silly message or stick a modal dialog box in my face,
	// please.
	_set_abort_behavior(0, ~0);
#endif  // !_MSC_VER

	google::protobuf::compiler::LuaCodeGenerator generator;
	return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}