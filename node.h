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
#include <list>
#include "json.h"
#include "socks.h"
#include "workstruct.hpp"

class node
{
public:
	inline static node& inst()
	{
		static node inst;
		return inst;
	}

	inline bool has_first_job()
	{
		return current_job != nullptr;
	}

	jobdata get_current_job()
	{
		return *current_job;
	}

	void start()
	{
		recv_thd = std::thread(&node::thread_main, this);
	}

	void shutdown()
	{
		run_loop = false;
		if(sock_fd != -1)
			soft_shutdown(sock_fd);
		recv_thd.join();
	}

	void send_job_result(const jobdata& data, uint64_t nonce, const v32& powhash)
	{
		char buffer[1024];
		const uint8_t* ph = powhash.data;
		int len = snprintf(buffer, sizeof(buffer), R"({"id":"0","jsonrpc":"2.0","method":"submit","params":)"
			R"({"height":%u,"job_id":%u,"nonce":%lu,"pow":{"RandomX":)"
			R"([%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u]}}})""\n",
			data.height, data.jobid, nonce, 
			ph[ 0], ph[ 1], ph[ 2], ph[ 3], ph[ 4], ph[ 5], ph[ 6], ph[ 7], ph[ 8], ph[ 9], ph[10], ph[11], ph[12], ph[13], ph[14], ph[15],
			ph[16], ph[17], ph[18], ph[19], ph[20], ph[21], ph[22], ph[23], ph[24], ph[25], ph[26], ph[27], ph[28], ph[29], ph[30], ph[31]);
		send(sock_fd, buffer, len, 0);
	}

private:
	node();

	bool node_connect(const char* addr, const char* port);
	ssize_t json_proc_msg(char* msg, size_t msglen);

	void thread_main();
	void recv_main();
	bool send_template_request();

	constexpr static size_t data_buffer_len = 16 * 1024;
	constexpr static size_t json_buffer_len = 8 * 1024;

	char send_buffer[data_buffer_len];
	char recv_buffer[data_buffer_len];
	char json_parse_buf[json_buffer_len];
	char json_dom_buf[json_buffer_len];

	MemoryPoolAllocator<> domAlloc;
	MemoryPoolAllocator<> parseAlloc;
	MemDocument jsonDoc;

	std::thread recv_thd;
	std::atomic<bool> run_loop;

	SOCKET sock_fd;

	std::list<jobdata> jobs;
	std::atomic<const jobdata*> current_job;

	uint64_t last_job_ts;
	bool logged_in;
};

