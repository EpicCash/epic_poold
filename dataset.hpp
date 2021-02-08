#pragma once
#include <shared_mutex>
#include <chrono>
#include <thread>
#include <vector>
#include <iostream>
#include <memory>
#include <atomic>

#include "randomx.h"
#include "vector32.h"

struct dataset
{
	dataset() : seed_id(0)
	{
		randomx_flags fl = (randomx_flags)(RANDOMX_FLAG_FULL_MEM | RANDOMX_FLAG_JIT | RANDOMX_FLAG_HARD_AES);
		ch = randomx_alloc_cache(fl);
		ds = randomx_alloc_dataset(fl);
	}
	
	~dataset()
	{
		randomx_release_dataset(ds);
		randomx_release_cache(ch);
	}

	void init_cache()
	{
		randomx_init_cache(ch, static_cast<const uint8_t*>(ds_seed), 32);
	}

	void calculate(size_t idx, size_t thd_cnt)
	{
		size_t work_size = randomx_dataset_item_count();
		size_t batch_size = work_size / thd_cnt;
		randomx_init_dataset(ds, ch, idx*batch_size, idx == thd_cnt-1 ? (batch_size+work_size%thd_cnt) : batch_size);
	}

	std::shared_timed_mutex mtx;
	v32 ds_seed;
	std::atomic<uint64_t> seed_id;
	randomx_cache* ch;
	randomx_dataset* ds; 
};
