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
#include "Player.h"
#include "TextureAtlas.h"

World::World(Shader* shader, TextureAtlas* atlas, Player* player) : _shader(shader), _textureAtlas(atlas), _player(player)
{
	player->_world = this;
	Init();
}

void World::Init()
{
	_centerChunk = glm::ivec2(0, 0);
	_renderDistance = 3;
	_extraLoadDistance = 1;
	_chunkOrigin = glm::ivec2(-_renderDistance, -_renderDistance);

	unsigned int totalChunks = ((2 * _renderDistance) + 1) * ((2 * _renderDistance) + 1);
	_chunks = std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>>();

	_noiseGenerator = new FastNoiseLite;
	_chunksToLoad = std::queue<glm::ivec2>();
	_chunksToGenMesh = std::queue<glm::ivec2>();


	_shader->Use();
	glm::mat4 projection = glm::perspective(glm::radians(_player->_camera->_fov), 800.f / 600.f, 0.1f, 300.0f);
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

void World::UpdateBlockAtPos(glm::ivec3 blockPos, uint8_t newBlock)
{
	glm::ivec2 chunkPos = BlockPosToAbsChunkPos(blockPos);
	if (_chunks.find(chunkPos) != _chunks.end())
	{
		glm::ivec3 relBlockPos = AbsBlockPosToChunkBlockPos(blockPos);
		std::shared_ptr<Chunk> oldChunk = _chunks[chunkPos];
		// queue for update data
		JobSystem::Execute([this, oldChunk, relBlockPos, newBlock]
			{
				std::shared_ptr<Chunk> chunkToUpdate = std::make_shared<Chunk>(*oldChunk);
				chunkToUpdate->SetData(relBlockPos, newBlock);
				this->_dataUpdateOutput.Enqueue(new std::pair<glm::ivec3, std::shared_ptr<Chunk>>(relBlockPos, chunkToUpdate));
			});
	}
}

void World::Update(float dt)
{
	// Check Data Gen
	std::shared_ptr<Chunk> outChunk;
	while (_dataGenOutput.Dequeue(outChunk))
	{
		_chunks[outChunk->_chunkPos] = outChunk;
		outChunk->GLLoad();
		_chunksToGenMesh.emplace(outChunk->_chunkPos);
	}

	// Check Mesh Gen
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

	// Check Data Update
	std::pair<glm::ivec3, std::shared_ptr<Chunk>>* outChunkAndPos;
	while (_dataUpdateOutput.Dequeue(outChunkAndPos))
	{
		outChunkAndPos->second->GLLoad();
		_chunksToUpdateMesh.emplace(*outChunkAndPos);
		delete outChunkAndPos;
		
	}
	
	// Check Mesh Update
	
	while (_meshUpdateOutput.Dequeue(outChunkAndPos))
	{
		outChunkAndPos->second->BufferMesh();
		_chunks[outChunkAndPos->second->_chunkPos] = outChunkAndPos->second;

		glm::ivec2 chunkPos = outChunkAndPos->second->_chunkPos;
		glm::ivec3 outPos = outChunkAndPos->first;
		if (outPos.x == 0)
		{
			glm::ivec2 minusXPos = { chunkPos[0] - 1, chunkPos[1] };
			if (_chunks.find(minusXPos) != _chunks.end())
			{
				_chunksToGenMesh.emplace(minusXPos);
			}
		}
		else if (outPos.x == CHUNK_WIDTH - 1)
		{
			glm::ivec2 plusXPos = { chunkPos[0] + 1, chunkPos[1] };
			if (_chunks.find(plusXPos) != _chunks.end())
			{
				_chunksToGenMesh.emplace(plusXPos);
			}
		}
		if (outPos.z == 0)
		{
			glm::ivec2 minusZPos = { chunkPos[0], chunkPos[1] - 1 };
			if (_chunks.find(minusZPos) != _chunks.end())
			{
				_chunksToGenMesh.emplace(minusZPos);
			}
		}
		else if (outPos.z == CHUNK_WIDTH - 1)
		{
			glm::ivec2 plusZPos = { chunkPos[0], chunkPos[1] + 1 };
			if (_chunks.find(plusZPos) != _chunks.end())
			{
				_chunksToGenMesh.emplace(plusZPos);
			}
		}

		delete outChunkAndPos;
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
	CreateUpdateMeshTasks();
	Render();
}

void World::Render()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _textureAtlas->_textureID);

	
	_shader->Use();
	//_shader->UniSetMat4f("view", _mainCamera->GetViewMatrix());
	_shader->SetViewMatrix(_player->_camera->GetViewMatrix());
	
	
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
	return _player->_camera;
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
		glm::ivec2 currPos = poses[posIdx];
		if (_chunks.find(currPos) != _chunks.end() && _chunks[currPos] != NULL)
		{
			neighbors[posIdx] = _chunks[currPos];
		}
		else
		{
			return false;
		}
	}

	std::shared_ptr<Chunk> chunk = _chunks[pos];
	JobSystem::Execute([this, chunk, neighbors]
	{
		ChunkMesh* mesh = chunk->GenerateMesh(neighbors);
		_meshGenOutput.Enqueue(new std::pair<glm::vec<2, int, glm::defaultp>, ChunkMesh*>((chunk->_chunkPos), mesh));
	});
	return true;
}

void World::CreateUpdateMeshTasks()
{
	std::vector<std::pair<glm::ivec3, std::shared_ptr<Chunk>>> outsideRange;
	while (!_chunksToUpdateMesh.empty())
	{
		std::pair<glm::ivec3, std::shared_ptr<Chunk>> chunkAndPos = _chunksToUpdateMesh.front();
		_chunksToUpdateMesh.pop();

		if (!CreateSingleUpdateMeshTask(chunkAndPos))
		{
			outsideRange.emplace_back(chunkAndPos);
		}
	}

	for (auto chunkAndPos : outsideRange)
	{
		_chunksToUpdateMesh.emplace(chunkAndPos);
	}
}

bool World::CreateSingleUpdateMeshTask(std::pair<glm::ivec3, std::shared_ptr<Chunk>> chunkAndBlockPos)
{
	std::shared_ptr<Chunk> chunk = chunkAndBlockPos.second;
	glm::ivec2 pos = chunk->_chunkPos;
	std::array<std::shared_ptr<Chunk>, 4> neighbors{};
	std::array<glm::ivec2, 4> poses = {
		glm::ivec2(pos[0] + 1, pos[1]),
		glm::ivec2(pos[0] - 1, pos[1]),
		glm::ivec2(pos[0], pos[1] + 1),
		glm::ivec2(pos[0], pos[1] - 1)
	};
	for (unsigned int posIdx = 0; posIdx < 4; posIdx++)
	{
		glm::ivec2 currPos = poses[posIdx];
		if (_chunks.find(currPos) != _chunks.end() && _chunks[currPos] != NULL)
		{
			neighbors[posIdx] = _chunks[currPos];
		}
		else
		{
			return false;
		}
	}
	glm::ivec3 blockPos = chunkAndBlockPos.first;
	JobSystem::Execute([this, chunk, neighbors, blockPos]
		{
			ChunkMesh* mesh = chunk->GenerateMesh(neighbors);
			chunk->_mesh = mesh;
			_meshUpdateOutput.Enqueue(new std::pair<glm::ivec3, std::shared_ptr<Chunk>>(blockPos, chunk));
		});
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

glm::ivec3 World::AbsBlockPosToChunkBlockPos(glm::ivec3 absBlockPos)
{
	return glm::ivec3(absBlockPos.x % CHUNK_WIDTH, absBlockPos.y, absBlockPos.z % CHUNK_WIDTH);
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

Player* World::GetPlayer()
{
	return _player;
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
