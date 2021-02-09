#pragma once

#include "log.hpp"
#include "socks.h"
#include "time.hpp"

#include <assert.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

// Memory-holder object. We will allocate memory only in blocks of pool size, not for every single peer
template<typename T>
struct placement_mem
{
	inline placement_mem() : mem{0}, ptr(nullptr) {}
	inline ~placement_mem()
	{
		if(ptr != nullptr)
			ptr->~T();
	}

	placement_mem(const placement_mem& r) = delete;
	placement_mem& operator=(const placement_mem& r) = delete;
	placement_mem(placement_mem&& r) = delete;
	placement_mem& operator=(placement_mem&& r) = delete;

	template<typename... Args>
	inline void construct(Args&&... args)
	{
		if(ptr != nullptr)
			throw std::runtime_error("Constructing over live object");
		
		ptr = new(mem) T(std::forward<Args>(args)...);
	}

	inline void free()
	{
		if(ptr == nullptr)
			throw std::runtime_error("Destructing empty object");
		
		ptr->~T();
		ptr = nullptr;
	}

	inline T* get()
	{
		return ptr;
	}

private:
	uint8_t mem[sizeof(T)];
	T* ptr;
};

template <typename cli_type, size_t pool_size>
class client_pool
{
public:
	client_pool() : active_cnt(0), thd_finished(false)
	{
		if((epfd = epoll_create1(O_CLOEXEC)) == -1)
			throw std::runtime_error("Limit of files / epoll instances reached.");
		
		if(pipe2(pipefds, O_CLOEXEC | O_NONBLOCK) == -1)
			throw std::runtime_error("Limit of files / pipe memory reached.");
		
		epoll_event event = {0};
		event.data.u32 = uint32_t(-1);
		event.events = EPOLLIN | EPOLLET;
		
		if(epoll_ctl(epfd, EPOLL_CTL_ADD, pipefds[0], &event) == -1)
			throw std::runtime_error("Limit of max_user_watches reached.");
		
		my_thd = std::thread(&client_pool<cli_type, pool_size>::pool_main, this);
	}
	
	~client_pool()
	{
		if(my_thd.joinable())
			my_thd.join();

		close(epfd);
		close(pipefds[1]);
		close(pipefds[0]);
	}
	
	bool is_finished()
	{
		return thd_finished;
	}
	
	uint32_t get_active()
	{
		return active_cnt;
	}
	
	bool is_full()
	{
		return active_cnt == pool_size;
	}

	void add_socket(SOCKET fd, in6_addr& ip, in_port_t port)
	{
		pipe_msg msg = {0};
		msg.cli_fd = fd;
		msg.cli_ip = ip;
		msg.cli_port = port;

		active_cnt++;
		if(write(pipefds[1], &msg, sizeof(pipe_msg)) != sizeof(pipe_msg))
			throw std::runtime_error("Writing to pipe failed!");
	}
	
	void on_new_block()
	{
		pipe_msg msg = {0};
		msg.cli_fd = dummy_new_block_fd;

		if(write(pipefds[1], &msg, sizeof(pipe_msg)) != sizeof(pipe_msg))
			throw std::runtime_error("Writing to pipe failed!");
	}

private:
	static constexpr int dummy_new_block_fd = -1;

	struct pipe_msg
	{
		SOCKET cli_fd;
		in6_addr cli_ip;
		in_port_t cli_port;
	};

	void remove_client(uint32_t idx)
	{
		/* This can happen if we get multiple signals for a socket close */
		if(clients[idx].get() == nullptr)
			return;

		if(epoll_ctl(epfd, EPOLL_CTL_DEL, clients[idx].get()->get_fd(), nullptr) == -1)
			throw std::runtime_error("File descriptor double free");

		clients[idx].free();
		active_cnt--;
	}
	
	void pool_main()
	{
		epoll_event events[pool_size];
		do
		{
			int n = epoll_wait(epfd, events, pool_size, -1);

			if(n == -1)
			{
				if(errno != EINTR)
					throw std::runtime_error("Strange epoll error.");
				else
					continue;
			}

			for(int i = 0; i < n; i++)
			{
				uint32_t mev = events[i].events;
				if(events[i].data.u32 == uint32_t(-1))
				{
					process_pipe();
					continue;
				}

				uint32_t idx = events[i].data.u32;
				if(mev & EPOLLIN)
				{
					if(!clients[idx].get()->on_socket_read())
						remove_client(idx);
				}
				else /* EPOLLERR EPOLLHUP */
				{
					remove_client(idx);
				}
			}
		}
		while(active_cnt > 0);
		thd_finished = true;
	}
	
	void process_pipe()
	{
		while(true)
		{
			int rt;
			pipe_msg msg;
			
			if((rt = read(pipefds[0], &msg, sizeof(pipe_msg))) <= 0)
			{
				if(rt == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
					return;
				throw std::runtime_error("Reading pipe failed!");
			}
			
			size_t cli_id = 0;
			if(msg.cli_fd == dummy_new_block_fd)
			{ /* on_new_block */
				int64_t time_now_ms = get_timestamp_ms();
				for(; cli_id < pool_size; cli_id++)
				{
					if(clients[cli_id].get() == nullptr)
						continue;
					
					if(!clients[cli_id].get()->on_new_block(time_now_ms))
						remove_client(cli_id);
				}
			}
			else
			{ /* add_socket */
				epoll_event event = {0};
				
				for(; cli_id < pool_size; cli_id++)
				{
					if(clients[cli_id].get() == nullptr)
						break;
				}
				
				if(cli_id == pool_size)
					throw std::runtime_error("Pool size overflow");
				
				try
				{
					clients[cli_id].construct(msg.cli_fd, msg.cli_ip, msg.cli_port);
					event.data.u32 = cli_id;
					event.events = EPOLLIN | EPOLLET;
					
					if(epoll_ctl(epfd, EPOLL_CTL_ADD, msg.cli_fd, &event) == -1)
						throw std::runtime_error("Limit of max_user_watches reached.");
				}
				catch(const std::exception& e)
				{
					if(clients[cli_id].get() != nullptr)
						clients[cli_id].free();

					close(msg.cli_fd);
					logger::inst().err("Exception while constructing client: ", e.what());
					active_cnt--;
				}
			}
		}
	}

	placement_mem<cli_type> clients[pool_size];
	std::atomic<uint32_t> active_cnt;
	std::atomic<bool> thd_finished;
	int epfd;
	int pipefds[2];
	std::thread my_thd;
};
