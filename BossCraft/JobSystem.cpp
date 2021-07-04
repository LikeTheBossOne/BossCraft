#include "JobSystem.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <windows.h>

unsigned int JobSystem::_numThreads = 0;
ConcurrentRingBuffer<std::function<void()>, 256> JobSystem::_jobPool;
std::condition_variable JobSystem::_wakeCondition;
std::mutex JobSystem::_wakeMutex;
uint64_t JobSystem::_currentLabel = 0;
std::atomic<uint64_t> JobSystem::_finishedLabel;

void JobSystem::Init()
{
	_finishedLabel.store(0);
	unsigned int  numCores = std::thread::hardware_concurrency();
	_numThreads = max(1u, numCores);

	// Create all our worker threads while immediately starting them:
	for (uint32_t threadID = 0; threadID < _numThreads; ++threadID)
	{
		std::thread worker([] {

			std::function<void()> job; // the current job for the thread, it's empty at start.

									   // This is the infinite loop that a worker thread will do 
			while (true)
			{
				if (_jobPool.Dequeue(job)) // try to grab a job from the jobPool queue
				{
					// It found a job, execute it:
					job(); // execute job
					_finishedLabel.fetch_add(1); // update worker label state
				}
				else
				{
					// no job, put thread to sleep
					std::unique_lock<std::mutex> lock(_wakeMutex);
					_wakeCondition.wait(lock);
				}
			}

			});

#ifdef _WIN32
		// Do Windows-specific thread setup:
		HANDLE handle = (HANDLE)worker.native_handle();

		// Put each thread on to dedicated core
		DWORD_PTR affinityMask = 1ull << threadID;
		DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
		assert(affinity_result > 0);

		// Increase thread priority:
		BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
		assert(priority_result != 0);

		// Name the thread:
		std::wstringstream wss;
		wss << "JobSystem_" << threadID;
		HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
		assert(SUCCEEDED(hr));
#endif // _WIN32

		worker.detach(); // forget about this thread, let it do it's job in the infinite loop that we created above
	}
}

void JobSystem::Execute(const std::function<void()>& job)
{
	_currentLabel++;
	
	while (!_jobPool.Enqueue(job))
	{
		Poll();
	}

	_wakeCondition.notify_one();
}

void JobSystem::Dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& job)
{
	if (jobCount == 0 || groupSize == 0)
	{
		return;
	}

	// Calculate the amount of job groups to dispatch (overestimate, or "ceil"):
	const uint32_t groupCount = (jobCount + groupSize - 1) / groupSize;

	// The main thread label state is updated:
	_currentLabel += groupCount;

	for (uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
	{
		// For each group, generate one real job:
		const auto& jobGroup = [jobCount, groupSize, job, groupIndex]() {

			// Calculate the current group's offset into the jobs:
			const uint32_t groupJobOffset = groupIndex * groupSize;
			const uint32_t groupJobEnd = min(groupJobOffset + groupSize, jobCount);

			JobDispatchArgs args;
			args.groupIndex = groupIndex;

			// Inside the group, loop through all job indices and execute job for each index:
			for (uint32_t i = groupJobOffset; i < groupJobEnd; ++i)
			{
				args.jobIndex = i;
				job(args);
			}
		};

		// Try to push a new job until it is pushed successfully:
		while (!_jobPool.Enqueue(jobGroup)) { Poll(); }

		_wakeCondition.notify_one(); // wake one thread
	}
}

bool JobSystem::IsBusy()
{
	return _finishedLabel.load() < _currentLabel;
}

void JobSystem::Wait()
{
	while (IsBusy())
	{
		Poll();
	}
}

void JobSystem::Poll()
{
	_wakeCondition.notify_one();
	std::this_thread::yield();
}
