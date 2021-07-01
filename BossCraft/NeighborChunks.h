#pragma once
#include <cstdint>
#include "Chunk.h"

struct NeighborChunks
{
	uint8_t plusXData[CHUNK_VOLUME];
	uint8_t minusXData[CHUNK_VOLUME];
	uint8_t plusZData[CHUNK_VOLUME];
	uint8_t minusZData[CHUNK_VOLUME];
	bool plusXLoaded;
	bool minusXLoaded;
	bool plusZLoaded;
	bool minusZLoaded;
};
