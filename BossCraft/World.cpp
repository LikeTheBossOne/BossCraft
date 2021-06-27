#include "World.h"
#include "Chunk.h"
#include "Camera.h"


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
	_mainCamera = mainCamera;

	_centerChunk = glm::ivec2(0, 0);
	_renderDistance = 1;
	_chunkOrigin = glm::ivec2(-_renderDistance, -_renderDistance);

	unsigned int totalChunks = ((2 * _renderDistance) + 1) * ((2 * _renderDistance) + 1);
	_chunks = (Chunk**)malloc(sizeof(Chunk*) * totalChunks);
	memset(_chunks, 0, sizeof(Chunk*) * totalChunks);

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
	_chunkOrigin += offset;
	_centerChunk = newCenterChunk;
	
	unsigned int totalChunks = ((2 * _renderDistance) + 1) * ((2 * _renderDistance) + 1);
	Chunk** oldChunks = static_cast<Chunk**>(malloc(sizeof(Chunk*) * totalChunks));

	memcpy(oldChunks, _chunks, sizeof(Chunk*) * totalChunks);
	memset(_chunks, 0, sizeof(Chunk*) * totalChunks);

	for (unsigned int i = 0; i < totalChunks; i++)
	{
		Chunk* oldChunk = oldChunks[i];
		if (oldChunk == NULL)
		{
			continue;
		}
		else if (ChunkInRenderDistance(oldChunk->_chunkPos))
		{
			_chunks[AbsChunkPosToRelIndex(oldChunk->_chunkPos)] = oldChunk;
		}
		else
		{
			// Destroy chunk
			delete oldChunk;
			oldChunk = NULL;
		}
	}
	LoadNewChunks();
}

void World::Update(float dt)
{
	unsigned int totalChunks = ((2 * _renderDistance) + 1) * ((2 * _renderDistance) + 1);
	for (int i = 0; i < totalChunks; i++)
	{
		Chunk* chunk = _chunks[i];
		if (chunk == NULL)
		{
			continue;
		}

		//std::thread t(&Chunk::LoadData, chunk);
		//std::thread t2(&Chunk::GenerateMesh, chunk);
		chunk->Update(dt);
	}
}

void World::UpdateMesh()
{
	
}

unsigned World::GetBlockAtAbsPos(glm::ivec3 blockPos)
{
	if (blockPos.y < 0 || blockPos.y >= CHUNK_HEIGHT) return 0;
	
	// Assumes that block is inside of render distance!
	unsigned int chunkIdx = BlockPosToRelChunkIndex(blockPos);

	// Get block's position inside of the chunk
	Chunk* chunk = _chunks[chunkIdx];
	glm::vec3 pos = glm::vec3(blockPos.x - (chunk->_chunkPos[0] * CHUNK_WIDTH), blockPos.y, blockPos.z - (chunk->_chunkPos[1] * CHUNK_WIDTH));

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
	unsigned int totalChunks = ((2 * _renderDistance) + 1) * ((2 * _renderDistance) + 1);
	for (unsigned int i = 0; i < totalChunks; i++)
	{
		if (_chunks[i] == NULL)
		{
			Chunk* chunk = new Chunk(RelChunkIndexToAbsChunkPos(i), this);
			_chunks[i] = chunk;
		}
	}
}

void World::LoadNewChunksRadially()
{
	for (int i = 0; i <= _renderDistance; i++)
	{
		// Get chunks in radius
		for (int j = -i; j < i; j+= i * i)
		{
			for (int k = -i; k < i; k+= i * i)
			{
				
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
