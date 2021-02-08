#pragma once
#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

#include "dataset.hpp"
#include "vector32.h"
#include "thdq.hpp"
#include "log.hpp"

class hashpool
{
public:
	constexpr static size_t hash_len = 32;
	constexpr static size_t hash_thd_count = 2;

	struct check_job
	{
		const void* data;
		size_t data_len;
		v32 hash;
		bool error;
		uint64_t dataset_id;
		std::promise<void> ready;
	};

	inline static hashpool& inst()
	{
		static hashpool inst;
		return inst;
	};

	inline void push_job(check_job& job) { jobs.push(&job); }

	inline bool has_dataset(uint64_t ndl)
	{
		for(size_t hay : ds_ids)
		{
			if(ndl == hay)
				return true;
		}
		return false;
	}

	inline void calculate_dataset(const v32& dataset_seed)
	{
		size_t i = ds_ctr.fetch_add(1) % ds.size();
		ds_ids[i] = dataset_seed.get_id();
		std::thread thd(&hashpool::dataset_thd_main, this, dataset_seed, i);
		thd.detach();
	}

private:
	hashpool();

	void dataset_thd_main(v32 seed, size_t ds_idx);
	void hash_thd_main();

	std::array<uint64_t, 2> ds_ids{{0, 0}};
	std::array<dataset, 2> ds;
	std::atomic<size_t> ds_ctr;

	thdq<check_job*> jobs;
	std::vector<std::thread> threads;
	
};
