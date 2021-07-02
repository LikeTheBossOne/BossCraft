#pragma once
#include <deque>
#include <future>
#include <queue>
#include <vector>

class ThreadPool
{
	std::vector<std::thread> _threads;
	
	std::condition_variable _eventVar;
	std::mutex _eventMutex;
	bool _stopping = false;
	std::queue<std::function<void(unsigned int)>> _tasks;
public:
	ThreadPool(size_t numThreads);
	~ThreadPool();
	
	void Start(std::size_t numThreads);
	void Stop();
	
	void Enqueue(std::function<void(unsigned int)> task);
};

