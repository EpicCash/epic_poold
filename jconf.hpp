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
#include <inttypes.h>
#include <stddef.h>
#include "loglevels.hpp"

class jconf
{
public:
	static jconf& inst()
	{
		static jconf inst;
		return inst;
	};

	bool parse_config(const char* filename);

	const char* get_pid_file();
	const char* get_log_file();
	log_level get_log_level();

	const char* get_db_hostname();
	const char* get_db_port();
	const char* get_db_name();
	const char* get_db_username();
	const char* get_db_password();

	const char* get_node_hostname();
	const char* get_node_port();
	const char* get_node_username();
	const char* get_node_password();

	bool daemonize();

	uint16_t get_server_port();
	uint16_t get_server_tls_port();

	const char* get_tls_cert_filename();
	const char* get_tls_cipher_list();

	size_t get_fatal_node_timeout();
	size_t get_template_timeout();

private:
	jconf();
	class jconfPrivate& d;
};
