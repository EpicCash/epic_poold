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
#include <chrono>
#include <unistd.h>

inline int64_t get_walltime()
{
	using namespace std::chrono;
	return time_point_cast<seconds>(system_clock::now()).time_since_epoch().count();
}

inline uint64_t get_timestamp_ms()
{
	using namespace std::chrono;
	return time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch().count();
}

inline int64_t get_timestamp()
{
	using namespace std::chrono;
	return time_point_cast<seconds>(steady_clock::now()).time_since_epoch().count();
}

/* our db epoch is 01/01/2010 */
inline int64_t db_time_to_walltime(int32_t db_time) { return int64_t(db_time) + 1262304000ll; }
inline int32_t walltime_to_db_time(int64_t walltime) { return int32_t(walltime - 1262304000ll); }

/* Handle wakeups while sleeping */
inline void unix_sleep(int time_s)
{
	while((time_s = sleep(time_s)) != 0);
}
