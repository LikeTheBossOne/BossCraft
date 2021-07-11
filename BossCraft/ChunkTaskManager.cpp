#include "ChunkTaskManager.h"

#include "Chunk.h"

ChunkTaskManager::ChunkTaskManager(World* owningWorld) : _owningWorld(owningWorld)
{
	_threadPool = new ThreadPool(2);
}

void ChunkTaskManager::AddDataGenTask(glm::ivec2 chunkPosToCreate)
{
	
}

void ChunkTaskManager::AddDataUpdateTask(std::shared_ptr<Chunk> chunk)
{
}

void ChunkTaskManager::AddMeshGenTask(std::shared_ptr<Chunk> chunk, std::array<std::shared_ptr<Chunk>, 4> neighbors)
{
	
}

unsigned ChunkTaskManager::PositionToIndex(glm::ivec3 pos)
{
	return pos.x * CHUNK_HEIGHT * CHUNK_WIDTH + pos.y * CHUNK_WIDTH + pos.z;
}

glm::ivec3 ChunkTaskManager::AbsBlockPosToRelPos(glm::ivec3 blockPos)
{
	return glm::ivec3(blockPos.x % CHUNK_WIDTH, blockPos.y, blockPos.z % CHUNK_WIDTH);
}

uint8_t ChunkTaskManager::GetNeighborBlockAtPos(glm::ivec3 pos, uint8_t* neighborData)
{
	return neighborData[PositionToIndex(pos)];
}
