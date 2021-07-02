#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <array>

#include "glm/gtx/hash.hpp"
#include "Shader.h"
#include <unordered_map>
#include "IEventHandler.h"

class ChunkTaskManager;
class Chunk;
class ChunkGenerator;
class Camera;

class World : public IEventHandler
{
private:
	const uint16_t MaxNewChunksPerFrame = 16;

	Camera* _mainCamera;
	Shader* _shader;

	std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>> _chunks;
	ChunkTaskManager* _chunkTaskManager;

	uint8_t _renderDistance;
	glm::ivec2 _chunkOrigin;
	glm::ivec2 _centerChunk;

public:
	std::array<std::shared_ptr<Chunk>, 20> _dataGenOutput;
	std::array<std::shared_ptr<Chunk>, 20> _meshGenOutput;
	
	unsigned int _textureID;

	World(Shader* shader, unsigned int textureID, Camera* mainCamera);
	World(Shader* shader, unsigned int textureID);

	void SetCenter(glm::vec3 blockPos);
	void Update(float dt);
	
	void Render();

	uint8_t GetBlockAtAbsPos(glm::ivec3 blockPos);
	bool BlockInRenderDistance(glm::ivec3 blockPos);
	glm::ivec2 BlockPosToAbsChunkPos(glm::ivec3 blockPos);

	Camera* GetCamera();
	Shader* GetShader();
private:
	void Init(Camera* mainCamera);
	
	void LoadNewChunks();

	unsigned int BlockPosToRelChunkIndex(glm::ivec3 blockPos);
	unsigned int AbsChunkPosToRelIndex(glm::ivec2 chunkPos);
	unsigned int RelChunkPosToRelIndex(glm::ivec2 chunkPos);
	bool ChunkInRenderDistance(glm::ivec2 chunkPos);
	glm::ivec2 RelChunkIndexToAbsChunkPos(unsigned int index);
	
public:
	void HandleEvent(EventBase* e) override;
};
