#pragma once
#include <mutex>
#include <atomic>
#include "itoa_ljust.h"
#include "vector32.h"
#include "encdec.h"
#include "loglevels.hpp"

class logger
{
public:
	inline static logger& inst()
	{
		static logger inst;
		return inst;
	};

	bool is_running() { return hFile != nullptr; }

	template<typename... Args>
	inline void err(Args... args)
	{
		log(log_level::error, get_default_colour(log_level::error), args...);
	}

	template<typename... Args>
	inline void warn(Args... args)
	{
		log(log_level::warn, get_default_colour(log_level::warn), args...);
	}

	template<typename... Args>
	inline void info(Args... args)
	{
		log(log_level::info, get_default_colour(log_level::info), args...);
	}

	template<typename... Args>
	inline void dbghi(Args... args)
	{
		log(log_level::debug_hi, get_default_colour(log_level::debug_hi), args...);
	}

	template<typename... Args>
	inline void dbglo(Args... args)
	{
		log(log_level::debug_lo, get_default_colour(log_level::debug_lo), args...);
	}

	template<typename... Args>
	inline void log(log_level lvl, out_colours clr, Args... args)
	{
		std::lock_guard<std::mutex> lck(log_lock);
		if(lvl < current_level)
			return;
		check_sighup();
		set_colour(clr);
		print_timestamp();
		print_all(args...);
		reset_colour();
	}

	void on_sighup()
	{
		sighup = true;
	}

private:
	inline void print_all()
	{
		print_element('\n');
		fflush(hFile);
	}

	template<typename Arg1, typename... Args>
	inline void print_all(Arg1& arg1, Args&... args)
	{
		print_element(arg1);
		print_all(args...);
	}

	inline void print_element(const char* str)
	{
		fputs(str, hFile);
	}

	inline void print_element(char c)
	{
		fputc(c, hFile);
	}

	inline void print_element(size_t arg)
	{
		char buf[32];
		itoa_ljust::itoa(arg, buf);
		print_element(buf);
	}

	inline void print_element(int64_t arg)
	{
		char buf[32];
		itoa_ljust::itoa(arg, buf);
		print_element(buf);
	}

	inline void print_element(uint32_t arg)
	{
		char buf[32];
		itoa_ljust::itoa(arg, buf);
		print_element(buf);
	}

	inline void print_element(int32_t arg)
	{
		char buf[32];
		itoa_ljust::itoa(arg, buf);
		print_element(buf);
	}

	inline void print_element(const v32& arg)
	{
		char buf[v32::size*2+1];
		bin2hex(arg.data, v32::size, buf);
		print_element(buf);
	}

	inline void check_sighup()
	{
		if(sighup)
			reopen_file();
	}

	inline void reset_colour()
	{
		if(hFile == stdout)
			fputs("\x1B[0m", stdout);
	}

	logger();
	void reopen_file();
	void print_timestamp();
	void set_colour(out_colours cl);

	FILE* hFile;
	log_level current_level;
	std::mutex log_lock;
	std::atomic<bool> sighup;
};
