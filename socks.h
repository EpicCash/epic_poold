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

#pragma once

/* Assume that any non-Windows platform uses POSIX-style sockets instead. */
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h> /* Needed for getaddrinfo() and freeaddrinfo() */
#include <string.h>
#include <sys/socket.h>
#include <unistd.h> /* Needed for close() */
#include <signal.h>

inline bool sock_init()
{
	static bool init = false;
	if(!init)
	{
		struct sigaction sa = {{0}};
		sa.sa_handler = SIG_IGN;
		sa.sa_flags = 0;
		if(sigaction(SIGPIPE, &sa, 0) != 0)
			return false;
		init = true;
	}
	return true;
}

typedef int SOCKET;

#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

inline void sock_close(SOCKET s)
{
	close(s);
}

/* Dafault linger behaviour, we will send any queued data in the background */
inline void soft_shutdown(SOCKET s)
{
	shutdown(s, SHUT_RDWR);
}

/* To be used when something is wrong on the protocol level. It will generate TCP RST */
inline void sock_abort(SOCKET s)
{
	struct linger so_linger;
	so_linger.l_onoff = 1;
	so_linger.l_linger = 0;
	setsockopt(s, SOL_SOCKET, SO_LINGER, &so_linger, sizeof so_linger);
	close(s);
}

inline const char* sock_strerror(char* buf, size_t len)
{
	buf[0] = '\0';
	return strerror_r(errno, buf, len);
}

inline const char* sock_gai_strerror(int err, char* buf, size_t len)
{
	buf[0] = '\0';
	return gai_strerror(err);
}
