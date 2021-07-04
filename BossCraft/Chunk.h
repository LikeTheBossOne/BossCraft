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
	unsigned int VAO, VBO, EBO;
	ChunkMesh* _mesh;
	
	World* _world;
	uint8_t _data[CHUNK_VOLUME];
public:
	glm::ivec2 _chunkPos;
	bool _isDirty;
	bool _meshIsLoaded;
	
	Chunk(glm::ivec2 chunkPos, World* owningWorld);
	~Chunk();

#pragma region Job Thread

	void LoadData();
	void GenerateMesh(std::array<std::shared_ptr<Chunk>, 4> neighbors, unsigned int outputIdx);
	
#pragma endregion

#pragma region Main Thread Only
	void GLLoad();
	void RenderMesh();

#pragma endregion
	

	unsigned int GetDataAtPosition(glm::vec3 pos);

private:
	glm::ivec3 AbsBlockPosToRelPos(glm::ivec3 blockPos);
	uint8_t GetNeighborBlockAtPos(glm::ivec3 pos, uint8_t* neighborData);
	unsigned int PositionToIndex(unsigned int posX, unsigned int posY, unsigned int posZ);
	unsigned int PositionToIndex(glm::ivec3 pos);
	glm::vec3 IndexToPosition(unsigned int index);
	bool BlockInChunkBounds(glm::ivec3 pos);
	void AddFaceToMesh(glm::vec3 blockPos, FaceDirection direction);
	void BufferMesh();
};

