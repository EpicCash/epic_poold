#pragma once
#include <mutex>
#include <atomic>
#include "itoa_ljust.h"

class logger
{
public:
	inline static logger& inst()
	{
		static logger inst;
		return inst;
	};

	bool is_running() { return hFile != nullptr; }
	void log_msg(const char* fmt, ...);
	void log_msg_short(const char* msg);
	void log_msg_long(const char* long_msg, const char* fmt, ...);
	
	template<typename... Args>
	inline void log(Args... args)
	{
		std::lock_guard<std::mutex> lck(log_lock);
		check_sighup();
		print_timestamp();
		print_all(args...);
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

	inline void check_sighup()
	{
		if(sighup)
			reopen_file();
	}

	logger();
	void reopen_file();
	void print_timestamp();
	static logger* pInst;

	FILE* hFile;
	std::mutex log_lock;
	std::mutex print_lock;
	std::atomic<bool> sighup;
};
