#pragma once
#include <stddef.h>
#include <stdio.h>
#include "socks.h"
#include <stdexcept>

#include "jconf.hpp"
#include "json.h"
#include "workstruct.hpp"
#include "node.h"
#include "check_job.hpp"

struct sock_buffer
{
	constexpr static size_t sock_buf_size = 4096;
	size_t len = 0;
	char buf[sock_buf_size];

	inline char* pos()
	{
		return buf+len;
	}
	
	inline size_t len_rem()
	{
		return sock_buf_size-len;
	}
};

struct client_ident
{
	uint8_t miner = 0;
	int16_t rid = -1;
	int32_t uid = -1;
};

class client
{
public:
	client(SOCKET fd, const in6_addr& ip_addr, in_port_t port);

	~client()
	{
		if(aborting)
			sock_abort(fd);
		else
			sock_close(fd);
	}

	client(const client& r) = delete;
	client& operator=(const client& r) = delete;
	client(client&& r) = delete;
	client& operator=(client&& r) = delete;

	static void set_limits()
	{
		max_calls_per_min = 60;// jconf::inst().get_max_calls_per_minute();
		/*bad_share_ban_cnt = jconf::inst().get_bad_share_ban_cnt();
		srv_start_diff = jconf::inst().get_starting_diff();
		srv_const_diff = jconf::inst().get_server_const_diff();
		start_window = jconf::inst().get_starting_window();
		target_time = jconf::inst().get_target_time();
		template_timeout = jconf::inst().get_template_timeout();
		client_timeouts[0] = jconf::inst().get_client_timeout0();
		client_timeouts[1] = jconf::inst().get_client_timeout1();
		client_timeouts[2] = jconf::inst().get_client_timeout2();*/
	}

	inline SOCKET get_fd() const { return fd; }

	inline void hard_abort() { ::soft_shutdown(fd); aborting = true; }
	inline void soft_shutdown() { ::soft_shutdown(fd); }
	
	/*
	 * Return values of net_recv are as follows
	 * > 0 	call succeeded, we have data
	 * 0	call failed, socket should be closed
	 * -1	call failed, socket hasn't got data
	 */
	inline int net_recv()
	{
		if(aborting)
			return -1;

		int ret = read(fd, recv_buf.pos(), recv_buf.len_rem());
		if(ret == -1)
		{
			if(errno != EAGAIN && errno != EWOULDBLOCK)
				return 0; // Exit due to socket abort
			else
				return -1; // Exit due to lack of data, socket stays open
		}

		recv_buf.len += ret;
		return ret;
	}
	
	/*
	 * We don't operate on large amounts of data, so if socket
	 * is not writeable, it should be considered as a fatal error
	 */
	inline void net_send()
	{
		if(size_t(write(fd, send_buf.buf, send_buf.len)) != send_buf.len)
			hard_abort();
		send_buf.len = 0;
	}

	bool on_socket_read();
	bool on_new_block(int64_t timestamp_ms);

protected:
	constexpr static uint32_t min_diff = 256;
	constexpr static size_t hashrate_store_size = 1024;

	static size_t max_calls_per_min;
	static size_t bad_share_ban_cnt;
	static uint32_t srv_start_diff;
	static uint32_t srv_const_diff;
	static size_t target_time;
	static size_t start_window;
	static size_t template_timeout;
	static size_t client_timeouts[3];

	inline uint32_t diff_to_target(uint32_t diff) { return 0xFFFFFFFFUL / diff; }

	inline uint64_t work_to_diff(uint64_t work)
	{
		if(work == 0)
			return 0xFFFFFFFFFFFFFFFFULL;
		else
			return 0xFFFFFFFFFFFFFFFFULL / work;
	}

	SOCKET fd;
	bool aborting = false;
	char ip_addr_str[128];
	sock_buffer recv_buf;
	sock_buffer send_buf;

	int64_t flood_timestamp = 0;
	uint32_t flood_count = 0;
	bool logged_in = false;
	int64_t active_time;
	bool has_results = false;
	client_ident my_id;

	void get_new_job()
	{
		cur_job = node::inst().get_current_job();
		jobid++;
	}

	jobdata cur_job;
	uint32_t jobid = 0;

	uint32_t extra_nonce;

	static constexpr size_t json_buf_size = 4096;
	uint8_t json_dom_buf[json_buf_size];
	uint8_t json_parse_buf[json_buf_size];
	MemoryPoolAllocator<> domAlloc;
	MemoryPoolAllocator<> parseAlloc;
	MemDocument jsonDoc;

	inline void clear_json_bufs()
	{
		jsonDoc.SetNull();
		domAlloc.Clear();
		parseAlloc.Clear();
		jsonDoc.SetNull();
	}

	bool process_line(char* buf, int len);

	typedef void (client::*method_call)(int64_t call_id, const Value& args);
	
	struct method_idx
	{
		const char* method;
		method_call call;
	};

	const static method_idx call_tab[];

	void process_method_login(int64_t call_id, const Value& args);
	void process_method_submit(int64_t call_id, const Value& args);
	void process_method_keepalive(int64_t call_id, const Value& args);
	
	void send_error_response(int64_t call_id, const char* msg);

	check_job check_client_work(uint32_t nonce); 
};
