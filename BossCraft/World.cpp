#include "World.h"
#include "Chunk.h"
#include "Camera.h"
#include <stdlib.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdlib.h>
#include "EventBase.h"
#include "ChunkLoadedEvent.h"
#include "ChunkGenerator.h"
#include "NeighborChunks.h"
#include "GlobalEventManager.h"


World::World(Shader* shader, unsigned int textureID) : _shader(shader), _textureID(textureID)
{
	Init(new Camera(this, glm::vec3(0, 65, 0)));
}

World::World(Shader* shader, unsigned int textureID, Camera* mainCamera) : _shader(shader), _textureID(textureID)
{
	Init(mainCamera);
	/*memset(_chunks, 0, sizeof(Chunk*) * totalChunks);
	
	LoadNewChunks();*/
}

void World::Init(Camera* mainCamera)
{
	GlobalEventManager::SubscribeToEvent(this, EventType::ChunkLoaded);
	
	_mainCamera = mainCamera;

	_centerChunk = glm::ivec2(0, 0);
	_renderDistance = 2;
	_chunkOrigin = glm::ivec2(-_renderDistance, -_renderDistance);

	unsigned int totalChunks = ((2 * _renderDistance) + 1) * ((2 * _renderDistance) + 1);
	_chunks = std::unordered_map<glm::ivec2, Chunk*>();

	_chunkGenerator = new ChunkGenerator;

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

	
	for (int x = oldOrigin.x; x < oldOrigin.x + (_renderDistance * 2) + 1; x++)
	{
		for (int z = oldOrigin[1]; z < oldOrigin[1] + (_renderDistance * 2) + 1; z++)
		{
			glm::ivec2 chunkPos = glm::ivec2(x, z);
			Chunk* oldChunk = NULL;
			if (_chunks.find(chunkPos) == _chunks.end() || (oldChunk = _chunks[chunkPos]) == NULL)
			{
				continue;
			}
			else
			{
				if (!ChunkInRenderDistance(chunkPos))
				{
					//delete oldChunk;
					//_chunks[chunkPos] = NULL;
				}
			}
		}
	}
	LoadNewChunks();
}

void World::Update(float dt)
{
	//std::thread* update = new std::thread(&World::UpdateChunks, this, dt);
	Render();
}

void World::UpdateChunks(float dt)
{
	UpdateChunkData(dt);
	UpdateChunkMeshes();
}

void World::UpdateChunkData(float dt)
{
	for (int x = _chunkOrigin[0]; x < _chunkOrigin[0] + (_renderDistance * 2) + 1; x++)
	{
		for (int z = _chunkOrigin[1]; z < _chunkOrigin[1] + (_renderDistance * 2) + 1; z++)
		{
			glm::ivec2 pos = glm::ivec2(x, z);
			Chunk* chunk = NULL;
			if (_chunks.find(pos) != _chunks.end() && (chunk = _chunks[pos]) != NULL)
			{
				chunk->LoadData();
			}
		}
	}
}

void World::UpdateChunkMeshes()
{
	for (int x = _chunkOrigin[0]; x < _chunkOrigin[0] + (_renderDistance * 2) + 1; x++)
	{
		for (int z = _chunkOrigin[1]; z < _chunkOrigin[1] + (_renderDistance * 2) + 1; z++)
		{
			glm::ivec2 pos = glm::ivec2(x, z);
			Chunk* chunk = NULL;
			if (_chunks.find(pos) != _chunks.end() && (chunk = _chunks[pos]) != NULL)
			{
				chunk->GenerateMesh();
			}
		}
	}
}

void World::Render()
{
	for (int x = _chunkOrigin[0]; x < _chunkOrigin[0] + (_renderDistance * 2) + 1; x++)
	{
		for (int z = _chunkOrigin[1]; z < _chunkOrigin[1] + (_renderDistance * 2) + 1; z++)
		{
			glm::ivec2 pos = glm::ivec2(x, z);
			Chunk* chunk = NULL;
			if (_chunks.find(pos) != _chunks.end() && (chunk = _chunks[pos]) != NULL)
			{
				chunk->RenderMesh();
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
	Chunk* chunk = _chunks[chunkPos];
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
	for (int x = _chunkOrigin[0]; x < _chunkOrigin[0] + (_renderDistance * 2) + 1; x++)
	{
		for (int z = _chunkOrigin[1]; z < _chunkOrigin[1] + (_renderDistance * 2) + 1; z++)
		{
			glm::ivec2 pos = glm::ivec2(x, z);
			Chunk* chunk;
			if (_chunks.find(pos) == _chunks.end() || _chunks[pos] == NULL)
			{
				// Create chunk task
				NeighborChunks* neighbor = new NeighborChunks();
				glm::ivec2 posXChunk = pos - glm::ivec2(1, 0);
				if ((neighbor->plusXLoaded = _chunks.find(posXChunk) != _chunks.end()))
				{
					//neighbor->plusXChunk = _chunks[posXChunk];
					memcpy(neighbor->plusXData, _chunks[posXChunk]->_data, sizeof(uint8_t) * CHUNK_VOLUME);
				}
				glm::ivec2 minusXChunk = pos - glm::ivec2(-1, 0);
				if ((neighbor->minusXLoaded = _chunks.find(minusXChunk) != _chunks.end()))
				{
					//neighbor->minusXChunk = _chunks[minusXChunk];
					memcpy(neighbor->minusXData, _chunks[minusXChunk]->_data, sizeof(uint8_t) * CHUNK_VOLUME);
				}
				glm::ivec2 posZChunk = pos - glm::ivec2(0, 1);
				if ((neighbor->plusZLoaded = _chunks.find(posZChunk) != _chunks.end()))
				{
					//neighbor->plusZChunk = _chunks[posZChunk];
					memcpy(neighbor->plusZData, _chunks[posZChunk]->_data, sizeof(uint8_t) * CHUNK_VOLUME);
				}
				glm::ivec2 minusZChunk = pos - glm::ivec2(1, 0);
				if ((neighbor->minusZLoaded = _chunks.find(minusZChunk) != _chunks.end()))
				{
					//neighbor->minusZChunk = _chunks[minusZChunk];
					memcpy(neighbor->minusZData, _chunks[minusZChunk]->_data, sizeof(uint8_t) * CHUNK_VOLUME);
				}
				auto thread = new std::thread(&ChunkGenerator::GenerateChunk, _chunkGenerator, new Chunk(pos, this), neighbor);
			}
		}
	}
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
		_chunks[clEvent->_loadedChunk->_chunkPos] = clEvent->_loadedChunk;
		clEvent->_loadedChunk->BufferMesh();
		clEvent->_loadedChunk->_meshIsLoaded = true;
		break;
	}
	default: ;
	}
}
