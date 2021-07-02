#pragma once
#include <glm/vec2.hpp>

#include "ThreadPool.h"
#include "World.h"

class ChunkTaskManager
{
private:
	ThreadPool* _threadPool;
	World* _owningWorld;
public:
	ChunkTaskManager(World* owningWorld);

	void AddDataGenTask(std::shared_ptr<Chunk> chunkToCreate);
	void AddDataUpdateTask(std::shared_ptr<Chunk> chunk);
	/**
	 * neighbors order = +x, -x, +z, -z
	 */
	void AddMeshGenTask(std::shared_ptr<Chunk> chunk, std::array<std::shared_ptr<Chunk>, 4> neighbors);

private:
	unsigned int PositionToIndex(glm::ivec3 pos);
	glm::ivec3 AbsBlockPosToRelPos(glm::ivec3 blockPos);
	uint8_t GetNeighborBlockAtPos(glm::ivec3 pos, uint8_t* neighborData);
};

