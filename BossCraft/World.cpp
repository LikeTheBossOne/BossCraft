#include "World.h"
#include "Chunk.h"
#include "Camera.h"
#include "EventBase.h"
#include "ChunkLoadedEvent.h"
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
	GlobalEventManager::SubscribeToEvent(this, EventType::ChunkLoaded);
	
	_mainCamera = mainCamera;

	_centerChunk = glm::ivec2(0, 0);
	_renderDistance = 15;
	_chunkOrigin = glm::ivec2(-_renderDistance, -_renderDistance);

	unsigned int totalChunks = ((2 * _renderDistance) + 1) * ((2 * _renderDistance) + 1);
	_chunks = std::unordered_map<glm::ivec2, Chunk*>();

	_chunkTaskManager = new ChunkTaskManager(this);
	_dataGenOutput.fill(NULL);
	_meshGenOutput.fill(NULL);
	_chunksToLoad = std::queue<glm::ivec2>();
	
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
			if (_chunks.find(chunkPos) == _chunks.end() || (_chunks[chunkPos]) == NULL)
			{
				continue;
			}
			else
			{
				if (!ChunkInRenderDistance(chunkPos))
				{
					//delete oldChunk;
					//_chunks.erase(chunkPos);
					//_chunks[chunkPos] = NULL;
				}
			}
		}
	}
	LoadNewChunks();
}

void World::Update(float dt)
{
	auto t1 = std::chrono::high_resolution_clock::now();
	std::cout << "start tick here" << std::endl;
	// First Check output arrays for new data

		for (unsigned int i = 0; i < _maxJobs; i++)
		{
			if (_dataGenOutput[i] != NULL)
			{
				Chunk* chunk = _dataGenOutput[i];
				_dataGenOutput[i] = NULL;

				//_chunks[chunk->_chunkPos] = chunk;

				// If a chunk data genned, it now needs a mesh generated, so queue that.
				//TODO: Ensure a mesh to be genned has all of its neighbors data genned
				std::array<Chunk*, 4> neighbors{};
				glm::ivec2 chunkPos = chunk->_chunkPos;
				std::array<glm::ivec2, 4> poses = {
					glm::ivec2(chunkPos[0] + 1, chunkPos[1]),
					glm::ivec2(chunkPos[0] - 1, chunkPos[1]),
					glm::ivec2(chunkPos[0], chunkPos[1] + 1),
					glm::ivec2(chunkPos[0], chunkPos[1] - 1)
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
						neighbors[posIdx] = NULL;
					}
				}

				chunk->GLLoad();
				//auto t = new std::thread(&Chunk::GenerateMesh, chunk, neighbors, i);
				JobSystem::Execute([chunk, neighbors, i] {chunk->GenerateMesh(neighbors, i); });
			}
		}

		auto t2 = std::chrono::high_resolution_clock::now();
		for (unsigned int i = 0; i < _maxJobs; i++)
		{
			if (_meshGenOutput[i] != NULL)
			{
				Chunk* chunk = _meshGenOutput[i];
				_meshGenOutput[i] = NULL;
				chunk->BufferMesh();
				_chunks[chunk->_chunkPos] = chunk;
			}
		}

		auto t3 = std::chrono::high_resolution_clock::now();
	
	CreateLoadChunksTasks();
	auto t4 = std::chrono::high_resolution_clock::now();
	Render();
	auto t5 = std::chrono::high_resolution_clock::now();
	//JobSystem::Wait();
	/*std::cout << "t1->t2: " << std::chrono::duration<float, std::chrono::milliseconds::period>(t1 - t2).count() << std::endl;
	std::cout << "t2->t3: " << std::chrono::duration<float, std::chrono::milliseconds::period>(t2 - t3).count() << std::endl;
	std::cout << "t3->t4: " << std::chrono::duration<float, std::chrono::milliseconds::period>(t3 - t4).count() << std::endl;
	std::cout << "t4->t5: " << std::chrono::duration<float, std::chrono::milliseconds::period>(t4 - t5).count() << std::endl;*/
}

void World::Render()
{
	auto t1 = std::chrono::high_resolution_clock::now();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _textureID);


	auto t2 = std::chrono::high_resolution_clock::now();
	glm::mat4 projection = glm::perspective(glm::radians(_mainCamera->_fov), 800.f / 600.f, 0.1f, 300.0f);

	auto t3 = std::chrono::high_resolution_clock::now();
	_shader->Use();
	_shader->UniSetMat4f("view", _mainCamera->GetViewMatrix());
	_shader->UniSetMat4f("projection", projection);
	auto t4 = std::chrono::high_resolution_clock::now();
	for (int x = _chunkOrigin[0]; x < _chunkOrigin[0] + (_renderDistance * 2) + 1; x++)
	{
		for (int z = _chunkOrigin[1]; z < _chunkOrigin[1] + (_renderDistance * 2) + 1; z++)
		{
			glm::ivec2 pos = glm::ivec2(x, z);
			Chunk* chunk = NULL;
			if (_chunks.find(pos) != _chunks.end() && (chunk = _chunks[pos]) != NULL)
			{
				chunk->RenderMesh(_shader);
			}
		}
		auto t5 = std::chrono::high_resolution_clock::now();
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
	int count = 0;
	while (count < _maxJobs && !_chunksToLoad.empty())
	{
		glm::ivec2 pos = _chunksToLoad.front();
		_chunksToLoad.pop();

		Chunk** outputArr = &_dataGenOutput[count];
		JobSystem::Execute([this, pos, outputArr]
			{
				Chunk* chunkToCreate = new Chunk(pos, this);
				chunkToCreate->LoadData();
				
				while (*outputArr != NULL)
				{
					
				}
				//std::cout << "data" << std::endl;
				*outputArr = chunkToCreate;
				
			});
		
		count++;
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
		//_chunks[clEvent->_loadedChunk->_chunkPos] = clEvent->_loadedChunk;
		clEvent->_loadedChunk->BufferMesh();
		clEvent->_loadedChunk->_meshIsLoaded = true;
		break;
	}
	default: ;
	}
}
