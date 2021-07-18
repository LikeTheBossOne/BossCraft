#pragma once
#include <array>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
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

class World;
struct ChunkMesh;

class Chunk
{
	friend class World;
private:
	unsigned int VAO, VBO, EBO;
	unsigned int _indexCount;
	ChunkMesh* _mesh;
	
	World* _world;
	std::array<uint8_t, CHUNK_VOLUME> _data;
public:
	glm::ivec2 _chunkPos;
	bool _isDirty;
	bool _meshIsLoaded;
	
	Chunk(glm::ivec2 chunkPos, World* owningWorld);
	Chunk(Chunk& other);
	~Chunk();

#pragma region Job Thread

	void SetData(glm::ivec3 blockPos, uint8_t blockType);
	void LoadData();
	ChunkMesh* GenerateMesh(std::array<std::shared_ptr<Chunk>, 4> neighbors);
	
#pragma endregion

#pragma region Main Thread Only
	void GLLoad();
	void GLUnload();
	void RenderMesh(Shader* shader);

#pragma endregion

	unsigned int GetDataAtPosition(glm::vec3 pos);

private:
	glm::ivec3 AbsBlockPosToRelPos(glm::ivec3 blockPos);
	uint8_t GetNeighborBlockAtPos(glm::ivec3 pos, uint8_t* neighborData);
	unsigned int PositionToIndex(unsigned int posX, unsigned int posY, unsigned int posZ);
	unsigned int PositionToIndex(glm::ivec3 pos);
	glm::vec3 IndexToPosition(unsigned int index);
	bool BlockInChunkBounds(glm::ivec3 pos);
	void AddFaceToMesh(glm::vec3 blockPos, FaceDirection direction, ChunkMesh* mesh);
	void BufferMesh();
};

