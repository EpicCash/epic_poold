#pragma once

#include "vector32.h"
#include <inttypes.h>

enum class pow_type : uint32_t
{
	randomx,
	progpow,
	cuckoo
};

struct jobdata
{
	pow_type type;
	uint64_t block_diff;
	uint32_t height;
	uint32_t jobid;
	uint8_t prepow[384];
	uint32_t prepow_len;
	v32 rx_seed;
	v32 rx_next_seed;
};

typedef void (*on_new_job_callback)(const jobdata&);

