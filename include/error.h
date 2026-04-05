#pragma once

#include "debug.h"
#include <iostream>
#include <string>
#include <cassert>

namespace Log {
	enum class Level {
		Trace = 0,
		Info = 1,
		Warning = 2,
		Error = 3,
		Critical = 4
	};

	inline constexpr const char* errorLvlToString(Log::Level lvl) {
		switch (lvl) {
			case Log::Level::Trace:    return "TRACE";
			case Log::Level::Info:     return "INFO";
			case Log::Level::Warning:  return "WARN";
			case Log::Level::Error:    return "ERR";
			case Log::Level::Critical: return "CRITICAL";
			default:				   return "UNKNOWN";
		}
	}

	inline void out(Level lvl, const std::string& msg) {
		#if !DEBUG_MODE
		if (lvl < Level::Warning) return;
		#endif

		const char* color = "\033[0m";
		switch (lvl) {
			case Level::Trace:		color = "\033[90m"; break;	 // Gray
			case Level::Info:		color = "\033[36m"; break;   // Cyan
			case Level::Warning:	color = "\033[33m"; break;   // Yellow
			case Level::Error:		color = "\033[31m"; break;	 // Red
			case Level::Critical:   color = "\033[1;31m"; break; // Bold red
		}

		std::cout << color << "[" << errorLvlToString(lvl) << "] "  << "\033[0m" << msg << std::endl;
	}
}

#if DEBUG_MODE
	#define COMPILER_ASSERT(condition, msg) \
		do { \
			if (!(condition)) { \
				Log::out(Log::Level::Critical, "ASSERTION FAILED: " + std::string(msg)); \
				assert(condition); \
			} \
		} while (0)
#else
	#define COMPILER_ASSERT(condition, msg) // Disappears in release
#endif