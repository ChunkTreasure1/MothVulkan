#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue() {}
	ThreadSafeQueue(const ThreadSafeQueue& copy)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue = copy.m_queue;
	}

	void Push(T val)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push(val);
		m_conditionVariable.notify_one();
	}

	void WaitAndPop(T& val)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_conditionVariable.wait([&m_queue]() { return !m_queue.empty(); });

		val = m_queue.front();
		m_queue.pop();
	}

	T WaitAndPop()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_conditionVariable.wait([&m_queue]() { return !m_queue.empty(); });

		T val = m_queue.front();
		m_queue.pop();

		return val;
	}

	bool TryPop(T& val)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_queue.empty())
		{
			return false;
		}

		val = m_queue.front();
		m_queue.pop();

		return true;
	}

	T TryPop()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_queue.empty())
		{
			return T();
		}

		T val = m_queue.front();
		m_queue.pop();

		return val;
	}

	bool Empty()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_queue.empty();
	}

	uint32_t Size()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return static_cast<uint32_t>(m_queue.size());
	}

private:
	std::mutex m_mutex;
	std::queue<T> m_queue;
	std::condition_variable m_conditionVariable;
};
