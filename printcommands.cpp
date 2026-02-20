#include "pch.h"
#include "cvar.h"
#include "convar.h"
AUTOHOOK_INIT();

typedef void (*PrintCommandHelpDialogue_Type)(const ConCommandBase* command, const char* name);
PrintCommandHelpDialogue_Type PrintCommandHelpDialogue_Func;
AUTOHOOK(PrintCommandHelpDialogue, Northstar.dll@"48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 D9 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 17 4C 8B E9", void, (const ConCommandBase* command, const char* name))
{
	if (!command || !command->IsFlagSet(FCVAR_CROUTON))
		return PrintCommandHelpDialogue(command, name);
	if (!command)
	{
		spdlog::info("unknown command {}", name);
		return;
	}

	// temp because command->IsCommand does not currently work
	ConVar* cvar = R2::g_pCVar->FindVar(command->m_pszName);

	// build string for flags if not FCVAR_NONE
	std::string flagString;
	if (command->GetFlags() != FCVAR_NONE)
	{
		flagString = "( ";

		for (auto& flagPair : g_PrintCommandFlags)
		{
			if (command->GetFlags() & flagPair.first)
			{
				// special case, slightly hacky: PRINTABLEONLY is for commands, GAMEDLL_FOR_REMOTE_CLIENTS is for concommands, both have the
				// same value
				if (flagPair.first == FCVAR_PRINTABLEONLY)
				{
					if (cvar && !strcmp(flagPair.second, "GAMEDLL_FOR_REMOTE_CLIENTS"))
						continue;

					if (!cvar && !strcmp(flagPair.second, "PRINTABLEONLY"))
						continue;
				}

				flagString += flagPair.second;
				flagString += " ";
			}
		}

		flagString += ") ";
	}

	if (cvar)
		spdlog::info("\"{}\" = \"{}\" {} - {}", cvar->GetBaseName(), cvar->GetString(), flagString, cvar->GetHelpText());
	else
		spdlog::info("\"{}\" {} - {}", command->m_pszName, flagString, command->GetHelpText());
}

ON_DLL_LOAD("engine.dll", ConvarHandler, (CModule module))
{

	AUTOHOOK_DISPATCH_MODULE(Northstar.dll)

}