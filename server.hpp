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

