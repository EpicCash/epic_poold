/*
 * Copyright (c) 2019, <copyright holder> <email>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY <copyright holder> <email> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> <email> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <thread>
#include <unordered_map>
#include <future>
#include "json.h"

class node
{
public:
	inline static node& inst()
	{
		static node inst;
		return inst;
	}

	inline void set_login_data(const char* host, const char* port, const char* username, const char* password, const char* agent)
	{
		this->host = host;
		this->port = port;
		login = username;
		passw = password;
		this->agent = agent;
	}

	inline void do_getblocktemplate()
	{
		response rsp;
		make_node_call("getjobtemplate", "\"algorithm\":\"randomx\"", rsp);
		//make_node_call("getblocktemplate", "", rsp);
	}

	void shutdown()
	{
		node_disconnect();
		recv_thd.join();
	}

private:
	node();

	bool node_connect(const char* addr, const char* port);
	void recv_main();
	void node_disconnect();
	void persistent_connect();

	struct response
	{
		bool error;
	};

	void make_node_call(const char* call, const char* data, response& rsp);

	constexpr static size_t data_buffer_len = 16 * 1024;
	constexpr static size_t json_buffer_len = 8 * 1024;

	char send_buffer[data_buffer_len];
	char recv_buffer[data_buffer_len];
	char json_parse_buf[json_buffer_len];
	char json_dom_buf[json_buffer_len];

	MemoryPoolAllocator<> domAlloc;
	MemoryPoolAllocator<> parseAlloc;
	MemDocument jsonDoc;

	ssize_t json_proc_msg(char* msg, size_t msglen);

	std::string host;
	std::string port;
	std::string login;
	std::string passw;
	std::string agent;

	void node_main();
	std::thread node_thd;
	std::thread recv_thd;

	std::mutex chk_mtx;

	std::mutex call_mtx;
	std::unordered_map<size_t, std::promise<response>> call_map;
	std::atomic<size_t> g_call_id;

	int sock_fd;
	bool is_connected() { return sock_fd != -1; }
};

