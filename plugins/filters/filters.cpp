#include "BasicFilterSystem.h"
#include "imodule.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(filters::BasicFilterSystemPtr(new filters::BasicFilterSystem));
}
