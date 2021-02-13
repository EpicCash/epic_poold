#include "socks.h"
#include "server.hpp"
#include "jconf.hpp"
#include <assert.h>
#include <netinet/tcp.h>

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

SSL_CTX* g_ssl_ctx = nullptr;
bool server::start()
{
	sockaddr_in6 my_addr;
	uint16_t plain_port = jconf::inst().get_server_port();
	uint16_t tls_port = jconf::inst().get_server_tls_port();

	client::set_limits();

	while(!node::inst().has_first_job())
	{
		logger::inst().info("Waiting for a job...");
		int slp = 10;
		while((slp = sleep(slp)) != 0);
	}

	if(plain_port == 0 && tls_port == 0)
		return false;

	if(plain_port != 0)
	{
		plain_sck = socket(AF_INET6, SOCK_STREAM, 0);
		
		if(plain_sck == INVALID_SOCKET)
			return false;
		
		int enable = 1;
		if(setsockopt(plain_sck, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
			logger::inst().warn("setsockopt(SO_REUSEADDR) failed");
		
		my_addr = {0};
		my_addr.sin6_family = AF_INET6;
		my_addr.sin6_addr = in6addr_any;
		my_addr.sin6_port = htons(plain_port);
		
		if(bind(plain_sck, (sockaddr*)&my_addr, sizeof(my_addr)) < 0)
			return false;
		
		if(listen(plain_sck, SOMAXCONN) < 0)
			return false;
	}
	
	if(tls_port != 0)
	{
		tls_sck = socket(AF_INET6, SOCK_STREAM, 0);
		
		if(tls_sck == INVALID_SOCKET)
			return false;
		
		int enable = 1;
		if(setsockopt(tls_sck, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
			logger::inst().warn("setsockopt(SO_REUSEADDR) failed");
		
		my_addr = {0};
		my_addr.sin6_family = AF_INET6;
		my_addr.sin6_addr = in6addr_any;
		my_addr.sin6_port = htons(tls_port);
		
		if(bind(tls_sck, (sockaddr*)&my_addr, sizeof(my_addr)) < 0)
			return false;
		
		if(listen(tls_sck, SOMAXCONN) < 0)
			return false;
	}
	
	if(plain_port != 0)
		listen_thd = std::thread(&server::listen_main<client_pool_t>, this);
	
	if(tls_port != 0)
		tls_listen_thd = std::thread(&server::listen_main<tls_client_pool_t>, this);
	return true;
}

template <typename pool_t>
void server::listen_main()
{
	SOCKET main_sock = std::is_same<pool_t, client_pool_t>::value ? plain_sck : tls_sck;
	std::mutex& mtx = std::is_same<pool_t, client_pool_t>::value ? store_mtx : tls_store_mtx;
	std::list<pool_t>& pools = select_pool<pool_t>();
	
	sockaddr_in6 cli_addr;
	SOCKET cli_sck;
	uint32_t errs = 0;

	logger::inst().info("Main listening thread started.");
	while(true)
	{
		cli_addr = {0};
		socklen_t ln = sizeof(cli_addr);
		cli_sck = accept4(main_sock, (sockaddr*)&cli_addr, &ln, SOCK_NONBLOCK | SOCK_CLOEXEC);
		
		if(cli_sck < 0)
		{
			int err = errno;
			if(err == EMFILE)
			{
				logger::inst().err("Max open files limit reached!");
				while(sleep(1) == 0); // sleep for a second
			}
			else
			{
				logger::inst().err("Error in accept4: ", err);
			}
			errs++;
			assert(errs < 5);
			continue;
		}
		else
		{
			errs = 0;
		}
		
		int optval = 1;
		if(setsockopt(cli_sck, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) != 0)
		{
			logger::inst().warn("SO_KEEPALIVE on socket failed");
			sock_abort(cli_sck);
			continue;
		}
		
		if(setsockopt(cli_sck, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) != 0)
		{
			logger::inst().warn("TCP_NODELAY on socket failed");
			sock_abort(cli_sck);
			continue;
		}
		
		std::unique_lock<std::mutex> mlock(mtx);
		
		pool_t* found_pool = nullptr;
		iterate_pools<pool_t>(pools, [&found_pool](pool_t& pool) { if(!pool.is_full()) {found_pool = &pool; return false;} else return true; });
		
		if(found_pool == nullptr)
		{
			pools.emplace_back();
			logger::inst().info("THMGT Client pool allocated. Size: ", pools.size());
			found_pool = &pools.back();
		}
		
		found_pool->add_socket(cli_sck, cli_addr.sin6_addr, cli_addr.sin6_port);
		mlock.unlock();
	}
}

void server::notify_new_block()
{
	uint32_t active_cli = 0;
	if(listen_thd.joinable())
	{
		std::unique_lock<std::mutex> mlock(store_mtx);
		iterate_pools<client_pool_t>(client_pools, [&active_cli](client_pool_t& pool) { active_cli += pool.get_active(); pool.on_new_block(); return true; });
		mlock.unlock();
		
		logger::inst().info("THMGT Active clients ", active_cli, " in ", client_pools.size(), " pools");
	}

	active_cli = 0;
	if(tls_listen_thd.joinable())
	{
		std::unique_lock<std::mutex> mlock_tls(tls_store_mtx);
		iterate_pools<tls_client_pool_t>(tls_client_pools, [&active_cli](tls_client_pool_t& pool) { active_cli += pool.get_active(); pool.on_new_block(); return true; });
		mlock_tls.unlock();
		
		logger::inst().info("THMGT Active clients ", active_cli, " in ", tls_client_pools.size(), " pools");
	}

	logger::inst().info("Block refreshed!");
}
