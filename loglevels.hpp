#pragma once

#include <inttypes.h>

/* This needs to be separate to break cyclic dependency between logger and jconf */
enum class log_level : uint32_t
{
	error = 5,
	warn = 4,
	info = 3,
	debug_hi = 2,
	debug_lo = 1,
	invalid = 0
};

enum class out_colours : uint32_t
{
	K_RED = 7,
	K_GREEN = 6,
	K_BLUE = 5,
	K_YELLOW = 4,
	K_CYAN = 3,
	K_MAGENTA = 2,
	K_WHITE = 1,
	K_NONE = 0
};

inline out_colours get_default_colour(log_level lvl)
{
	switch(lvl)
	{
	case log_level::error:
		return out_colours::K_RED;
	case log_level::warn:
		return out_colours::K_YELLOW;
	case log_level::info:
	case log_level::debug_hi:
	case log_level::debug_lo:
	case log_level::invalid:
		return out_colours::K_NONE;
	}
}
