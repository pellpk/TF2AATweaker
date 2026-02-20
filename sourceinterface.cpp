#include "pch.h"
#include "sourceinterface.h"

AUTOHOOK_INIT()

// clang-format off
ON_DLL_LOAD("client.dll", ClientInterface, (CModule module)) { AUTOHOOK_DISPATCH_MODULE(client.dll) }
ON_DLL_LOAD("server.dll", ServerInterface, (CModule module)) { AUTOHOOK_DISPATCH_MODULE(server.dll) }
ON_DLL_LOAD("engine.dll", EngineInterface, (CModule module)) { AUTOHOOK_DISPATCH_MODULE(engine.dll) }
// clang-format on