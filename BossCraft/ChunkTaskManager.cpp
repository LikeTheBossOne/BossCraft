#include "ChunkTaskManager.h"

#include "Chunk.h"

ChunkTaskManager::ChunkTaskManager(World* owningWorld) : _owningWorld(owningWorld)
{
	_threadPool = new ThreadPool(2);
}

void ChunkTaskManager::AddDataGenTask(glm::ivec2 chunkPosToCreate)
{
	_threadPool->Enqueue([&, this, chunkPosToCreate](unsigned int threadNum)
		{
			std::shared_ptr<Chunk> chunkToCreate = std::make_shared<Chunk>(chunkPosToCreate, _owningWorld);
			chunkToCreate->LoadData();
			auto &output = _owningWorld->_dataGenOutput;
			bool found = false;

			while (!found)
			{
				if (output[threadNum] == NULL)
				{
					found = true;
					output[threadNum] = chunkToCreate;
				}
			}
		/*while (!found)
			{
				if (output[threadNum * 8] == NULL)
				{
					found = true;
					output[threadNum * 8] = chunkToCreate;
				}
				else if (output[threadNum * 8 + 1] == NULL)
				{
					found = true;
					output[threadNum * 8 + 1] = chunkToCreate;
				}
				else if (output[threadNum * 8 + 2] == NULL)
				{
					found = true;
					output[threadNum * 8 + 2] = chunkToCreate;
				}
				else if (output[threadNum * 8 + 3] == NULL)
				{
					found = true;
					output[threadNum * 8 + 3] = chunkToCreate;
				}
				else if (output[threadNum * 8 + 4] == NULL)
				{
					found = true;
					output[threadNum * 8 + 4] = chunkToCreate;
				}
				else if (output[threadNum * 8 + 5] == NULL)
				{
					found = true;
					output[threadNum * 8 + 5] = chunkToCreate;
				}
				else if (output[threadNum * 8 + 6] == NULL)
				{
					found = true;
					output[threadNum * 8+ 6] = chunkToCreate;
				}
				else if (output[threadNum * 8 + 7] == NULL)
				{
					found = true;
					output[threadNum * 8 + 7] = chunkToCreate;
				}
			}*/
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
			bool found = false;
			while (!found)
			{
				if (output[threadNum] == NULL)
				{
					found = true;
					//output[threadNum] = chunk;
				}
				/*else if (output[threadNum * 4 + 1] == NULL)
				{
					found = true;
					output[threadNum * 4 + 1] = chunk;
				}
				else if (output[threadNum * 4 + 2] == NULL)
				{
					found = true;
					output[threadNum * 4 + 2] = chunk;
				}
				else if (output[threadNum * 4 + 3] == NULL)
				{
					found = true;
					output[threadNum * 4 + 3] = chunk;
				}*/
			}
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
