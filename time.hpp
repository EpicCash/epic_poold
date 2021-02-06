#pragma once
#include <chrono>

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
