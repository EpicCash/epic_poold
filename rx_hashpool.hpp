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
#include <shared_mutex>
#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

#include "randomx.h"
#include "vector32.h"
#include "thdq.hpp"
#include "log.hpp"
#include "check_job.hpp"

constexpr uint64_t invalid_id = uint64_t(-1);

struct rx_dataset
{
	rx_dataset() : loaded_seed_id(invalid_id), ready_seed_id(invalid_id)
	{
		ch = randomx_alloc_cache((randomx_flags)(RANDOMX_FLAG_LARGE_PAGES | RANDOMX_FLAG_JIT));
		if(ch == nullptr)
		{
			logger::inst().err("Failed to allocate RandomX cache with large pages. Enable large page support for faster hashing.");
			ch = randomx_alloc_cache((randomx_flags)(RANDOMX_FLAG_JIT));
			if(ch == nullptr)
			{
				logger::inst().err("Failed to allocate RandomX cache (not enough RAM).");
				exit(0);
			}
		}
	}

	rx_dataset(const rx_dataset& r) = delete;
	rx_dataset& operator=(const rx_dataset& r) = delete;
	rx_dataset(rx_dataset&& r) = delete;
	rx_dataset& operator=(rx_dataset&& r) = delete;

	~rx_dataset()
	{
		randomx_release_cache(ch);
	}
	
	void init_cache()
	{
		randomx_init_cache(ch, static_cast<const uint8_t*>(ds_seed), 32);
	}

	/* 
	 * loaded_seed_id - seed id calculating or ready (or zero)
	 * ready_seed_id - seed id that is ready (or zero), set after calculation
	 */

	v32 ds_seed;
	std::atomic<uint64_t> loaded_seed_id;
	std::atomic<uint64_t> ready_seed_id;
	std::shared_timed_mutex mtx;
	randomx_cache* ch;
};

class rx_hashpool
{
public:
	constexpr static size_t hash_len = 32;
	constexpr static size_t hash_thd_count = 2;

	struct rx_check_job : public check_job
	{
		std::promise<void> ready;
		uint64_t dataset_id;
	};

	inline static rx_hashpool& inst()
	{
		static rx_hashpool inst;
		return inst;
	};

	inline void push_job(rx_check_job& job) { jobs.push(&job); }

	inline bool has_dataset(uint64_t ndl)
	{
		for(rx_dataset& hay : ds)
		{
			if(ndl == hay.loaded_seed_id)
				return true;
		}
		return false;
	}

	inline void calculate_dataset(const v32& dataset_seed)
	{
		size_t i = ds_ctr.fetch_add(1) % ds.size();
		ds[i].ready_seed_id = invalid_id;
		ds[i].ds_seed = dataset_seed;
		ds[i].loaded_seed_id = dataset_seed.get_id();
		std::thread thd(&rx_hashpool::dataset_thd_main, this, i);
		thd.detach();
	}

private:
	rx_hashpool();

	void dataset_thd_main(size_t ds_idx);
	void hash_thd_main();

	std::array<rx_dataset, 2> ds;
	std::atomic<size_t> ds_ctr;

	thdq<rx_check_job*> jobs;
	std::vector<std::thread> threads;
	
};
