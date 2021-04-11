#include "time.hpp"
#include "client.hpp"
#include "encdec.h"
#include "hashpool.hpp"

constexpr uint32_t fix_diff = 4096;

const client::method_idx client::call_tab[] =
{
	{"submit", &client::process_method_submit},
	{"login", &client::process_method_login},
	{"keepalived", &client::process_method_keepalive},
	{nullptr, nullptr}
};

size_t client::max_calls_per_min = 0;
size_t client::bad_share_ban_cnt = 0;
uint32_t client::srv_start_diff = client::min_diff;
uint32_t client::srv_const_diff = 0;
size_t client::target_time = 0;
size_t client::start_window = 0;
size_t client::template_timeout = 0;
size_t client::client_timeouts[3] = {0};

std::atomic<uint32_t> g_extra_nonce_ctr(0);

client::client(SOCKET fd, const in6_addr& ip_addr, in_port_t port) : 
	fd(fd), active_time(get_timestamp()), domAlloc(json_dom_buf, json_buf_size), 
	parseAlloc(json_parse_buf, json_buf_size), jsonDoc(&domAlloc, json_buf_size, &parseAlloc)
{
	char str[64];
	ip_addr_str[0] = '\0';
	
	if(inet_ntop(AF_INET6, &ip_addr, str, sizeof(str)) == nullptr)
		throw std::runtime_error("inet_ntop failed!");
	
	snprintf(ip_addr_str, sizeof(ip_addr_str), "[%s]:%u", str, ntohs(port));

	extra_nonce = g_extra_nonce_ctr.fetch_add(1);
}

bool client::on_socket_read()
{
	while(true)
	{
		int ret = net_recv();
		if(ret <= 0)
			return !(ret == 0);

		if(recv_buf.len >= sock_buffer::sock_buf_size)
		{
			hard_abort();
			return false; // Exit due to buffer overflow
		}
		
		char* lnend;
		char* lnstart = recv_buf.buf;
		while((lnend = (char*)memchr(lnstart, '\n', recv_buf.len)) != nullptr)
		{
			lnend++;
			int lnlen = lnend - lnstart;
			
			if(!process_line(lnstart, lnlen))
			{
				hard_abort();
				return false; // Exit due to parsing error
			}
			
			recv_buf.len -= lnlen;
			lnstart = lnend;
		}
		
		//Got leftover data? Move it to the front
		if(recv_buf.len > 0 && recv_buf.buf != lnstart)
			memmove(recv_buf.buf, lnstart, recv_buf.len);
	}
}

bool client::process_line(char* buf, int len)
{
	int64_t call_id = 0;
	
	if(len <= 0)
		return false;
	
	clear_json_bufs();
	
	buf[len - 1] = '\0';
	if(jsonDoc.ParseInsitu(buf).HasParseError())
		return false;
	
	if(!jsonDoc.IsObject())
		return false;
	
	lpcJsVal cid = GetObjectMember(jsonDoc, "id");
	if(cid != nullptr && cid->IsInt64())
		call_id = cid->GetInt64();
	
	lpcJsVal method = GetObjectMember(jsonDoc, "method");
	lpcJsVal args = GetObjectMember(jsonDoc, "params");
	
	if(method == nullptr || args == nullptr || !method->IsString() || !args->IsObject())
	{
		send_error_response(call_id, "Malformed request");
		return true;
	}
	
	int64_t time = get_timestamp();
	if(time - flood_timestamp > 60)
	{
		flood_timestamp = time;
		flood_count = 1;
	}
	else
		flood_count++;
	
	if(flood_count > max_calls_per_min)
	{
		send_error_response(call_id, "Slow down");
		return true;
	}
	
	size_t i = 0;
	const char* s_method = method->GetString();
	while(call_tab[i].method != nullptr)
	{
		if(strcasecmp(call_tab[i].method, s_method) == 0)
		{
			(this->*call_tab[i].call)(call_id, *args);
			return true;
		}
		i++;
	}
	
	send_error_response(call_id, "Unknown method");
	return true;
}

void client::send_error_response(int64_t call_id, const char* msg)
{
	send_buf.len = snprintf(send_buf.buf, sizeof(send_buf.buf), 
			"{\"id\":%lld,\"jsonrpc\":\"2.0\",\"error\":{\"code\":-1,\"message\":\"%s\"}}\n", (long long int)call_id, msg);
	net_send();
}

void client::process_method_login(int64_t call_id, const Value& args)
{
	if(logged_in)
	{
		send_error_response(call_id, "You are logged in.");
		return;
	}

	GetObjectMember(args, "pass");
	GetObjectMember(args, "login");

	my_id.uid = 1;
	my_id.miner = 0;
	my_id.rid = 0;

	get_new_job();

	char hex_jobid[9];
	char hex_target[9];
	char hex_blob[768];
	const char* pow_type = pow_type_to_str(cur_job.type);

	memcpy(cur_job.prepow + cur_job.prepow_len - sizeof(uint32_t)*2, &extra_nonce, sizeof(uint32_t));

	bin2hex(cur_job.prepow, cur_job.prepow_len, hex_blob);
	bin2hex((const unsigned char*)&jobid, sizeof(uint32_t), hex_jobid);
 	uint32_t t = diff_to_target(fix_diff);
	bin2hex((const unsigned char*)&t, sizeof(uint32_t), hex_target);

	char seed_hash[65];
	bin2hex(cur_job.rx_seed.data, cur_job.rx_seed.size, seed_hash);
	send_buf.len = snprintf(send_buf.buf, sizeof(send_buf.buf), "{\"id\":%lld,\"jsonrpc\":\"2.0\",\"error\":null,\"result\":"
		"{\"id\":\"decafbad0\",\"job\":{\"blob\":\"%s\",\"job_id\":\"%s\",\"target\":\"%s\",\"pow_algo\":\"%s\",\"seed_hash\":\"%s\"},\"status\":\"OK\"}}\n",
		(long long int)call_id, hex_blob, hex_jobid, hex_target, pow_type, seed_hash);

	net_send();
	logged_in = true;
}

void client::process_method_submit(int64_t call_id, const Value& args)
{
	lpcJsVal s_jobid, s_nonce, s_result, s_hashcnt;
	s_jobid = GetObjectMember(args, "job_id");
	s_nonce = GetObjectMember(args, "nonce");
	s_result = GetObjectMember(args, "result");
	s_hashcnt = GetObjectMember(args, "hashcount_total");

	if(s_jobid == nullptr || s_nonce == nullptr || s_result == nullptr ||
		!s_jobid->IsString() || !s_nonce->IsString() || !s_result->IsString() ||
		s_jobid->GetStringLength() != 8 || s_nonce->GetStringLength() != 8 || s_result->GetStringLength() != 64)
	{
		send_error_response(call_id, "Malformed submit");
		return;
	}

	uint32_t net_jobid, nonce;
	if(!hex2bin(s_jobid->GetString(), 8, (unsigned char*)&net_jobid))
	{
		send_error_response(call_id, "Invalid jobid");
		return;
	}

	if(!hex2bin(s_nonce->GetString(), 8, (unsigned char*)&nonce))
	{
		send_error_response(call_id, "Invalid nonce");
		return;
	}

	uint64_t claim_work;
	if(!hex2bin(s_result->GetString() + 48, 16, (unsigned char*)&claim_work))
	{
		send_error_response(call_id, "Invalid result");
		return;
	}

	if(net_jobid != jobid)
	{
		send_error_response(call_id, "Stale job id");
		return;
	}

	memcpy(cur_job.prepow + cur_job.prepow_len - sizeof(uint32_t), &nonce, sizeof(uint32_t));

	std::future<void> future;
	hashpool::check_job job;
	job.data = cur_job.prepow;
	job.data_len = cur_job.prepow_len;
	job.dataset_id = cur_job.rx_seed.get_id();
	future = job.ready.get_future();
	
	hashpool::inst().push_job(job);
	future.wait();

	if(job.error)
	{
		send_error_response(call_id, "Server error while checking share.");
		return;
	}

	uint32_t target = 0xFFFFFFFFU / fix_diff;
	if(job.hash.get_work32() > target)
	{
		send_error_response(call_id, "Bad share");
		return;
	}

	uint64_t actual_diff = work_to_diff(job.hash.get_work64());
	
	if(actual_diff > 4096)//cur_job.block_diff)
	{
		uint64_t full_nonce = __builtin_bswap64((uint64_t(nonce) << 32ull) | extra_nonce);
		logger::inst().info("Block submit: ", actual_diff);
		node::inst().send_job_result(cur_job, full_nonce, job.hash);
	}

	send_buf.len = snprintf(send_buf.buf, sizeof(send_buf.buf),
		"{\"id\":%lld,\"jsonrpc\":\"2.0\",\"error\":null,\"result\":{\"status\":\"OK\"}}\n", (long long int)call_id);
	net_send();
}

void client::process_method_keepalive(int64_t call_id, const Value& args)
{
	send_buf.len = snprintf(send_buf.buf, sizeof(send_buf.buf),
		"{\"id\":%lld,\"jsonrpc\":\"2.0\",\"error\":null,\"result\":{\"status\":\"OK\"}}\n", (long long int)call_id);
	net_send();
}

bool client::on_new_block(int64_t timestamp_ms)
{
	if(aborting)
		return false;

	get_new_job();

	if(!logged_in)
		return true;

	memcpy(cur_job.prepow + cur_job.prepow_len - sizeof(uint32_t)*2, &extra_nonce, sizeof(uint32_t));

	char hex_jobid[9];
	char hex_target[9];
	char hex_blob[768];
	const char* pow_type = pow_type_to_str(cur_job.type);

	bin2hex(cur_job.prepow, cur_job.prepow_len, hex_blob);
	bin2hex((const unsigned char*)&jobid, sizeof(uint32_t), hex_jobid);
	uint32_t t = diff_to_target(fix_diff);
	bin2hex((const unsigned char*)&t, sizeof(uint32_t), hex_target);
	
	char seed_hash[65];
	bin2hex(cur_job.rx_seed.data, cur_job.rx_seed.size, seed_hash);

	send_buf.len = snprintf(send_buf.buf, sizeof(send_buf.buf), "{\"jsonrpc\":\"2.0\",\"method\":\"job\",\"params\":"
		"{\"blob\":\"%s\",\"job_id\":\"%s\",\"target\":\"%s\",\"pow_algo\":\"%s\",\"seed_hash\":\"%s\"}}\n",
		hex_blob, hex_jobid, hex_target,pow_type,seed_hash);

	net_send();
	return true;
}
