#pragma once
#include <list>
#include <thread>
#include <mutex>

#include "socks.h"
#include "log.hpp"
#include "client_pool.hpp"
#include "client.hpp"

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

	using client_pool_t = client_pool<client, 256>;
	using tls_client_pool_t = client_pool<client, 254>;

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
				logger::inst().dbghi("THMGT Client pool deallocated. Size: ", pools.size());
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

