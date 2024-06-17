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
