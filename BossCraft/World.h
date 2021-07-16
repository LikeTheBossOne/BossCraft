#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <array>
#include <FastNoiseLite.h>
#include <queue>

#include "glm/gtx/hash.hpp"
#include "Shader.h"
#include <unordered_map>

#include "ConcurrentRingBuffer.h"
#include "IEventHandler.h"

class Player;
class TextureAtlas;
struct ChunkMesh;
class ChunkTaskManager;
class Chunk;
class ChunkGenerator;
class Camera;

class World : public IEventHandler
{
private:
	static const size_t _maxJobs = 1;
	Player* _player;
	Shader* _shader;

	std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>> _chunks;
	

	uint8_t _renderDistance;
	uint8_t _extraLoadDistance;
	glm::ivec2 _chunkOrigin;
	glm::ivec2 _centerChunk;

public:
	FastNoiseLite* _noiseGenerator;
	ConcurrentRingBuffer<std::shared_ptr<Chunk>, _maxJobs * 16> _dataGenOutput{};
	ConcurrentRingBuffer<std::pair<glm::ivec2, ChunkMesh*>*, _maxJobs * 16> _meshGenOutput{};
	ConcurrentRingBuffer<std::array<unsigned int, 3>*, _maxJobs * 16> _chunkUnload{};
	std::queue<glm::ivec2> _chunksToLoad;
	std::queue<glm::ivec2> _chunksToGenMesh;
	
	TextureAtlas* _textureAtlas;

	World(Shader* shader, TextureAtlas* atlas, Player* player);

	void SetCenter(glm::vec3 blockPos);
	void UpdateBlockAtPos(glm::ivec3 blockPos, uint8_t newBlock);
	
	void Update(float dt);
	
	void Render();

	uint8_t GetBlockAtAbsPos(glm::ivec3 blockPos);
	bool BlockInRenderDistance(glm::ivec3 blockPos);
	glm::ivec2 BlockPosToAbsChunkPos(glm::ivec3 blockPos);

	Player* GetPlayer();
	Camera* GetCamera();
	Shader* GetShader();
private:
	void Init();
	
	void LoadNewChunks();
	void CreateLoadChunksTasks();
	void CreateGenMeshTasks();
	bool CreateSingleGenMeshTask(glm::ivec2 pos);
	
	unsigned int BlockPosToRelChunkIndex(glm::ivec3 blockPos);
	unsigned int AbsChunkPosToRelIndex(glm::ivec2 chunkPos);
	unsigned int RelChunkPosToRelIndex(glm::ivec2 chunkPos);
	glm::ivec3 AbsBlockPosToChunkBlockPos(glm::ivec3 absBlockPos);
	bool ChunkInRenderDistance(glm::ivec2 chunkPos);
	bool ChunkInLoadDistance(glm::ivec2 chunkPos);
	glm::ivec2 RelChunkIndexToAbsChunkPos(unsigned int index);
	
public:
	void HandleEvent(EventBase* e) override;
};
