// Copyright (c) 2014-2023, Epic Cash and fireice-uk
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
