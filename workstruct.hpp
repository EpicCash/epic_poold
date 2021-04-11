#pragma once

#include "vector32.h"
#include <inttypes.h>

enum class pow_type : uint32_t
{
	randomx,
	progpow,
	cuckoo
};

inline const char* pow_type_to_str(pow_type type)
{
	switch(type)
	{
	case pow_type::randomx:
		return "randomx";
	case pow_type::progpow:
		return "progpow";
	case pow_type::cuckoo:
		return "cuckoo";
	}
}

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

