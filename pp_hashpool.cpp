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
