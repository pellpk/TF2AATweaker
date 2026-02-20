#include "pch.h"
#include "logging.h"
#include "convar.h"
#include "concommand.h"
#include "tier0.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <iomanip>
#include <sstream>

AUTOHOOK_INIT()

std::shared_ptr<ColoredLogger> NS::log::NORTHSTAR;

std::vector<std::shared_ptr<ColoredLogger>> loggers{};

void ExternalConsoleSink::sink_it_(const spdlog::details::log_msg& msg)
{
	throw std::runtime_error("sink_it_ called on SourceConsoleSink with pure log_msg. This is an error!");
}

void ExternalConsoleSink::custom_sink_it_(const custom_log_msg& msg)
{
	spdlog::memory_buf_t formatted;
	spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);

	std::string out = "";
	// if ansi colour is turned off, just use WriteConsoleA and return
	if (!g_bSpdLog_UseAnsiColor)
	{
		out += fmt::to_string(formatted);
	}

	// print to the console with colours
	else
	{
		// get message string
		std::string str = fmt::to_string(formatted);

		std::string levelColor = m_LogColours[msg.level];
		std::string name{ msg.logger_name.begin(), msg.logger_name.end() };

		std::string name_str = "[NAME]";
		int name_pos = str.find(name_str);
		str.replace(name_pos, name_str.length(), msg.origin->ANSIColor + "[" + name + "]" + default_color);

		std::string level_str = "[LVL]";
		int level_pos = str.find(level_str);
		str.replace(level_pos, level_str.length(), levelColor + "[" + std::string(level_names[msg.level]) + "]" + default_color);

		out += str;
	}
	// print the string to the console - this is definitely bad i think
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	auto ignored = WriteConsoleA(handle, out.c_str(), std::strlen(out.c_str()), nullptr, nullptr);
	(void)ignored;
}

void ExternalConsoleSink::flush_()
{
	std::cout << std::flush;
}

void CustomSink::custom_log(const custom_log_msg& msg)
{
	std::lock_guard<std::mutex> lock(mutex_);
	custom_sink_it_(msg);
}

void InitialiseConsole()
{
	// this if statement is adapted from r5sdk
	if (!strstr(GetCommandLineA(), "-noansiclr"))
	{
		g_bSpdLog_UseAnsiColor = true;
		DWORD dwMode = NULL;
		HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);

		GetConsoleMode(hOutput, &dwMode);
		dwMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

		if (!SetConsoleMode(hOutput, dwMode)) // Some editions of Windows have 'VirtualTerminalLevel' disabled by default.
		{
			// If 'VirtualTerminalLevel' can't be set, just disable ANSI color, since it wouldnt work anyway.
			spdlog::warn("could not set VirtualTerminalLevel. Disabling color output");
			g_bSpdLog_UseAnsiColor = false;
		}
	}
}

void RegisterCustomSink(std::shared_ptr<CustomSink> sink)
{
	for (auto& logger : loggers)
	{
		logger->custom_sinks_.push_back(sink);
	}
};

void InitialiseLogging()
{
	// create a logger, and set it to default
	NS::log::NORTHSTAR = std::make_shared<ColoredLogger>("SOUP", NS::Colors::NORTHSTAR, true);
	NS::log::NORTHSTAR->sinks().clear();
	loggers.push_back(NS::log::NORTHSTAR);
	spdlog::set_default_logger(NS::log::NORTHSTAR);

	// create our console sink
	auto sink = std::make_shared<ExternalConsoleSink>();
	// set the pattern
	if (g_bSpdLog_UseAnsiColor)
		// dont put the log level in the pattern if we are using colours, as the colour will show the log level
		sink->set_pattern("[%H:%M:%S] [NAME] [LVL] %v");
	else
		sink->set_pattern("[%H:%M:%S] [%n] [%l] %v");

	// add our sink to the logger
	NS::log::NORTHSTAR->custom_sinks_.push_back(sink);
}

void CleanupLogging()
{
	// Drop all registered loggers so sinks referencing our memory are released
	// before the DLL is unloaded.
	spdlog::drop_all();
	loggers.clear();
	NS::log::NORTHSTAR = nullptr;
}