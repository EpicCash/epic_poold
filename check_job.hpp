#pragma once

#include <inttypes.h>
#include <future>
#include "vector32.h"

struct check_job
{
	const uint8_t* data;
	size_t data_len;
	v32 hash;
	bool error;
};
