#include "randomx.h"
#include "pp_hashpool.hpp"
#include <shared_mutex>
#include <string.h>

#include <cpuid.h>

pp_hashpool::pp_hashpool() : ds_ctr(0)
{
	threads.reserve(hash_thd_count);
	for(size_t i = 0; i < hash_thd_count; i++)
		threads.emplace_back(&pp_hashpool::hash_thd_main, this);
}

void pp_hashpool::hash_thd_main()
{
	while(true)
	{
		pp_check_job* job = jobs.pop();
		uint64_t job_epoch_number = ethash::get_epoch_number(job->block_number);

		size_t dsidx;
		for(dsidx = 0; dsidx < ds.size(); dsidx++)
		{
			if(ds[dsidx].ready_epoch == job_epoch_number)
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

		pp_dataset& nds = ds[dsidx];
		std::shared_lock<std::shared_timed_mutex> lk(nds.mtx);
		if(nds.ready_epoch != job_epoch_number)
		{
			logger::inst().warn("Precalculated dataset was swapped out before we locked. We lost a share!");
			job->hash.set_all_ones();
			job->error = true;
			job->ready.set_value();
			continue;
		}

		ethash_hash256 header_hash = ethash::keccak256(job->data, job->data_len);

		v32 hh = header_hash.bytes;
		logger::inst().dbglo("Nonce: ", job->nonce, "\nhh: ", hh, "\nbn: ", job->block_number);
		char blob[1024];
		bin2hex((uint8_t*)job->data, job->data_len, blob);
		logger::inst().dbglo("blob len: ", job->data_len, " blob: ", blob);

		job->res = progpow::hash(*nds.dataset_ptr, job->block_number, header_hash, job->nonce);
		job->hash = job->res.final_hash.bytes;

		job->error = false;
		job->ready.set_value();
	}
}

void pp_hashpool::dataset_thd_main(size_t ds_idx)
{
	pp_dataset& nds = ds[ds_idx];
	std::lock_guard<std::shared_timed_mutex> lk(nds.mtx);
	uint64_t epoch_number = nds.loaded_epoch.load();
	if(nds.dataset_ptr != nullptr)
		ethash_destroy_epoch_context(nds.dataset_ptr);
	nds.dataset_ptr = ethash_create_epoch_context(epoch_number);
	nds.ready_epoch = epoch_number;
}
