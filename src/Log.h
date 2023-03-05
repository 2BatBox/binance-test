#pragma once

#include <cstdio>
#include <sys/time.h>
#include <sys/types.h>

#include "Config.h"

#ifndef LOG_LOG
#define LOG_LOG(Symbol, Color, ...) { \
	printf("%s[" Symbol "] ", Config::ColorLog ? Color : "" ); \
	Log::time_short(stdout); \
    printf(__VA_ARGS__); \
    printf("%s\n", Config::ColorLog ? Log::NORMAL : "" ); \
    fflush(stdout); \
}
#endif

#ifndef LOG_INFO
#define LOG_INFO(...) LOG_LOG("I", Log::NORMAL, __VA_ARGS__);
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG(...) LOG_LOG("D", Log::BLUE, __VA_ARGS__);
#endif

#ifndef LOG_ERROR
#define LOG_ERROR(...) LOG_LOG("E", Log::YELLOW, __VA_ARGS__);
#endif

#ifndef LOG_CRITICAL
#define LOG_CRITICAL(...) { \
	printf("%s[C] ", Config::ColorLog ? Log::RED : ""); \
	Log::time_short(stdout); \
	printf("%s : %s : %d ", __FUNCTION__, __FILE__, __LINE__); \
    printf(__VA_ARGS__); \
    printf("%s\n", Config::ColorLog ? Log::NORMAL : "" ); \
    fflush(stdout); \
}
#endif

class Log {

public:

	static constexpr const char* BLACK = "\033[1;30m";
	static constexpr const char* RED = "\033[1;31m";
	static constexpr const char* GREEN = "\033[1;32m";
	static constexpr const char* YELLOW = "\033[1;33m";
	static constexpr const char* BLUE = "\033[1;34m";
	static constexpr const char* PURPLE = "\033[1;35m";
	static constexpr const char* CYAN = "\033[1;36m";
	static constexpr const char* WHITE = "\033[1;37m";
	static constexpr const char* NORMAL = "\033[0m";

	static void time_short(FILE* out) noexcept {
		timeval time_val;
		struct tm time;

		gettimeofday(&time_val, nullptr);
		localtime_r(&(time_val.tv_sec), &time);

		fprintf(out, "%04d-%02d-%02d %02d:%02d:%02d ", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
		        time.tm_hour, time.tm_min, time.tm_sec);
	}

};

