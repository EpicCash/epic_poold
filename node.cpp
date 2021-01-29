#include "node.h"

#include <iostream>
#include <stdio.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */
#include <errno.h>
#include <string.h>

node::node() : domAlloc(json_dom_buf, json_buffer_len),
	parseAlloc(json_parse_buf, json_buffer_len),
	jsonDoc(&domAlloc, json_buffer_len, &parseAlloc),
	logged_in(false), g_call_id(1), sock_fd(-1)
{
}

bool node::node_connect(const char* addr, const char* port)
{
	logged_in = false;

	addrinfo hints = { 0 };
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	addrinfo *ptr;
	if (getaddrinfo(addr, port, &hints, &ptr) != 0)
	{
		std::cout << "getaddrinfo failed" << std::endl;
		return false;
	}

	sock_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

	if(sock_fd < 0)
	{
		sock_fd = -1;
		std::cout << "socket failed" << std::endl;
		return false;
	}

	int ret = connect(sock_fd, ptr->ai_addr, ptr->ai_addrlen);
	freeaddrinfo(ptr);

	if(ret != 0)
	{
		close(sock_fd);
		sock_fd = -1;
		std::cout << "connect failed" << std::endl;
		return false;
	}

	std::cout << "Connected!" << std::endl;
	recv_thd = std::thread(&node::recv_main, this);
	return true;
}

void node::node_disconnect()
{
	if(sock_fd != -1)
	{
		::shutdown(sock_fd, SHUT_RDWR);
	}
}

void node::recv_main()
{
	int ret;
	ret = snprintf(send_buffer, data_buffer_len, "{\"id\":\"0\",\"jsonrpc\":\"2.0\",\"method\":\"login\", \"params\":"
		"{\"login\":\"%s\",\"pass\":\"%s\",\"agent\":\"%s\"}}\n", login.c_str(), passw.c_str(), agent.c_str());

	if(ret <=0 || send(sock_fd, send_buffer, ret, 0) != ret)
	{
		close(sock_fd);
		sock_fd = -1;
		return;
	}

	size_t datalen = 0;
	while (true)
	{
		ret = recv(sock_fd, recv_buffer + datalen, data_buffer_len - datalen, 0);

		if(ret <= 0)
			break;

		datalen += ret;

		if(datalen >= data_buffer_len)
			break;

		char* msgstart = recv_buffer;
		ssize_t retlen;
		while ((retlen = json_proc_msg(msgstart, datalen)) != 0)
		{
			if(retlen < 0)
				break;

			datalen -= retlen;
			msgstart += retlen;
		}

		//Got leftover data? Move it to the front
		if (datalen > 0 && recv_buffer != msgstart)
			memmove(recv_buffer, msgstart, datalen);
	}

	logged_in = false;
	close(sock_fd);
	sock_fd = -1;
}

void node::make_node_call(const char* call, const char* data, response& rsp)
{
	bool first=true;
	while(true)
	{
		if(!first)
			std::this_thread::sleep_for(std::chrono::seconds(5));
		first = false;

		persistent_connect();
		size_t my_call_id = g_call_id.fetch_add(1);
		std::future<response> ready;

		{
			std::unique_lock<std::mutex> lck(call_mtx);
			ready = call_map.emplace(std::piecewise_construct,
				std::forward_as_tuple(my_call_id),
				std::forward_as_tuple()).first->second.get_future();
		}

		int ret;
		ret = snprintf(send_buffer, data_buffer_len, "{\"id\":\"%zu\",\"jsonrpc\":\"2.0\",\"method\":\"%s\", \"params\":"
			"{%s}}\n", my_call_id, call, data);

		std::cout << send_buffer << std::endl;
		if(ret <=0 || send(sock_fd, send_buffer, ret, 0) != ret)
		{
			node_disconnect();
			recv_thd.join();
			continue;
		}

		auto res = ready.wait_for(std::chrono::seconds(5));
		
		{
			std::unique_lock<std::mutex> lck(call_mtx);
			call_map.erase(my_call_id);
		}

		if(res == std::future_status::timeout)
		{
			std::cout << "Call timed out!" << std::endl;
			node_disconnect();
			recv_thd.join();
			continue;
		}
		
		rsp = ready.get();
		std::cout << "Call success" << std::endl;
		return;
	}
}

void node::persistent_connect()
{
	std::unique_lock<std::mutex> lck(chk_mtx);
	bool first=true;
	while(true)
	{
		std::cout << "Pers connect!" << std::endl;
		if(!first)
			std::this_thread::sleep_for(std::chrono::seconds(5));
		first = false;

		if(logged_in)
			return;

		if(!node_connect(host.c_str(), port.c_str()))
			continue;

		std::unique_lock<std::mutex> lck2(cv_login_mtx);
		if(cv_login.wait_for(lck2, std::chrono::seconds(5), [&]{return logged_in.load();}))
			return;

		std::cout << "Login timed out!" << std::endl;

		node_disconnect();
		recv_thd.join();
	}
}

ssize_t node::json_proc_msg(char* msg, size_t msglen)
{
	size_t i;
	for(i = 0; i < msglen; i++)
	{
		if(msg[i] == '\n')
			break;
	}

	//Newline not found
	if(i == msglen)
		return 0;

	msg[i] = '\0';
	jsonDoc.SetNull();
	parseAlloc.Clear();
	domAlloc.Clear();
	jsonDoc.SetNull();

	std::cout << msg << std::endl;
	if(jsonDoc.ParseInsitu(msg).HasParseError() || !jsonDoc.IsObject())
	{
		std::cout << "JSON parsing error" << std::endl;
		return -1;
	}

	lpcJsVal result, error;
	//todo: needs to be caught
	size_t call_id = GetJsonUInt64(jsonDoc, "id");
	const char* method = GetJsonString(jsonDoc, "method");

	result = GetObjectMember(jsonDoc, "result");
	error = GetObjectMember(jsonDoc, "error");

	if(result == nullptr && error == nullptr)
	{
		std::cout << "JSON parsing error" << std::endl;
		return -1;
	}

	bool has_error = false;
	if(error != nullptr && !error->IsNull())
		has_error = true;

	if(strcmp(method, "login") == 0)
	{
		//Handle login here
		if(logged_in)
		{
			std::cout << "Invalid login reply" << std::endl;
			return -1;
		}
		
		if(has_error)
		{
			std::cout << "Login failed" << std::endl;
			return -1;
		}

		std::cout << "Login OK" << std::endl;
		logged_in = true;
		cv_login.notify_all();
		return msglen;
	}

	if(strcmp(method, "job") == 0)
	{
		std::cout << "Job packet recevied OK" << std::endl;
		return msglen;
	}

	std::unique_lock<std::mutex> lck(call_mtx);
	auto it = call_map.find(call_id);

	if(it != call_map.end())
	{
		response res;
		res.error = has_error;
		it->second.set_value(res);
		return msglen;
	}

	return -1;
}
