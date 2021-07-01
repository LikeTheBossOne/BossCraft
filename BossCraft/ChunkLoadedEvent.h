#pragma once
#include "EventBase.h"
#include "Chunk.h"

class ChunkLoadedEvent : public EventBase
{
public:
	Chunk* _loadedChunk;

	ChunkLoadedEvent(Chunk* loadedChunk) : _loadedChunk(loadedChunk)
	{
		_eventType = EventType::ChunkLoaded;
	}
};
