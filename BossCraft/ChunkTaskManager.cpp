#include "ChunkTaskManager.h"

#include "Chunk.h"

ChunkTaskManager::ChunkTaskManager(World* owningWorld) : _owningWorld(owningWorld)
{
	_threadPool = new ThreadPool(20);
}

void ChunkTaskManager::AddDataGenTask(std::shared_ptr<Chunk> chunkToCreate)
{
	_threadPool->Enqueue([&, this, chunkToCreate](unsigned int threadNum)
		{
			chunkToCreate->LoadData();
			auto &output = _owningWorld->_dataGenOutput;
			while (output[threadNum] != NULL)
			{
			}
			output[threadNum] = chunkToCreate;
		});
}

void ChunkTaskManager::AddDataUpdateTask(std::shared_ptr<Chunk> chunk)
{
}

void ChunkTaskManager::AddMeshGenTask(std::shared_ptr<Chunk> chunk, std::array<std::shared_ptr<Chunk>, 4> neighbors)
{
	_threadPool->Enqueue([&, this, chunk, neighbors](unsigned int threadNum)
		{
			// Generate Mesh
			for (unsigned x = 0; x < CHUNK_WIDTH; x++)
			{
				for (unsigned y = 0; y < CHUNK_HEIGHT; y++)
				{
					for (unsigned z = 0; z < CHUNK_WIDTH; z++)
					{
						glm::ivec3 pos(x, y, z);
						glm::ivec3 wPos = pos + glm::ivec3(chunk->_chunkPos[0] * CHUNK_WIDTH, 0, chunk->_chunkPos[1] * CHUNK_WIDTH);
						unsigned int data = chunk->_data[PositionToIndex(pos)];

						if (data != 0)
						{
							// loop over directions
							for (int d = 0; d < 6; d++)
							{
								glm::ivec3 dirVec = DIRECTION_VEC[d];
								glm::ivec3 neighbor = pos + dirVec;

								bool visible = false;

								if (chunk->BlockInChunkBounds(neighbor))
								{
									// determine if block is transparent (0 = transparent block)
									visible = (chunk->_data[PositionToIndex(neighbor)] == 0);
								}
								else
								{
									World* world = chunk->_world;
									glm::ivec3 wNeighbor = wPos + dirVec;

									if (chunk->_world->BlockInRenderDistance(wNeighbor))
									{
										visible = true;

										glm::ivec3 blockRelPos = AbsBlockPosToRelPos(wNeighbor);
										// Figure out which neighbor to send it to
										if (d == FaceDirection::EAST)
										{
											if (neighbors[0] != NULL)
											{
												visible = (GetNeighborBlockAtPos(blockRelPos, neighbors[0]->_data) == 0);
											}
										}
										else if (d == FaceDirection::WEST)
										{
											if (neighbors[1] != NULL)
											{
												visible = (GetNeighborBlockAtPos(blockRelPos, neighbors[1]->_data) == 0);
											}
										}
										else if (d == FaceDirection::SOUTH)
										{
											if (neighbors[2] != NULL)
											{
												visible = (GetNeighborBlockAtPos(blockRelPos, neighbors[2]->_data) == 0);
											}
										}
										else if (d == FaceDirection::NORTH)
										{
											if (neighbors[3] != NULL)
											{
												visible = (GetNeighborBlockAtPos(blockRelPos, neighbors[3]->_data) == 0);
											}
										}
									}
									else
									{
										visible = false;
									}
								}

								if (visible)
								{
									chunk->AddFaceToMesh(pos, static_cast<FaceDirection>(d));
								}
							}
						}
					}
				}
			}
			auto& output = _owningWorld->_meshGenOutput;
			while (output[threadNum] != NULL)
			{
			}
			output[threadNum] = chunk;
		});
}

unsigned ChunkTaskManager::PositionToIndex(glm::ivec3 pos)
{
	return pos.x * CHUNK_HEIGHT * CHUNK_WIDTH + pos.y * CHUNK_WIDTH + pos.z;
}

glm::ivec3 ChunkTaskManager::AbsBlockPosToRelPos(glm::ivec3 blockPos)
{
	return glm::ivec3(blockPos.x % CHUNK_WIDTH, blockPos.y, blockPos.z % CHUNK_WIDTH);
}

uint8_t ChunkTaskManager::GetNeighborBlockAtPos(glm::ivec3 pos, uint8_t* neighborData)
{
	return neighborData[PositionToIndex(pos)];
}
