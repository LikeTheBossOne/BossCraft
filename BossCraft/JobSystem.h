#pragma once
#include <functional>

#include "ConcurrentRingBuffer.h"

// A Dispatched job will receive this as function argument:
struct JobDispatchArgs
{
	uint32_t jobIndex;
	uint32_t groupIndex;
};

class JobSystem
{
private:
	static unsigned int _numThreads;
	static ConcurrentRingBuffer<std::function<void()>, 256> _jobPool;
	static std::condition_variable _wakeCondition;
	static std::mutex _wakeMutex;
	static uint64_t _currentLabel;
	static std::atomic<uint64_t> _finishedLabel;
	
public:
    // Create the internal resources such as worker threads, etc. Call it once when initializing the application.
    static void Init();

	// Add a job to execute asynchronously. Any idle thread will execute this job.
	static void Execute(const std::function<void()>& job);

	// Divide a job onto multiple jobs and execute in parallel.
	//	jobCount	: how many jobs to generate for this task.
	//	groupSize	: how many jobs to execute per thread. Jobs inside a group execute serially. It might be worth to increase for small jobs
	//	func		: receives a JobDispatchArgs as parameter
	static void Dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& job);

	// Check if any threads are working currently or not
	static bool IsBusy();

	// Wait until all threads become idle
	static void Wait();

private:
	static void Poll();
};

