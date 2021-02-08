#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

template <typename T>
class thdq
{
public:
	T pop()
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		while(queue_.empty())
		{
			cond_.wait(mlock);
		}
		auto item = std::move(queue_.front());
		queue_.pop();
		return item;
	}

	void pop(T& item)
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		while(queue_.empty())
		{
			cond_.wait(mlock);
		}
		item = queue_.front();
		queue_.pop();
	}

	void push(const T& item)
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		queue_.push(item);
		mlock.unlock();
		cond_.notify_one();
	}

	void push(T&& item)
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		queue_.push(std::move(item));
		mlock.unlock();
		cond_.notify_one();
	}

private:
	std::queue<T> queue_;
	std::mutex mutex_;
	std::condition_variable cond_;
};

template <typename T>
class thdv
{
public:
	thdv() { vector_.reserve(1024); }

	void pop_all(std::vector<T>& out)
	{
		out.clear();
		std::unique_lock<std::mutex> mlock(mutex_);
		cond_.wait(mlock, [this] { return !vector_.empty(); });
		vector_.swap(out);
		vector_.reserve(1024);
		mlock.unlock();
	}

	template <class... Args>
	void emplace(Args&&... args)
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		vector_.emplace_back(args...);
		mlock.unlock();
		cond_.notify_one();
	}

private:
	std::vector<T> vector_;
	std::mutex mutex_;
	std::condition_variable cond_;
};
