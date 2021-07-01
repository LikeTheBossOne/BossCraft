#pragma once

enum class EventType
{
	ChunkLoaded = 0,
	Size,
};

class EventBase
{
public:
	EventType _eventType;
};