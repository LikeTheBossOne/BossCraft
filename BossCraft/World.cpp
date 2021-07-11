#include "World.h"
#include "Chunk.h"
#include "Camera.h"
#include "EventBase.h"
#include "ChunkLoadedEvent.h"
#include "ChunkMesh.h"
#include "ChunkTaskManager.h"
#include "NeighborChunks.h"
#include "GlobalEventManager.h"
#include "JobSystem.h"


World::World(Shader* shader, unsigned int textureID) : _shader(shader), _textureID(textureID)
{
	Init(new Camera(this, glm::vec3(0, 65, 0)));
}

World::World(Shader* shader, unsigned int textureID, Camera* mainCamera) : _shader(shader), _textureID(textureID)
{
	Init(mainCamera);
}

void World::Init(Camera* mainCamera)
{
	_mainCamera = mainCamera;

	_centerChunk = glm::ivec2(0, 0);
	_renderDistance = 10;
	_extraLoadDistance = 1;
	_chunkOrigin = glm::ivec2(-_renderDistance, -_renderDistance);

	unsigned int totalChunks = ((2 * _renderDistance) + 1) * ((2 * _renderDistance) + 1);
	_chunks = std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>>();

	_noiseGenerator = new FastNoiseLite;
	_chunksToLoad = std::queue<glm::ivec2>();
	_chunksToGenMesh = std::queue<glm::ivec2>();


	_shader->Use();
	glm::mat4 projection = glm::perspective(glm::radians(_mainCamera->_fov), 800.f / 600.f, 0.1f, 300.0f);
	_shader->UniSetMat4f("projection", projection);

	
	LoadNewChunks();
}

void World::SetCenter(glm::vec3 blockPos)
{
	glm::ivec2 newCenterChunk = BlockPosToAbsChunkPos(blockPos);

	if (newCenterChunk == _centerChunk)
	{
		return;
	}
	
	glm::ivec2 offset = newCenterChunk - _centerChunk;
	glm::ivec2 oldOrigin = _chunkOrigin;
	_chunkOrigin += offset;
	_centerChunk = newCenterChunk;

	auto it = _chunks.begin();
	
	while (it != _chunks.end())
	{
		if (!ChunkInLoadDistance(it->first))
		{
			//it->second->GLUnload();
			_chunks[it->first] = NULL;
			it = _chunks.erase(it);
			//_chunks[chunkPos] = NULL;
		}
		else
		{
			it++;
		}
	}
	LoadNewChunks();
}

void World::Update(float dt)
{
	// First Check output arrays for new data
	std::shared_ptr<Chunk> outChunk;
	while (_dataGenOutput.Dequeue(outChunk))
	{
		_chunks[outChunk->_chunkPos] = outChunk;
		outChunk->GLLoad();
		_chunksToGenMesh.emplace(outChunk->_chunkPos);
	}

	std::pair<glm::ivec2, ChunkMesh*>* outMesh;
	while (_meshGenOutput.Dequeue(outMesh))
	{
		glm::ivec2 pos = outMesh->first;
		std::shared_ptr<Chunk> chunk = nullptr;
		if (_chunks.find(pos) != _chunks.end() && (chunk = _chunks[pos]) != NULL)
		{
			delete chunk->_mesh;

			chunk->_mesh = outMesh->second;
			chunk->BufferMesh();
		}
	}

	std::array<unsigned int, 3>* outBuffers;
	while (_chunkUnload.Dequeue(outBuffers))
	{
		// Delete VAO
		glDeleteVertexArrays(1, &(*outBuffers)[0]);
		glDeleteBuffers(1, &(*outBuffers)[1]);
		glDeleteBuffers(1, &(*outBuffers)[2]);
	}

	CreateGenMeshTasks();
	CreateLoadChunksTasks();
	Render();
}

void World::Render()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _textureID);

	
	_shader->Use();
	//_shader->UniSetMat4f("view", _mainCamera->GetViewMatrix());
	_shader->SetViewMatrix(_mainCamera->GetViewMatrix());
	
	
	for (int x = _chunkOrigin[0]; x < _chunkOrigin[0] + (_renderDistance * 2) + 1; x++)
	{
		for (int z = _chunkOrigin[1]; z < _chunkOrigin[1] + (_renderDistance * 2) + 1; z++)
		{
			glm::ivec2 pos = glm::ivec2(x, z);
			std::shared_ptr<Chunk> chunk = NULL;
			if (_chunks.find(pos) != _chunks.end() && (chunk = _chunks[pos]) != NULL)
			{
				chunk->RenderMesh(_shader);
			}
		}
	}
}

uint8_t World::GetBlockAtAbsPos(glm::ivec3 blockPos)
{
	if (blockPos.y < 0 || blockPos.y >= CHUNK_HEIGHT) return 0;
	
	// Assumes that block is inside of render distance!
	glm::ivec2 chunkPos = BlockPosToAbsChunkPos(blockPos);

	// Get block's position inside of the chunk
	std::shared_ptr<Chunk> chunk = _chunks[chunkPos];
	glm::vec3 pos = glm::vec3(blockPos.x - (chunkPos[0] * CHUNK_WIDTH), blockPos.y, blockPos.z - (chunkPos[1] * CHUNK_WIDTH));

	//if (pos.x <= 0 || pos.x >= CHUNK_WIDTH || pos.z <= 0 || pos.z >= CHUNK_WIDTH) return 0;
	
	return chunk->GetDataAtPosition(pos);
}

Camera* World::GetCamera()
{
	return _mainCamera;
}

Shader* World::GetShader()
{
	return _shader;
}

void World::LoadNewChunks()
{
	for (int x = _chunkOrigin[0] - _extraLoadDistance; x < _chunkOrigin[0] + (_renderDistance * 2) + 1 + _extraLoadDistance; x++)
	{
		for (int z = _chunkOrigin[1] - _extraLoadDistance; z < _chunkOrigin[1] + (_renderDistance * 2) + 1 + _extraLoadDistance; z++)
		{
			glm::ivec2 pos = glm::ivec2(x, z);
			std::shared_ptr<Chunk> chunk;
			if (_chunks.find(pos) == _chunks.end())
			{
				_chunks[pos] = NULL;
				_chunksToLoad.push(pos);
			}
		}
	}
}

void World::CreateLoadChunksTasks()
{
	size_t count = 0;
	while (count < _maxJobs && !_chunksToLoad.empty())
	{
		glm::ivec2 pos = _chunksToLoad.front();
		_chunksToLoad.pop();

		JobSystem::Execute([this, pos]
			{
				std::shared_ptr<Chunk> chunkToCreate = std::make_shared<Chunk>(pos, this);
				chunkToCreate->LoadData();
				this->_dataGenOutput.Enqueue(chunkToCreate);
				
			});
		
		count++;
	}
}

void World::CreateGenMeshTasks()
{
	std::vector<glm::ivec2> outsideRange;
	while (!_chunksToGenMesh.empty())
	{
		glm::ivec2 pos = _chunksToGenMesh.front();
		_chunksToGenMesh.pop();

		if (!ChunkInRenderDistance(pos))
		{
			outsideRange.emplace_back(pos);
			continue;
		}
		
		if (_chunks.find(pos) != _chunks.end())
		{
			if (!CreateSingleGenMeshTask(pos))
			{
				outsideRange.emplace_back(pos);
			}
		}
	}

	for (auto pos : outsideRange)
	{
		_chunksToGenMesh.emplace(pos);
	}
}

bool World::CreateSingleGenMeshTask(glm::ivec2 pos)
{
	std::array<std::shared_ptr<Chunk>, 4> neighbors{};
	std::array<glm::ivec2, 4> poses = {
		glm::ivec2(pos[0] + 1, pos[1]),
		glm::ivec2(pos[0] - 1, pos[1]),
		glm::ivec2(pos[0], pos[1] + 1),
		glm::ivec2(pos[0], pos[1] - 1)
	};
	for (unsigned int posIdx = 0; posIdx < 4; posIdx++)
	{
		glm::ivec2 pos = poses[posIdx];
		if (_chunks.find(pos) != _chunks.end() && _chunks[pos] != NULL)
		{
			neighbors[posIdx] = _chunks[pos];
		}
		else
		{
			return false;
		}
	}

	std::shared_ptr<Chunk> chunk = _chunks[pos];
	JobSystem::Execute([chunk, neighbors] {chunk->GenerateMesh(neighbors); });
	return true;
}

unsigned World::AbsChunkPosToRelIndex(glm::ivec2 chunkPos)
{
	glm::ivec2 relChunkPos = chunkPos - /*-*/_centerChunk;
	return RelChunkPosToRelIndex(relChunkPos);
}

unsigned World::RelChunkPosToRelIndex(glm::ivec2 chunkPos)
{
	unsigned int totalDistance = (_renderDistance * 2) + 1;
	return (chunkPos[1] + _renderDistance) * totalDistance + (chunkPos[0] + _renderDistance);
}

unsigned World::BlockPosToRelChunkIndex(glm::ivec3 blockPos)
{
	return AbsChunkPosToRelIndex(BlockPosToAbsChunkPos(blockPos));
}

bool World::BlockInRenderDistance(glm::ivec3 blockPos)
{
	if (blockPos.y < 0) return false;
	glm::ivec2 absChunkPos = BlockPosToAbsChunkPos(blockPos);
	return ChunkInRenderDistance(absChunkPos);
}

glm::ivec2 World::BlockPosToAbsChunkPos(glm::ivec3 blockPos)
{
	return glm::ivec2(floorf(blockPos.x / static_cast<float>(CHUNK_WIDTH)), floorf(blockPos.z / static_cast<float>(CHUNK_WIDTH)));
}

bool World::ChunkInRenderDistance(glm::ivec2 chunkPos)
{
	return (chunkPos[0] >= _centerChunk[0] - _renderDistance) &&
		(chunkPos[1] >= _centerChunk[1] - _renderDistance) &&
		(chunkPos[0] <= _centerChunk[0] + _renderDistance) &&
		(chunkPos[1] <= _centerChunk[1] + _renderDistance);
}

bool World::ChunkInLoadDistance(glm::ivec2 chunkPos)
{
	return (chunkPos[0] >= _centerChunk[0] - _renderDistance - _extraLoadDistance) &&
		(chunkPos[1] >= _centerChunk[1] - _renderDistance - _extraLoadDistance) &&
		(chunkPos[0] <= _centerChunk[0] + _renderDistance + _extraLoadDistance) &&
		(chunkPos[1] <= _centerChunk[1] + _renderDistance + _extraLoadDistance);
}

glm::ivec2 World::RelChunkIndexToAbsChunkPos(unsigned index)
{
	unsigned int totalDistance = (_renderDistance * 2) + 1;
	glm::ivec2 relPos = glm::ivec2(index % totalDistance, index / totalDistance);
	return relPos + _chunkOrigin;
}

void World::HandleEvent(EventBase* e)
{
	switch (e->_eventType)
	{
	case EventType::ChunkLoaded:
	{
		auto clEvent = static_cast<ChunkLoadedEvent*>(e);
		//_chunks[clEvent->_loadedChunk->_chunkPos] = clEvent->_loadedChunk;
		clEvent->_loadedChunk->BufferMesh();
		clEvent->_loadedChunk->_meshIsLoaded = true;
		break;
	}
	default: ;
	}
}
