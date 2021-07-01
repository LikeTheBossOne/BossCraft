#include "GlobalEventManager.h"
#include "EventBase.h"
#include "IEventHandler.h"


std::queue<EventBase*> GlobalEventManager::_eventQueue = std::queue<EventBase*>();
std::unordered_map<EventType, std::unordered_set<IEventHandler*>> GlobalEventManager::_handlerMap
	= std::unordered_map<EventType, std::unordered_set<IEventHandler*>>();
std::mutex GlobalEventManager::_queueMutex;
std::mutex GlobalEventManager::_mapMutex;

void GlobalEventManager::Init()
{
	_mapMutex.lock();
	for (unsigned int i = 0; i < static_cast<unsigned int>(EventType::Size); i++)
	{
		_handlerMap[static_cast<EventType>(i)] = std::unordered_set<IEventHandler*>();
	}
	_mapMutex.unlock();
}

void GlobalEventManager::SubscribeToEvent(IEventHandler* handler, EventType type)
{
	_mapMutex.lock();
	_handlerMap[type].insert(handler);
	_mapMutex.unlock();
}

void GlobalEventManager::UnsubscribeToEvent(IEventHandler* handler, EventType type)
{
}

void GlobalEventManager::RaiseEvent(EventBase* e)
{
	_queueMutex.lock();
	_eventQueue.push(e);
	_queueMutex.unlock();
}

void GlobalEventManager::ProcessEvents()
{
	while (!_eventQueue.empty())
	{
		_queueMutex.lock();
		EventBase* e = _eventQueue.front();
		_eventQueue.pop();
		_queueMutex.unlock();

		_mapMutex.lock();
		auto handlers = _handlerMap[e->_eventType];
		for (auto iter = handlers.begin(); iter != handlers.end(); ++iter)
		{
			(*iter)->HandleEvent(e);
		}
		_mapMutex.unlock();
	}
}
