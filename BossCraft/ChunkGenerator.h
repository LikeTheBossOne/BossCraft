#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <unordered_map>

class World;
class Chunk;
struct NeighborChunks;

class ChunkGenerator
{
public:
	Chunk** GenerateChunks(glm::ivec2 centerChunk, uint8_t renderDistance, World* world);
	void LoadChunksData(glm::ivec2 centerChunk, uint8_t drawRadius, World* world);
	void GenerateChunksMeshes(glm::ivec2 centerChunk, uint8_t drawRadius, World* world);
	void LoadSingleChunkData(glm::ivec2 chunkPos);
	void GenerateChunk(Chunk* chunk, NeighborChunks* neighbors);

private:
	glm::ivec3 AbsBlockPosToRelPos(glm::ivec3 blockPos);
	unsigned int PositionToIndex(unsigned int posX, unsigned int posY, unsigned int posZ);
	unsigned int PositionToIndex(glm::ivec3 pos);
	uint8_t GetNeighborBlockAtPos(glm::ivec3 pos, uint8_t* neighborData);
};

