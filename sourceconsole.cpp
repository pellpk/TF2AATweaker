#include "pch.h"
#include "convar.h"
#include "sourceconsole.h"
#include "sourceinterface.h"
#include "concommand.h"

SourceInterface<CGameConsole>* g_pSourceGameConsole;

void SourceConsoleSink::custom_sink_it_(const custom_log_msg& msg)
{
	if (!(*g_pSourceGameConsole)->m_bInitialized)
		return;

	spdlog::memory_buf_t formatted;
	spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);

	// get message string
	std::string str = fmt::to_string(formatted);

	SourceColor levelColor = m_LogColours[msg.level];
	std::string name{ msg.logger_name.begin(), msg.logger_name.end() };

	(*g_pSourceGameConsole)->m_pConsole->m_pConsolePanel->ColorPrint(msg.origin->SRCColor, ("[" + name + "]").c_str());
	(*g_pSourceGameConsole)->m_pConsole->m_pConsolePanel->Print(" ");
	(*g_pSourceGameConsole)->m_pConsole->m_pConsolePanel->ColorPrint(levelColor, ("[" + std::string(level_names[msg.level]) + "]").c_str());
	(*g_pSourceGameConsole)->m_pConsole->m_pConsolePanel->Print(" ");
	(*g_pSourceGameConsole)->m_pConsole->m_pConsolePanel->Print(fmt::to_string(formatted).c_str());
}

void SourceConsoleSink::sink_it_(const spdlog::details::log_msg& msg)
{
	throw std::runtime_error("sink_it_ called on SourceConsoleSink with pure log_msg. This is an error!");
}

void SourceConsoleSink::flush_() {}

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", SourceConsole, ConCommand, (CModule module))
{
	g_pSourceGameConsole = new SourceInterface<CGameConsole>("client.dll", "GameConsole004");

	auto consoleSink = std::make_shared<SourceConsoleSink>();
	consoleSink->set_pattern("%v");
	RegisterCustomSink(consoleSink);
}

void CleanupConsole()
{
	delete g_pSourceGameConsole;
	g_pSourceGameConsole = nullptr;
}