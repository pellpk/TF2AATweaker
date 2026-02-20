// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

void Soup_OnAttach();
void Soup_OnDetach();

static DWORD WINAPI AttachThread(LPVOID)
{
	Soup_OnAttach();
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// Prevent per-thread DllMain calls; we only need attach/detach.
		DisableThreadLibraryCalls(hModule);
		// When loaded via rpak HAS_MODULE, PLUGIN_INIT is never called by the
		// engine - only DllMain fires. Spawn a thread so we avoid holding the
		// loader lock during the full initialisation sequence.
		CreateThread(nullptr, 0, AttachThread, nullptr, 0, nullptr);
		break;

	case DLL_PROCESS_DETACH:
		// Safety-net for rpak unload path: engine calls FreeLibrary without
		// an explicit PLUGIN_UNLOAD. CleanupSoup() is idempotent (guarded by
		// bInitialised), so calling it here is safe even if PLUGIN_UNLOAD
		// was already called by the Northstar plugin loader.
		Soup_OnDetach();
		break;
	}
	return TRUE;
}
