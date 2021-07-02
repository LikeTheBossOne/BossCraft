#pragma once
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include "ChunkMesh.h"
#include <glm/mat4x4.hpp>
#include "FaceDirection.h"
#include <mutex>

class ChunkGenerator;
class Shader;
class Camera;
// Chunks are 16x128x16
const unsigned CHUNK_WIDTH = 16;
const unsigned CHUNK_HEIGHT = 64;
const unsigned int CHUNK_VOLUME = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH;

struct
{
	float data[(CHUNK_VOLUME) * 8 * 5 * sizeof(float)];
	uint16_t indices[(CHUNK_VOLUME) * 36 * sizeof(uint16_t)];
} buffers;

class World;

class Chunk
{
	friend class ChunkTaskManager;
	friend class World;
private:
	ChunkMesh* _mesh;
	std::mutex _meshMutex;
	
	World* _world;
	uint8_t _data[CHUNK_VOLUME];
public:
	glm::vec2 _chunkPos;
	bool _isDirty;
	bool _meshIsLoaded;
	
	Chunk(glm::vec2 chunkPos, World* owningWorld);
	~Chunk();

#pragma region Job Thread

	void LoadData();
	void GenerateMesh();
	
#pragma endregion

#pragma region Main Thread Only
	void RenderMesh();

#pragma endregion
	

	unsigned int GetDataAtPosition(glm::vec3 pos);

private:
	unsigned int PositionToIndex(unsigned int posX, unsigned int posY, unsigned int posZ);
	unsigned int PositionToIndex(glm::ivec3 pos);
	glm::vec3 IndexToPosition(unsigned int index);
	bool BlockInChunkBounds(glm::ivec3 pos);
	void AddFaceToMesh(glm::vec3 blockPos, FaceDirection direction);
	void BufferMesh();
};

