#include "Chunk.h"
#include <glad/glad.h>
#include <glm/mat4x4.hpp>
#include "Shader.h"
#include "Camera.h"
#include "ChunkMesh.h"
#include "FaceDirection.h"
#include "World.h"

Chunk::Chunk(glm::ivec2 chunkPos, World* owningWorld) : _chunkPos(chunkPos), _world(owningWorld)
{
	_isDirty = true;
	_meshIsLoaded = false;
	for (int x = 0; x < CHUNK_WIDTH; x++)
	{
		for (int y = 0; y < CHUNK_HEIGHT; y++)
		{
			for (int z = 0; z < CHUNK_WIDTH; z++)
			{
				if (y == CHUNK_HEIGHT-1 && x == 0)
				{
					_data[PositionToIndex(x, y, z)] = 0;
				}
				else
				{
					_data[PositionToIndex(x, y, z)] = 1;
				}
			}
		}
	}

	//unsigned int VAO, VBO, EBO;
	//glGenVertexArrays(1, &VAO);
	//glGenBuffers(1, &VBO);
	//glGenBuffers(1, &EBO);

	//glBindVertexArray(VAO);
	//glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//// position attribute
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	//glEnableVertexAttribArray(0);

	//// texture coord attribute
	//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	//glEnableVertexAttribArray(1);

	//glBindVertexArray(0);

	_indexCount = 0;
	_mesh = new ChunkMesh
	{
		0,
		0,
		0,
		(float*)malloc(sizeof(float) * (CHUNK_VOLUME) * 8 * 5),
		(uint16_t*)malloc((CHUNK_VOLUME) * 36 * sizeof(uint16_t)),
	};
}

Chunk::~Chunk()
{
	free(_mesh->dataBuffer);
	free(_mesh->indexBuffer);
	
	delete _mesh;
}

unsigned int Chunk::PositionToIndex(unsigned int posX, unsigned int posY, unsigned int posZ)
{
	return posX * CHUNK_HEIGHT * CHUNK_WIDTH + posY * CHUNK_WIDTH + posZ;
}

unsigned Chunk::PositionToIndex(glm::ivec3 pos)
{
	return pos.x * CHUNK_HEIGHT * CHUNK_WIDTH + pos.y * CHUNK_WIDTH + pos.z;
}

glm::vec3 Chunk::IndexToPosition(unsigned int index)
{
	const int z = index % CHUNK_WIDTH;
	const int y = (index / CHUNK_WIDTH) % CHUNK_HEIGHT;
	const int x = index / (CHUNK_HEIGHT * CHUNK_WIDTH);
	return glm::vec3(x, y, z);
}

bool Chunk::BlockInChunkBounds(glm::ivec3 pos)
{
	return pos.x >= 0 && pos.y >= 0 && pos.z >= 0 &&
		pos.x < CHUNK_WIDTH && pos.y < CHUNK_HEIGHT && pos.z < CHUNK_WIDTH;
}

/**
 * Re-Loads data into the data arrays if it is dirty
 */
void Chunk::LoadData()
{
	if (_isDirty)
	{
		// First reset mesh data.
		_mesh->vertexCount = 0;
		_mesh->dataIndex = 0;
		_mesh->indicesIndex = 0;

		//TODO: In the future, find a better way then mem-copying all of buffers.data. Maybe malloc would be better
		//memcpy(_mesh->dataBuffer, buffers.data, sizeof(buffers.data));
		//memcpy(_mesh->indexBuffer, buffers.indices, sizeof(buffers.indices));

		// Unlock after use
		_isDirty = false;
	}
}

void Chunk::GenerateMesh(std::array<std::shared_ptr<Chunk>, 4> neighbors, unsigned int outputIdx)
{
	// Generate Mesh
	for (unsigned x = 0; x < CHUNK_WIDTH; x++)
	{
		for (unsigned y = 0; y < CHUNK_HEIGHT; y++)
		{
			for (unsigned z = 0; z < CHUNK_WIDTH; z++)
			{
				glm::ivec3 pos(x, y, z);
				glm::ivec3 wPos = pos + glm::ivec3(_chunkPos[0] * CHUNK_WIDTH, 0, _chunkPos[1] * CHUNK_WIDTH);
				unsigned int data = _data[PositionToIndex(pos)];

				if (data != 0)
				{
					// loop over directions
					for (int d = 0; d < 6; d++)
					{
						glm::ivec3 dirVec = DIRECTION_VEC[d];
						glm::ivec3 neighbor = pos + dirVec;

						bool visible = false;

						if (BlockInChunkBounds(neighbor))
						{
							// determine if block is transparent (0 = transparent block)
							visible = (_data[PositionToIndex(neighbor)] == 0);
						}
						else
						{
							glm::ivec3 wNeighbor = wPos + dirVec;
							
							if (_world->BlockInRenderDistance(wNeighbor))
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
							AddFaceToMesh(pos, static_cast<FaceDirection>(d));
						}
					}
				}
			}
		}
	}
	//std::cout << "Mesh" << std::endl;
	auto& output = _world->_meshGenOutput;
	output[outputIdx] = &_chunkPos;
}

void Chunk::GLLoad()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	_meshIsLoaded = true;
}


/**
 * Renders the ChunkMesh. MUST BE RUN ON MAIN THREAD
 */
void Chunk::RenderMesh(Shader* shader)
{
	if (!_meshIsLoaded)
	{
		return;
	}
	
	//Shader* shader = _world->GetShader();
	shader->Use();
	glm::mat4 model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(_chunkPos[0] * 1.f * CHUNK_WIDTH, 0, _chunkPos[1] * 1.f * CHUNK_WIDTH));

	shader->UniSetMat4f("model", model);

	
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}

void Chunk::AddFaceToMesh(glm::vec3 blockPos, FaceDirection direction)
{
	for (int i = 0; i < 4; i++)
	{
		const float* vertex = &CUBE_VERTICES[CUBE_INDICES[(direction * 6) + UNIQUE_INDICES[i]] * 3];
		_mesh->dataBuffer[_mesh->dataIndex++] = blockPos.x + vertex[0];
		_mesh->dataBuffer[_mesh->dataIndex++] = blockPos.y + vertex[1];
		_mesh->dataBuffer[_mesh->dataIndex++] = blockPos.z + vertex[2];
		_mesh->dataBuffer[_mesh->dataIndex++] = CUBE_UVS[(i * 2) + 0];
		_mesh->dataBuffer[_mesh->dataIndex++] = CUBE_UVS[(i * 2) + 1];
	}

	for (int i = 0; i < 6; i++)
	{
		_mesh->indexBuffer[_mesh->indicesIndex++] = _mesh->vertexCount + FACE_INDICES[i];
	}

	_mesh->vertexCount += 4;
}

void Chunk::BufferMesh()
{
	_indexCount = _mesh->indicesIndex;
	
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, _mesh->dataIndex * sizeof(float), _mesh->dataBuffer, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indexCount * sizeof(uint16_t), _mesh->indexBuffer, GL_STATIC_DRAW);
}


unsigned Chunk::GetDataAtPosition(glm::vec3 pos)
{
	return _data[PositionToIndex(pos)];
}

glm::ivec3 Chunk::AbsBlockPosToRelPos(glm::ivec3 blockPos)
{
	return glm::ivec3(blockPos.x % CHUNK_WIDTH, blockPos.y, blockPos.z % CHUNK_WIDTH);
}

uint8_t Chunk::GetNeighborBlockAtPos(glm::ivec3 pos, uint8_t* neighborData)
{
	return neighborData[PositionToIndex(pos)];
}
