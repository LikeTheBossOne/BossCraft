#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads)
{
	Start(numThreads);
}

ThreadPool::~ThreadPool()
{
	Stop();
}

void ThreadPool::Start(std::size_t numThreads)
{
	for (unsigned int i = 0; i < numThreads; i++)
	{
		_threads.emplace_back([=]
			{
				while (true)
				{
					std::function<void(unsigned int)> task;
					
					{
						std::unique_lock<std::mutex> lock(_eventMutex);

						_eventVar.wait(lock, [=] { return _stopping || !_tasks.empty(); });

						if (_stopping && _tasks.empty())
						{
							break;
						}

						task = std::move(_tasks.front());
						_tasks.pop();
					}
					
					task(i);
				}
			});
	}
}

void ThreadPool::Stop()
{
	{
		std::unique_lock<std::mutex> lock(_eventMutex);
		_stopping = true;
	}
		
	_eventVar.notify_all();
	for (auto& thread : _threads)
	{
		thread.join();
	}
}

void ThreadPool::Enqueue(std::function<void(unsigned int)> task)
{
	{
		std::unique_lock<std::mutex> lock(_eventMutex);
		_tasks.emplace(std::move(task));
	}
	
	_eventVar.notify_one();
}
