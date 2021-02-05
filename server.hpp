#pragma once
#include <list>
#include <thread>
#include <mutex>

#include "socks.h"
#include "log.hpp"

struct dummy_client_pool
{
	void add_socket(SOCKET fd, in6_addr& ip, in_port_t port)
	{
	}

	bool is_full()
	{
		return false;
	}

	void on_new_block()
	{
	}

	uint32_t get_active()
	{
		return 0;
	}

	bool is_finished()
	{
		return false;
	}
};

struct dummy_tls_client_pool : public dummy_client_pool
{
};

class server
{
public:
	inline static server& inst()
	{
		static server inst;
		return inst;
	}

	bool start();
	void notify_new_block();

private:
	server() {};

	template <typename pool_t>
	void listen_main();

	using client_pool_t = dummy_client_pool;
	using tls_client_pool_t = dummy_tls_client_pool;

	template <typename T>
	inline std::list<T>& select_pool();
	
	std::mutex store_mtx;
	std::list<client_pool_t> client_pools;
	std::thread listen_thd;
	
	std::mutex tls_store_mtx;
	std::list<tls_client_pool_t> tls_client_pools;
	std::thread tls_listen_thd;

	SOCKET plain_sck;
	SOCKET tls_sck;
	
	template <typename pool_t, typename Functor>
	inline void iterate_pools(std::list<pool_t>& pools, Functor functor)
	{
		/*Iterate over client_pools removing empty ones*/
		auto it = pools.begin();
		while(it != pools.end())
		{
			pool_t& pool = *it;
			
			if(pool.is_finished())
			{
				it = pools.erase(it);
				logger::inst().log_msg("THMGT Client pool deallocated. Size: %lu", pools.size());
				continue;
			}
			else
				it++;
			
			if(!functor(pool))
				break;
		}
	}
};


template <>
inline std::list<server::client_pool_t>& server::select_pool<server::client_pool_t>() { return client_pools; }

template <>
inline std::list<server::tls_client_pool_t>& server::select_pool<server::tls_client_pool_t>() { return tls_client_pools; }

