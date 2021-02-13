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

hashpool::hashpool() : ds_ctr(0)
{
	threads.reserve(hash_thd_count);
	for(size_t i = 0; i < hash_thd_count; i++)
		threads.emplace_back(&hashpool::hash_thd_main, this);
}

void hashpool::hash_thd_main()
{
	randomx_flags fl = (randomx_flags)(RANDOMX_FLAG_FULL_MEM | RANDOMX_FLAG_JIT | RANDOMX_FLAG_HARD_AES);
	randomx_vm* v = randomx_create_vm(fl, nullptr, ds[0].ds);
	while(true)
	{
		check_job* job = jobs.pop();

		size_t dsidx;
		for(dsidx = 0; dsidx < ds.size(); dsidx++)
		{
			if(ds[dsidx].seed_id == job->dataset_id)
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

		std::shared_lock<std::shared_timed_mutex> lk(ds[dsidx].mtx);
		if(ds[dsidx].seed_id != job->dataset_id)
		{
			logger::inst().warn("Precalculated dataset was swapped out before we locked. We lost a share!");
			job->hash.set_all_ones();
			job->error = true;
			job->ready.set_value();
			continue;
		}

		dataset& mds = ds[dsidx];
		randomx_vm_set_dataset(v, mds.ds);
		randomx_calculate_hash(v, job->data, job->data_len, static_cast<uint8_t*>(job->hash));

		logger::inst().dbglo("Dataset id: ", mds.seed_id.load(), "\nhash: ", job->hash);

		char blob[1024];
		bin2hex((uint8_t*)job->data, job->data_len, blob);
		logger::inst().dbglo("blob len: ", job->data_len, " blob: ", blob);

		job->error = false;
		job->ready.set_value();
	}
}

void hashpool::dataset_thd_main(v32 seed, size_t ds_idx)
{
	dataset& nds = ds[ds_idx];
	std::lock_guard<std::shared_timed_mutex> lk(nds.mtx);

	nds.ds_seed = seed;
	nds.seed_id = seed.get_id();

	logger::inst().info("Calculating dataset 0x", nds.seed_id.load(), " - ", seed);

	nds.init_cache();

	std::vector<std::thread> thd;
	size_t thdcnt = std::thread::hardware_concurrency();
	thd.reserve(thdcnt-1);

	for(size_t i=1; i < thdcnt; i++)
		thd.emplace_back(&dataset::calculate, &nds, i, thdcnt);

	nds.calculate(0, thdcnt);

	for(auto& t : thd)
		t.join();

	logger::inst().info("Calculating dataset 0x", nds.seed_id.load());
}
