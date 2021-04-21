#include "randomx.h"
#include "hashpool.hpp"
#include <shared_mutex>
#include <string.h>

#include <cpuid.h>

inline bool has_hardware_aes()
{
	int32_t cpu_info[4] = {0};
	__cpuid_count(1, 0, cpu_info[0], cpu_info[1], cpu_info[2], cpu_info[3]);
	return (cpu_info[2] & (1 << 25)) != 0;
}

rx_hashpool::rx_hashpool() : ds_ctr(0)
{
	threads.reserve(hash_thd_count);
	for(size_t i = 0; i < hash_thd_count; i++)
		threads.emplace_back(&rx_hashpool::hash_thd_main, this);
}

void rx_hashpool::hash_thd_main()
{
	int fl = RANDOMX_FLAG_JIT;
	if(has_hardware_aes())
		fl |= RANDOMX_FLAG_HARD_AES;

	randomx_vm* v = randomx_create_vm((randomx_flags)fl, ds[0].ch, nullptr);
	while(true)
	{
		rx_check_job* job = jobs.pop();

		size_t dsidx;
		for(dsidx = 0; dsidx < ds.size(); dsidx++)
		{
			if(ds[dsidx].ready_seed_id == job->dataset_id)
				break;
		}

		if(dsidx == ds.size())
		{
			logger::inst().warn("Precalculated dataset for a job was not found! We lost a share!");
			job->hash.set_all_ones();
			job->error = true;
			job->ready.set_value();
			continue;
		}

		rx_dataset& nds = ds[dsidx];
		std::shared_lock<std::shared_timed_mutex> lk(nds.mtx);
		if(nds.ready_seed_id != job->dataset_id)
		{
			logger::inst().warn("Precalculated dataset was swapped out before we locked. We lost a share!");
			job->hash.set_all_ones();
			job->error = true;
			job->ready.set_value();
			continue;
		}

		randomx_vm_set_cache(v, nds.ch);
		randomx_calculate_hash(v, job->data, job->data_len, static_cast<uint8_t*>(job->hash));

		logger::inst().dbglo("Dataset id: ", nds.ready_seed_id.load(), "\nhash: ", job->hash);

		char blob[1024];
		bin2hex((uint8_t*)job->data, job->data_len, blob);
		logger::inst().dbglo("blob len: ", job->data_len, " blob: ", blob);

		job->error = false;
		job->ready.set_value();
	}
}

void rx_hashpool::dataset_thd_main(size_t ds_idx)
{
	rx_dataset& nds = ds[ds_idx];
	std::lock_guard<std::shared_timed_mutex> lk(nds.mtx);
	randomx_init_cache(nds.ch, static_cast<const uint8_t*>(nds.ds_seed), nds.ds_seed.size);
	nds.ready_seed_id = nds.ds_seed.get_id();
}
