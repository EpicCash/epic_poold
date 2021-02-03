#include "log.hpp"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

logger::logger() : sighup(false)
{
	const char* log_file = nullptr;
	if(log_file != nullptr && log_file[0] != '\0')
		hFile = fopen(log_file, "ab");
	else
		hFile = stdout;
}

void logger::log_msg(const char* fmt, ...)
{
	std::lock_guard<std::mutex> lck(log_lock);
	char buf[1024] = {0};
	size_t bpos;
	tm stime;

	check_sighup();

	time_t now = time(nullptr);
	localtime_r(&now, &stime);
	strftime(buf, sizeof(buf), "[%F %T]: ", &stime);
	bpos = strlen(buf);

	va_list args;
	va_start(args, fmt);
	vsnprintf(buf + bpos, sizeof(buf) - bpos, fmt, args);
	va_end(args);

	fputs(buf, hFile);
	fputc('\n', hFile);
	fflush(hFile);
}

void logger::print_timestamp()
{
	char buf[64];
	tm stime;
	time_t now = time(nullptr);
	localtime_r(&now, &stime);
	strftime(buf, sizeof(buf), "[%F %T]: ", &stime);
	fputs(buf, hFile);
}

void logger::log_msg_short(const char* msg)
{
	std::lock_guard<std::mutex> lck(log_lock);
	char buf[256] = {0};
	tm stime;

	check_sighup();

	time_t now = time(nullptr);
	localtime_r(&now, &stime);
	strftime(buf, sizeof(buf), "[%F %T]: ", &stime);

	fputs(buf, hFile);
	fputs(msg, hFile);
	fputc('\n', hFile);
	fflush(hFile);
}

void logger::log_msg_long(const char* long_msg, const char* fmt, ...)
{
	std::lock_guard<std::mutex> lck(log_lock);
	char buf[1024] = {0};
	size_t bpos;
	tm stime;

	check_sighup();

	time_t now = time(nullptr);
	localtime_r(&now, &stime);
	strftime(buf, sizeof(buf), "[%F %T]: ", &stime);
	bpos = strlen(buf);

	va_list args;
	va_start(args, fmt);
	vsnprintf(buf + bpos, sizeof(buf) - bpos, fmt, args);
	va_end(args);

	fputs(buf, hFile);
	fputs(long_msg, hFile);
	fputc('\n', hFile);
	fflush(hFile);
}

void logger::reopen_file()
{
	sighup = false;
	fputs("I caught SIGHUP but the output is going to console!\n", hFile);
	fflush(hFile);
}
