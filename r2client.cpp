#include "pch.h"
#include "r2client.h"

using namespace R2;

// use the R2 namespace for game funcs
namespace R2
{
	GetBaseLocalClientType GetBaseLocalClient;
} // namespace R2

ON_DLL_LOAD("engine.dll", R2EngineClient, (CModule module))
{

	GetBaseLocalClient = module.Offset(0x78200).As<GetBaseLocalClientType>();
}