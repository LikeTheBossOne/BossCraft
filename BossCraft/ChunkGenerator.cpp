#include "ChunkGenerator.h"
#include "NeighborChunks.h"
#include "World.h"
#include "GlobalEventManager.h"
#include "ChunkLoadedEvent.h"

void ChunkGenerator::GenerateChunks(glm::ivec2 centerChunk, uint8_t renderDistance, World* world)
{
	for (uint8_t i = 0; i <= renderDistance; i++)
	{
		LoadChunksData(centerChunk, i, world);
		GenerateChunksMeshes(centerChunk, renderDistance, world);
	}
}

void ChunkGenerator::LoadChunksData(glm::ivec2 centerChunk, uint8_t drawRadius, World* world)
{
	// load chunks on radius
	// draw +X chunks
	// draw -X chunks
	// draw +Z chunks
	// draw -Z chunks
	// dont overlap
}

void ChunkGenerator::GenerateChunksMeshes(glm::ivec2 centerChunk, uint8_t drawRadius, World* world)
{
	// Load Chunk Meshes radially
}

void ChunkGenerator::GenerateChunk(Chunk* chunk, NeighborChunks* neighbors)
{
	// Reload data
	if (chunk->_isDirty)
	{
		ChunkMesh* mesh = chunk->_mesh;
		
		// First reset mesh data.
		mesh->vertexCount = 0;
		mesh->dataIndex = 0;
		mesh->indicesIndex = 0;

		//TODO: In the future, find a better way then mem-copying all of buffers.data. Maybe malloc would be better
		memcpy(mesh->dataBuffer, buffers.data, sizeof(buffers.data));
		memcpy(mesh->indexBuffer, buffers.indices, sizeof(buffers.indices));
	}

	// Generate Mesh
	for (unsigned x = 0; x < CHUNK_WIDTH; x++)
	{
		for (unsigned y = 0; y < CHUNK_HEIGHT; y++)
		{
			for (unsigned z = 0; z < CHUNK_WIDTH; z++)
			{
				glm::ivec3 pos(x, y, z);
				glm::ivec3 wPos = pos + glm::ivec3(chunk->_chunkPos[0] * CHUNK_WIDTH, 0, chunk->_chunkPos[1] * CHUNK_WIDTH);
				unsigned int data = chunk->_data[PositionToIndex(x, y, z)];

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
							visible = (chunk->_data[PositionToIndex(neighbor.x, neighbor.y, neighbor.z)] == 0);
						}
						else
						{
							World* world = chunk->_world;
							glm::ivec3 wNeighbor = wPos + dirVec;
							/*if (world->IsChunkLoaded(world->BlockPosToAbsChunkPos(wNeighbor)))
							{
								visible = world->BlockInRenderDistance(wNeighbor) && world->GetBlockAtAbsPos(wNeighbor) == 0;
							}
							else
							{
								visible = true;
							}*/
							
							
							if (chunk->_world->BlockInRenderDistance(wNeighbor))
							{
								visible = true;
								
								glm::ivec3 blockRelPos = AbsBlockPosToRelPos(wNeighbor);
								// Figure out which neighbor to send it to
								if (d == FaceDirection::EAST)
								{
									if (neighbors->plusXLoaded)
									{
										visible = (GetNeighborBlockAtPos(blockRelPos, neighbors->plusXData) == 0);
									}
								}
								else if (d == FaceDirection::WEST)
								{
									if (neighbors->minusXLoaded)
									{
										visible = (GetNeighborBlockAtPos(blockRelPos, neighbors->minusXData) == 0);
									}	
								}
								else if (d == FaceDirection::SOUTH)
								{
									if (neighbors->plusZLoaded)
									{
										visible = (GetNeighborBlockAtPos(blockRelPos, neighbors->plusZData) == 0);
									}
								}
								else if (d == FaceDirection::NORTH)
								{
									if (neighbors->minusZLoaded)
									{
										visible = (GetNeighborBlockAtPos(blockRelPos, neighbors->minusZData) == 0);
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

	GlobalEventManager::RaiseEvent(new ChunkLoadedEvent(chunk));
	
	chunk->_isDirty = false;
}

glm::ivec3 ChunkGenerator::AbsBlockPosToRelPos(glm::ivec3 blockPos)
{
	return glm::ivec3(blockPos.x % CHUNK_WIDTH, blockPos.y, blockPos.z % CHUNK_WIDTH);
}

uint8_t ChunkGenerator::GetNeighborBlockAtPos(glm::ivec3 pos, uint8_t* neighborData)
{
	return neighborData[PositionToIndex(pos)];
}

unsigned int ChunkGenerator::PositionToIndex(unsigned int posX, unsigned int posY, unsigned int posZ)
{
	return posX * CHUNK_HEIGHT * CHUNK_WIDTH + posY * CHUNK_WIDTH + posZ;
}

unsigned ChunkGenerator::PositionToIndex(glm::ivec3 pos)
{
	return pos.x * CHUNK_HEIGHT * CHUNK_WIDTH + pos.y * CHUNK_WIDTH + pos.z;
}