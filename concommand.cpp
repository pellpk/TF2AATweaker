#include "pch.h"
#include "concommand.h"

#include <iostream>
#include "cvar.h"
#include "convar.h"

static std::vector<ConCommand*> s_registeredConCommands;

// Stamp the Soup DLL identifier into ConCommandBase at +0x14.
// This is the unk int32 sitting in pad_0011[7] — same slot as m_nDLLIdentifier
// in Source SDK, confirmed by the Portal2 ICvar vtable layout matching ours.
static inline void SetDLLId(ConCommandBase* pBase)
{
	*reinterpret_cast<int*>(reinterpret_cast<char*>(pBase) + 0x14) = g_soupDLLIdentifier;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this is a command
// Output : bool
//-----------------------------------------------------------------------------
bool ConCommand::IsCommand(void) const
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this is a command
// Output : bool
//-----------------------------------------------------------------------------
bool ConCommandBase::IsCommand(void) const
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Has this cvar been registered
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ConCommandBase::IsRegistered(void) const
{
	return m_bRegistered;
}

//-----------------------------------------------------------------------------
// Purpose: Test each ConCommand query before execution.
// Input  : *pCommandBase - nFlags
// Output : False if execution is permitted, true if not.
//-----------------------------------------------------------------------------
bool ConCommandBase::IsFlagSet(int nFlags) const
{
	return m_nFlags & nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if ConCommand has requested flags.
// Input  : nFlags -
// Output : True if ConCommand has nFlags.
//-----------------------------------------------------------------------------
bool ConCommandBase::HasFlags(int nFlags)
{
	return m_nFlags & nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Add's flags to ConCommand.
// Input  : nFlags -
//-----------------------------------------------------------------------------
void ConCommandBase::AddFlags(int nFlags)
{
	m_nFlags |= nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Removes flags from ConCommand.
// Input  : nFlags -
//-----------------------------------------------------------------------------
void ConCommandBase::RemoveFlags(int nFlags)
{
	m_nFlags &= ~nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Returns current flags.
// Output : int
//-----------------------------------------------------------------------------
int ConCommandBase::GetFlags(void) const
{
	return m_nFlags;
}

//-----------------------------------------------------------------------------
// Purpose:
// Output : const ConCommandBase
//-----------------------------------------------------------------------------
ConCommandBase* ConCommandBase::GetNext(void) const
{
	return m_pNext;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the ConCommandBase help text.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConCommandBase::GetHelpText(void) const
{
	return m_pszHelpString;
}

//-----------------------------------------------------------------------------
// Purpose: Copies string using local new/delete operators
// Input  : *szFrom -
// Output : char
//-----------------------------------------------------------------------------
char* ConCommandBase::CopyString(const char* szFrom) const
{
	size_t nLen;
	char* szTo;

	nLen = strlen(szFrom);
	if (nLen <= 0)
	{
		szTo = new char[1];
		szTo[0] = 0;
	}
	else
	{
		szTo = new char[nLen + 1];
		memmove(szTo, szFrom, nLen + 1);
	}
	return szTo;
}

typedef void (*ConCommandConstructorType)(
	ConCommand* newCommand, const char* name, FnCommandCallback_t callback, const char* helpString, int flags, void* parent);
ConCommandConstructorType ConCommandConstructor;

void RegisterConCommand(const char* name, FnCommandCallback_t callback, const char* helpString, int flags)
{
	spdlog::info("Registering SoupCommand \"{}\"", name);

	ConCommand* newCommand = new ConCommand;
	ConCommandConstructor(newCommand, name, callback, helpString, flags, nullptr);
	SetDLLId(newCommand);
	s_registeredConCommands.push_back(newCommand);
}

void RegisterConCommand(
	const char* name, FnCommandCallback_t callback, const char* helpString, int flags, FnCommandCompletionCallback completionCallback)
{
	spdlog::info("Registering SoupCommand \"{}\"", name);

	ConCommand* newCommand = new ConCommand;
	ConCommandConstructor(newCommand, name, callback, helpString, flags, nullptr);
	newCommand->m_pCompletionCallback = completionCallback;
	SetDLLId(newCommand);
	s_registeredConCommands.push_back(newCommand);
}

void CleanupConCommands()
{
	// Unregister by pointer — UnregisterConCommands(dllId) relies on GetDLLIdentifier()
	// which reads a per-DLL static we never set, so it silently skips our commands.
	// UnregisterConCommand walks the list and unlinks by pointer comparison, which always works.
	if (R2::g_pCVar)
	{
		for (ConCommand* p : s_registeredConCommands)
		{
			R2::g_pCVar->UnregisterConCommand(p);
			delete p;
		}
	}
	s_registeredConCommands.clear();
}

static void Soup_Test(const CCommand& args)
{
	spdlog::info("soup_test: {} arg(s)", args.ArgC() - 1);
	for (int i = 1; i < args.ArgC(); i++)
		spdlog::info("  [{}] {}", i, args[i]);
}

ON_DLL_LOAD("engine.dll", ConCommand, (CModule module))
{
	ConCommandConstructor = module.Offset(0x415F60).As<ConCommandConstructorType>();
}

ON_DLL_LOAD_RELIESON("engine.dll", SoupTestCmd, ConCommand, (CModule module))
{
	RegisterConCommand("soup_test", Soup_Test, "Soup test command", FCVAR_NONE);
}