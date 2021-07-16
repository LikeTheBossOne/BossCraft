#include "Chunk.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>

#include "BlockProvider.h"
#include "Shader.h"
#include "Camera.h"
#include "ChunkMesh.h"
#include "FaceDirection.h"
#include "TextureAtlas.h"
#include "World.h"

Chunk::Chunk(glm::ivec2 chunkPos, World* owningWorld) : _chunkPos(chunkPos), _world(owningWorld)
{
	_isDirty = true;
	_meshIsLoaded = false;

	_indexCount = 0;
	_mesh = NULL;
}

Chunk::Chunk(Chunk& other)
{
	VAO = 0;
	VBO = 0;
	EBO = 0;
	_indexCount = other._indexCount;
	_mesh = NULL;

	_world = other._world;
	_data = other._data;
	_chunkPos = other._chunkPos;
	_isDirty = other._isDirty;
	_meshIsLoaded = other._meshIsLoaded;
}

Chunk::~Chunk()
{
	_world->_chunkUnload.Enqueue(new std::array<unsigned int, 3>({ VAO, VBO, EBO }));
	delete _mesh;
}

void Chunk::SetData(glm::ivec3 blockPos, uint8_t blockType)
{
	_data[PositionToIndex(blockPos)] = blockType;
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
 * Re-Loads data into the data arrays if it is dirty -> Not main thread
 */
void Chunk::LoadData()
{
	if (_isDirty)
	{
		for (unsigned int x = 0; x < CHUNK_WIDTH; x++)
		{
			for (unsigned int z = 0; z < CHUNK_WIDTH; z++)
			{
				float absX = static_cast<float>(x) + (static_cast<float>(_chunkPos[0]) * static_cast<float>(CHUNK_WIDTH));
				float absZ = static_cast<float>(z) + (static_cast<float>(_chunkPos[1]) * static_cast<float>(CHUNK_WIDTH));
				float noise = _world->_noiseGenerator->GetNoise(absX, absZ);
				unsigned int normNoise = floor((noise + 1) * 5 + (CHUNK_HEIGHT / 2.f));
				normNoise = std::min(normNoise, CHUNK_HEIGHT - 1);
				for (unsigned int y = 0; y < CHUNK_HEIGHT; y++)
				{
					if (y < normNoise)
					{
						_data[PositionToIndex(x, y, z)] = 1;
					}
					else
					{
						_data[PositionToIndex(x, y, z)] = 0;
					}
				}

				/*for (int z = 0; z < CHUNK_WIDTH; z++)
				{
					if (y == CHUNK_HEIGHT-1 && x == 0)
					{
						_data[PositionToIndex(x, y, z)] = 0;
					}
					else
					{
						_data[PositionToIndex(x, y, z)] = 1;
					}

				}*/
			}
		}

		// Unlock after use
		_isDirty = false;
	}
}

/**
 * Not main thread
 */
void Chunk::GenerateMesh(std::array<std::shared_ptr<Chunk>, 4> neighbors)
{
	ChunkMesh* mesh = new ChunkMesh;
	// Generate Mesh
	for (unsigned x = 0; x < CHUNK_WIDTH; x++)
	{
		for (unsigned y = 0; y < CHUNK_HEIGHT; y++)
		{
			for (unsigned z = 0; z < CHUNK_WIDTH; z++)
			{
				glm::ivec3 pos(x, y, z);
				glm::ivec3 wPos = pos + glm::ivec3(_chunkPos[0] * CHUNK_WIDTH, 0, _chunkPos[1] * CHUNK_WIDTH);
				uint8_t data = _data[PositionToIndex(pos)];

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
							//visible = true;
						}
						else
						{
							glm::ivec3 wNeighbor = wPos + dirVec;
							
							visible = true;

							glm::ivec3 blockRelPos = AbsBlockPosToRelPos(wNeighbor);
							// Figure out which neighbor to send it to
							if (d == FaceDirection::EAST)
							{
								if (neighbors[0] != NULL)
								{
									visible = (GetNeighborBlockAtPos(blockRelPos, neighbors[0]->_data.data()) == 0);
								}
							}
							else if (d == FaceDirection::WEST)
							{
								if (neighbors[1] != NULL)
								{
									visible = (GetNeighborBlockAtPos(blockRelPos, neighbors[1]->_data.data()) == 0);
								}
							}
							else if (d == FaceDirection::SOUTH)
							{
								if (neighbors[2] != NULL)
								{
									visible = (GetNeighborBlockAtPos(blockRelPos, neighbors[2]->_data.data()) == 0);
								}
							}
							else if (d == FaceDirection::NORTH)
							{
								if (neighbors[3] != NULL)
								{
									visible = (GetNeighborBlockAtPos(blockRelPos, neighbors[3]->_data.data()) == 0);
								}
							}
						}

						if (visible)
						{
							AddFaceToMesh(pos, static_cast<FaceDirection>(d), mesh);
						}
					}
				}
			}
		}
	}
	//std::string output = "GenMesh: " + std::to_string(_chunkPos[0]) + ", " + std::to_string(_chunkPos[1]);
	//std::cout << output << std::endl;
	//std::cout << "Mesh" << std::endl;
	_world->_meshGenOutput.Enqueue(new std::pair<glm::vec<2, int, glm::defaultp>, ChunkMesh*>(_chunkPos, mesh));
}

void Chunk::GLLoad()
{
	//std::string output = "Load: " + std::to_string(_chunkPos[0]) + ", " + std::to_string(_chunkPos[1]);
	//std::cout << output << std::endl;
	
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	_meshIsLoaded = true;
}

void Chunk::GLUnload()
{
	/*glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);*/
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
	
	shader->Use();
	glm::mat4 model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(_chunkPos[0] * 1.f * CHUNK_WIDTH, 0, _chunkPos[1] * 1.f * CHUNK_WIDTH));

	shader->SetModel(model);
	//shader->UniSetMat4f("model", model);

	
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}

void Chunk::AddFaceToMesh(glm::vec3 blockPos, FaceDirection direction, ChunkMesh* mesh)
{
	TextureAtlas* atlas = _world->_textureAtlas;
	unsigned char block = _data[PositionToIndex(blockPos)];
	glm::vec2 texCoords = BlockProvider::GetBlockTextureLocation(block, direction);
	for (int i = 0; i < 4; i++)
	{
		const float* vertex = &CUBE_VERTICES[CUBE_INDICES[(direction * 6) + UNIQUE_INDICES[i]] * 3];
		mesh->dataBuffer[mesh->dataIndex++] = blockPos.x + vertex[0];
		mesh->dataBuffer[mesh->dataIndex++] = blockPos.y + vertex[1];
		mesh->dataBuffer[mesh->dataIndex++] = blockPos.z + vertex[2];
		mesh->dataBuffer[mesh->dataIndex++] = texCoords[0] * atlas->_uStep + (atlas->_uStep * CUBE_UVS[(i * 2) + 0]);
		mesh->dataBuffer[mesh->dataIndex++] = texCoords[1] * (16.f / 256.f) + ((16.f / 256.f) * CUBE_UVS[(i * 2) + 1]);
		if (direction == FaceDirection::EAST)
		{
			mesh->dataBuffer[mesh->dataIndex++] = 0.1f;
		}
		else if (direction == FaceDirection::WEST)
		{
			mesh->dataBuffer[mesh->dataIndex++] = 0.2f;
		}
		else if (direction == FaceDirection::NORTH)
		{
			mesh->dataBuffer[mesh->dataIndex++] = 0.3f;
		}
		else if (direction == FaceDirection::SOUTH)
		{
			mesh->dataBuffer[mesh->dataIndex++] = 0.2f;
		}
		else
		{
			mesh->dataBuffer[mesh->dataIndex++] = 0.0f;
		}
		
	}

	for (int i = 0; i < 6; i++)
	{
		mesh->indexBuffer[mesh->indicesIndex++] = mesh->vertexCount + FACE_INDICES[i];
	}

	mesh->vertexCount += 4;
}

void Chunk::BufferMesh()
{
	//std::string output = "BufferMesh: " + std::to_string(_chunkPos[0]) + ", " + std::to_string(_chunkPos[1]);
	//std::cout << output << std::endl;
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
