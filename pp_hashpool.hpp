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

#include "ethash/keccak.hpp"
#include "ethash/progpow.hpp"
#include "vector32.h"
#include "thdq.hpp"
#include "log.hpp"
#include "check_job.hpp"

constexpr uint64_t invalid_epoch = uint64_t(-1);

struct pp_dataset
{
	pp_dataset() : loaded_epoch(invalid_epoch), ready_epoch(invalid_epoch), dataset_ptr(nullptr)
	{
	}

	pp_dataset(const pp_dataset& r) = delete;
	pp_dataset& operator=(const pp_dataset& r) = delete;
	pp_dataset(pp_dataset&& r) = delete;
	pp_dataset& operator=(pp_dataset&& r) = delete;

	~pp_dataset()
	{
		if(dataset_ptr != nullptr)
			ethash_destroy_epoch_context(dataset_ptr);
	}

	/* 
	 * loaded_seed_id - seed id calculating or ready (or zero)
	 * ready_seed_id - seed id that is ready (or zero), set after calculation
	 */
	std::atomic<uint64_t> loaded_epoch;
	std::atomic<uint64_t> ready_epoch;
	std::shared_timed_mutex mtx;
	ethash::epoch_context* dataset_ptr;
};

class pp_hashpool
{
public:
	constexpr static size_t hash_len = 32;
	constexpr static size_t hash_thd_count = 2;

	struct pp_check_job : public check_job
	{
		std::promise<void> ready;
		uint64_t nonce;
		uint64_t block_number;
		ethash::result res;
	};

	inline static pp_hashpool& inst()
	{
		static pp_hashpool inst;
		return inst;
	};

	inline void push_job(pp_check_job& job) { jobs.push(&job); }

	inline void notify_block(uint64_t block_number)
	{
		notify_epoch(ethash::get_epoch_number(block_number));
		notify_epoch(ethash::get_epoch_number(block_number+1));
	}

private:
	inline void notify_epoch(uint64_t epoch_number)
	{
		if(!has_dataset(epoch_number))
		{
			size_t i = ds_ctr.fetch_add(1) % ds.size();
			ds[i].ready_epoch = invalid_epoch;
			ds[i].loaded_epoch = epoch_number;
			std::thread thd(&pp_hashpool::dataset_thd_main, this, i);
			thd.detach();
		}
	}

	inline bool has_dataset(uint64_t epoch_number)
	{
		for(pp_dataset& hay : ds)
		{
			if(epoch_number == hay.loaded_epoch)
				return true;
		}
		return false;
	}

	pp_hashpool();

	void dataset_thd_main(size_t ds_idx);
	void hash_thd_main();

	std::array<pp_dataset, 2> ds;
	std::atomic<size_t> ds_ctr;

	thdq<pp_check_job*> jobs;
	std::vector<std::thread> threads;
	
};
