#include "pch.h"

#include "logging.h"
#include "sourceconsole.h"
#include "cvar.h"
#include "concommand.h"
#include "convar.h"

static bool bInitialised = false;

static void InitialiseSoup()
{
	if (bInitialised)
		return;

	bInitialised = true;

	curl_global_init_mem(CURL_GLOBAL_DEFAULT, _malloc_base, _free_base, _realloc_base, _strdup_base, _calloc_base);

	InitialiseLogging();
	InitialiseConsole();

	InstallInitialHooks();
	CallAllPendingDLLLoadCallbacks();
}

static void CleanupSoup()
{
	if (!bInitialised)
		return;

	bInitialised = false;

	// Unregister all ConCommands and ConVars from CCvar (needs g_pCVar alive),
	// then delete our allocated objects.
	CleanupConCommands();
	CleanupConVars();

	CleanupConsole();
	CleanupLogging();
	CleanupHooks();
	curl_global_cleanup();

	// Free game-heap allocations made during ON_DLL_LOAD handlers.
	delete R2::g_pCVarInterface;
	R2::g_pCVarInterface = nullptr;
	R2::g_pCVar = nullptr;
}

// Called from DllMain thread when loaded via rpak HAS_MODULE
void Soup_OnAttach()
{
	InitialiseSoup();
}

// Called from DllMain on DLL_PROCESS_DETACH (safety net for rpak unload)
void Soup_OnDetach()
{
	CleanupSoup();
}

extern "C" __declspec(dllexport) void PLUGIN_INIT(void* funcs, void* data)
{
	InitialiseSoup();
}

extern "C" __declspec(dllexport) void PLUGIN_UNLOAD()
{
	CleanupSoup();
}
