#pragma once
#include <mutex>

template <typename T, size_t capacity>
class ConcurrentRingBuffer
{
private:
	T _data[capacity];
	size_t _head;
	size_t _tail;
	std::mutex _lock;

public:
	ConcurrentRingBuffer()
	{
		_head = 0;
		_tail = 0;
	}
	
	bool Enqueue(const T& item)
	{
		bool result = false;
		_lock.lock();
		size_t next = (_head + 1) % capacity;
		if (next != _tail)
		{
			_data[_head] = item;
			_head = next;
			result = true;
		}
		_lock.unlock();
		return result;
	}
	
	bool Dequeue(T& item)
	{
		bool result = false;
		_lock.lock();
		if (_tail != _head)
		{
			item = _data[_tail];
			_data[_tail] = NULL;
			_tail = (_tail + 1) % capacity;
			result = true;
		}
		_lock.unlock();
		return result;
	}

	bool Empty()
	{
		return _head == _tail;
	}
};
