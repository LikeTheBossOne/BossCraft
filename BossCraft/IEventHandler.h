#pragma once

class EventBase;

class IEventHandler
{
public:
	virtual void HandleEvent(EventBase* e) = 0;
};
