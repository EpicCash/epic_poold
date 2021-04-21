#include "node.h"

#include <iostream>
#include <stdio.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */
#include <errno.h>
#include <string.h>

#include "encdec.h"
#include "log.hpp"
#include "jconf.hpp"
#include "time.hpp"
#include "server.hpp"
#include "hashpool.hpp"

node::node() : domAlloc(json_dom_buf, json_buffer_len),
	parseAlloc(json_parse_buf, json_buffer_len),
	jsonDoc(&domAlloc, json_buffer_len, &parseAlloc),
	run_loop(true), sock_fd(-1), current_job(nullptr),
	last_job_ts(0), logged_in(false)
{
}

bool node::node_connect(const char* addr, const char* port)
{
	addrinfo hints = { 0 };
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	addrinfo *ptr;
	if (getaddrinfo(addr, port, &hints, &ptr) != 0)
	{
		logger::inst().err("Node getaddrinfo failed");
		return false;
	}

	sock_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

	if(sock_fd < 0)
	{
		sock_fd = -1;
		logger::inst().err("Node socket failed");
		return false;
	}

	int ret = connect(sock_fd, ptr->ai_addr, ptr->ai_addrlen);
	freeaddrinfo(ptr);

	if(ret != 0)
	{
		close(sock_fd);
		sock_fd = -1;
		logger::inst().err("Node connect failed");
		return false;
	}

	logger::inst().info("Node connected!");
	return true;
}

void node::thread_main()
{
	while(run_loop)
	{
		if(!node_connect(jconf::inst().get_node_hostname(), jconf::inst().get_node_port()))
		{
			unix_sleep(5);
			continue;
		}

		recv_main();

		close(sock_fd);
		sock_fd = -1;
		unix_sleep(1); // Prevent rapid polling on repetivie errors
	}
}
void node::recv_main()
{
	int ret;
	ret = snprintf(send_buffer, data_buffer_len, "{\"id\":\"0\",\"jsonrpc\":\"2.0\",\"method\":\"login\", \"params\":"
		"{\"login\":\"%s\",\"pass\":\"%s\",\"agent\":\"epic_poold\"}}\n", 
		jconf::inst().get_node_username(), jconf::inst().get_node_password());

	last_job_ts = get_timestamp_ms();

	if(ret <=0 || send(sock_fd, send_buffer, ret, 0) != ret)
		return;

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

	size_t datalen = 0;
	uint64_t fatal_node_timeout_ms = jconf::inst().get_fatal_node_timeout() * 1000;
	uint64_t template_timeout_ms = jconf::inst().get_template_timeout() * 1000;
	while(run_loop)
	{
		ret = recv(sock_fd, recv_buffer + datalen, data_buffer_len - datalen, 0);

		if(ret == -1 && (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK))
		{
			uint64_t last_job_delta = get_timestamp_ms()-last_job_ts;
			if(last_job_delta > fatal_node_timeout_ms)
			{
				logger::inst().err("Fatal node timeout.");
				exit(0);
			}
			if(last_job_delta > template_timeout_ms)
			{
				send_template_request();
				last_job_ts = get_timestamp_ms();
			}
			continue;
		}

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
				return;

			datalen -= retlen;
			msgstart += retlen;
		}

		//Got leftover data? Move it to the front
		if (datalen > 0 && recv_buffer != msgstart)
			memmove(recv_buffer, msgstart, datalen);
	}
}

v32 IntArrayToVector(const Value& arr_v)
{
	v32 ret;
	auto arr = GetArray(arr_v, 32);
	for(size_t i=0; i < 32; i++)
	{
		uint32_t v = GetUint(arr[i]);
		if(v >= 256) /* Because using hex was too complicated... */
			throw json_parse_error("Invalid Int Array");
		ret.data[i] = uint8_t(v);
	}
	return ret;
}

bool node::send_template_request()
{
	const char job_tmpl[] ="{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"getjobtemplate\",\"params\":{\"algorithm\":\"randomx\"}}\n";
	if(send(sock_fd, job_tmpl, sizeof(job_tmpl)-1, 0) != sizeof(job_tmpl)-1)
	{
		logger::inst().err("Node: Send socket error.");
		return false;
	}
	return true;
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

	msglen = i+1;
	msg[i] = '\0';

	jsonDoc.SetNull();
	parseAlloc.Clear();
	domAlloc.Clear();
	jsonDoc.SetNull();

	logger::inst().dbglo("Node RPC recv: ", msg);
	if(jsonDoc.ParseInsitu(msg).HasParseError() || !jsonDoc.IsObject())
	{
		logger::inst().err("Node RPC Error: JSON parsing error");
		return -1;
	}

	try
	{
		lpcJsVal result, error = nullptr;
		ssize_t call_id = GetJsonCallId(jsonDoc);
		const char* method = GetJsonString(jsonDoc, "method");
		bool has_error = false;

		if(call_id != -1)
		{
			result = GetObjectMember(jsonDoc, "result");
			error = GetObjectMember(jsonDoc, "error");

			if(result == nullptr && error == nullptr)
				throw json_parse_error("call with no result or error");

			if(error != nullptr && !error->IsNull())
				has_error = true;
		}
		else
		{
			result = GetObjectMember(jsonDoc, "params");
			if(result == nullptr)
				throw json_parse_error("invoke with no params");
		}

		if(has_error)
		{
			const Value& error_msg = GetObjectMemberT(*error, "message");
			const Value& error_cde = GetObjectMemberT(*error, "code");
			logger::inst().err("Node RPC Error: ", error_msg.GetString(), " code: ", error_cde.GetInt());
			return msglen;
		}

		if(strcmp(method, "submit") == 0)
		{
			logger::inst().info("Block submit OK!");
			return msglen;
		}

		if(strcmp(method, "login") == 0)
		{
			logger::inst().dbghi("Node login OK");
			send_template_request();
			return msglen;
		}

		if(strcmp(method, "job") == 0 || strcmp(method, "getjobtemplate") == 0)
		{
			const Value& res = *result;
			uint32_t height = GetJsonUInt(res, "height");

			// Daemon does that sometimes (?)
			if(height == 0)
			{
				logger::inst().info("Ignoring 0-height job.");
				send_template_request();
				return msglen;
			}

			pow_type job_type;
			uint64_t block_diff;
			const char* algo = GetJsonString(res, "algorithm");
			if(strcmp(algo, "randomx") == 0)
				job_type = pow_type::randomx;
			else if(strcmp(algo, "progpow") == 0)
				job_type = pow_type::progpow;
			else if(strcmp(algo, "cuckoo") == 0)
				job_type = pow_type::cuckoo;
			else
				throw json_parse_error(std::string("Unknown algorithm: ") + algo);

			for(const Value& v : GetArray(GetObjectMemberT(res, "block_difficulty")))
			{
				auto a = GetArray(v, 2);
				if(strcmp(GetString(a[0]), algo) == 0)
					block_diff = GetUint64(a[1]);
			}

			if(block_diff == 0)
			{
				logger::inst().info("Ignoring 0-diff job.");
				send_template_request();
				return msglen;
			}

			jobs.emplace_back();
			jobdata* job = &jobs.back();
			job->rx_seed.set_all_zero();
			job->rx_next_seed.set_all_zero();

			job->type = job_type;
			job->block_diff = block_diff;

			auto epochs = GetArray(GetObjectMemberT(res, "epochs"));
			if(epochs.Size() != 1 && epochs.Size() != 2)
				throw json_parse_error("Unexpected epochs size");

			bool has_our_epoch = false;
			for(const Value& epoch : epochs)
			{
				auto epoch_data = GetArray(epoch, 3);
				uint32_t real_start = GetUint(epoch_data[0]);
				uint32_t real_end = GetUint(epoch_data[1]);
				if(real_end == 0 || real_end <= real_start)
					throw json_parse_error("Epoch sanity check failed!");
				real_start += 1; //epic daemon bug

				logger::inst().dbghi("Epoch: real_start: ", real_start, " real_end: ", real_end, " height: ", height);
				if(height >= real_start && height <= real_end)
				{
					has_our_epoch = true;
					job->rx_seed = IntArrayToVector(epoch_data[2]);
					if(!hashpool::inst().has_dataset(job->rx_seed.get_id()))
						hashpool::inst().calculate_dataset(job->rx_seed);
				}
				else if(height < real_start)
				{
					job->rx_next_seed = IntArrayToVector(epoch_data[2]);
					if(!hashpool::inst().has_dataset(job->rx_next_seed.get_id()))
						hashpool::inst().calculate_dataset(job->rx_next_seed);
				}
				else
					throw json_parse_error("Unidentfied epoch");
			}

			if(!has_our_epoch)
				throw json_parse_error("We don't have our epoch");

			job->jobid = GetJsonUInt(res, "job_id");
			job->height = height;

			unsigned prepow_len;
			const char* prepow = GetJsonString(res, "pre_pow", prepow_len);

			if(prepow_len/2 > sizeof(job->prepow) - sizeof(uint64_t))
				throw json_parse_error(std::string("Too long prepow, size: ") + std::to_string(prepow_len/2));

			if(!hex2bin(prepow, prepow_len, job->prepow))
				throw json_parse_error("Hex-decode on prepow failed");
			job->prepow_len = prepow_len / 2;

			memset(job->prepow + job->prepow_len, 0, sizeof(uint64_t));
			job->prepow_len += sizeof(uint64_t);

			logger::inst().dbglo("Got a job:", 
				"\ntype: ", uint32_t(job->type),
				"\nblock_diff: ", job->block_diff,
				"\nheight: ", job->height,
				"\nrx_seed: ", job->rx_seed,
				"\nrx_next_seed: ", job->rx_next_seed);

			last_job_ts = get_timestamp_ms();
			current_job = job;
			server::inst().notify_new_block();

			if(jobs.size() > 5)
				jobs.pop_front();

			return msglen;
		}

		throw json_parse_error(std::string("Unkonwn method: ") + method);
	}
	catch(const json_parse_error& exc)
	{
		logger::inst().err("Node error: ", exc.what());
	}

	return -1;
}
