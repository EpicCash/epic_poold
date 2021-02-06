#include "log.hpp"
#include "jconf.hpp"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

logger::logger() : sighup(false)
{
	const char* log_file = jconf::inst().get_log_file();
	if(log_file != nullptr && log_file[0] != '\0')
		hFile = fopen(log_file, "ab");
	else
		hFile = stdout;
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

void logger::reopen_file()
{
	sighup = false;
	if(jconf::inst().daemonize())
	{
		fclose(hFile);
		hFile = fopen(jconf::inst().get_log_file(), "ab");
	}
	else
	{
		fputs("I caught SIGHUP but the output is going to console!\n", hFile);
		fflush(hFile);
	}
}

void logger::set_colour(out_colours cl)
{
	if(hFile != stdout)
		return;

	switch(cl)
	{
	case out_colours::K_RED:
		fputs("\x1B[1;31m", stdout);
		break;
	case out_colours::K_GREEN:
		fputs("\x1B[1;32m", stdout);
		break;
	case out_colours::K_BLUE:
		fputs("\x1B[1;34m", stdout);
		break;
	case out_colours::K_YELLOW:
		fputs("\x1B[1;33m", stdout);
		break;
	case out_colours::K_CYAN:
		fputs("\x1B[1;36m", stdout);
		break;
	case out_colours::K_MAGENTA:
		fputs("\x1B[1;35m", stdout);
		break;
	case out_colours::K_WHITE:
		fputs("\x1B[1;37m", stdout);
		break;
	case out_colours::K_NONE:
		fputs("\x1B[0m", stdout);
		break;
	default:
		break;
	}
}
