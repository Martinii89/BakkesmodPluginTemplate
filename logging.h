// ReSharper disable CppNonExplicitConvertingConstructor
#pragma once
#include <string>
#include <source_location>
#include <format>
#include <memory>

#include "bakkesmod/wrappers/cvarmanagerwrapper.h"

extern std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
constexpr bool DEBUG_LOG = false;


struct FormatString
{
	std::string_view str;
	std::source_location loc{};

	FormatString(const char* str, const std::source_location& loc = std::source_location::current()) : str(str), loc(loc)
	{
	}

	FormatString(const std::string&& str, const std::source_location& loc = std::source_location::current()) : str(str), loc(loc)
	{
	}

	[[nodiscard]] std::string GetLocation() const
	{
		return std::format("[{} ({}:{})]", loc.function_name(), loc.file_name(), loc.line());
	}
};

struct FormatWstring
{
	std::wstring_view str;
	std::source_location loc{};

	FormatWstring(const wchar_t* str, const std::source_location& loc = std::source_location::current()) : str(str), loc(loc)
	{
	}

	FormatWstring(const std::wstring&& str, const std::source_location& loc = std::source_location::current()) : str(str), loc(loc)
	{
	}

	[[nodiscard]] std::wstring GetLocation() const
	{
		auto basic_string = std::format("[{} ({}:{})]", loc.function_name(), loc.file_name(), loc.line());
		return std::wstring(basic_string.begin(), basic_string.end());
	}
};


template <typename... Args>
void LOG(std::string_view format_str, Args&&... args)
{
	_globalCvarManager->log(std::vformat(format_str, std::make_format_args(std::forward<Args>(args)...)));
}

template <typename... Args>
void LOG(std::wstring_view format_str, Args&&... args)
{
	_globalCvarManager->log(std::vformat(format_str, std::make_wformat_args(std::forward<Args>(args)...)));
}


template <typename... Args>
void DEBUGLOG(const FormatString& format_str, Args&&... args)
{
	if constexpr (DEBUG_LOG)
	{
		auto text = std::vformat(format_str.str, std::make_format_args(std::forward<Args>(args)...));
		auto location = format_str.GetLocation();
		_globalCvarManager->log(std::format("{} {}", text, location));
	}
}

template <typename... Args>
void DEBUGLOG(const FormatWstring& format_str, Args&&... args)
{
	if constexpr (DEBUG_LOG)
	{
		auto text = std::vformat(format_str.str, std::make_wformat_args(std::forward<Args>(args)...));
		auto location = format_str.GetLocation();
		_globalCvarManager->log(std::format(L"{} {}", text, location));
	}
}
