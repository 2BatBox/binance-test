#pragma once

#include <cstdio>
#include <sys/time.h>
#include <sys/types.h>

#include "Config.h"

#ifndef LOG_PREFIX

#define LOG_PREFIX(Symbol, Color) { \
    printf("%s[" Symbol "] ", Config::ColorLog ? Color : "" ); \
    Log::time_short(stdout); \
}

#define LOG_TRACE(...)  { printf("%s : function '%s()' : line %d \n", __FILE__, __FUNCTION__, __LINE__); }
#define LOG_PLAIN(...)  { printf(__VA_ARGS__); fflush(stdout); }

#define LOG_INFO(...)     LOG_PREFIX("I", Log::NORMAL); LOG_PLAIN(__VA_ARGS__);
#define LOG_DEBUG(...)    LOG_PREFIX("D", Log::BLUE);   LOG_PLAIN(__VA_ARGS__);
#define LOG_ERROR(...)    LOG_PREFIX("E", Log::YELLOW); LOG_PLAIN(__VA_ARGS__);
#define LOG_CRITICAL(...) LOG_PREFIX("C", Log::RED); LOG_TRACE();  LOG_PLAIN(__VA_ARGS__);

#define LOG_UD_FLOAT(Field, New, Old) { \
	const bool ls = (New.Field < Old.Field); \
	const bool gr = (New.Field > Old.Field); \
	if(ls || gr) { \
        if(ls) printf("%s", Log::YELLOW);\
        if(gr) printf("%s", Log::GREEN);\
	} else { printf("%s", Log::NORMAL); } \
	printf(" "#Field "=%f", New.Field); \
}

#endif // LOG_PREFIX

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

