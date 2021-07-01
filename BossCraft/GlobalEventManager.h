#pragma once
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

enum class EventType;
class EventBase;
class IEventHandler;

/**
 * Handles multi-cast events. Events should be things that happened already, not instructions to do something.
 */
class GlobalEventManager
{
private:
	static std::queue<EventBase*> _eventQueue;
	static std::unordered_map <EventType, std::unordered_set<IEventHandler*>> _handlerMap;
	static std::mutex _queueMutex;
	static std::mutex _mapMutex;
	
public:
	static void Init();
	
	static void SubscribeToEvent(IEventHandler* handler, EventType type);
	static void UnsubscribeToEvent(IEventHandler* handler, EventType type);
	static void RaiseEvent(EventBase* e);
	static void ProcessEvents();
};
