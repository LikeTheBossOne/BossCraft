#include "Chunk.h"
#include <glad/glad.h>
#include <glm/mat4x4.hpp>
#include "Shader.h"
#include "Camera.h"
#include "ChunkMesh.h"
#include "FaceDirection.h"
#include "World.h"

Chunk::Chunk(glm::vec2 chunkPos, World* owningWorld) : _chunkPos(chunkPos), _world(owningWorld)
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

	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	_mesh = new ChunkMesh
	{
		0,
		0,
		0,
		VAO,
		VBO,
		EBO,
		(float*)malloc(sizeof(buffers.data)),
		(uint16_t*)malloc(sizeof(buffers.indices)),
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
	if (_isDirty && _meshMutex.try_lock())
	{
		// First reset mesh data.
		_mesh->vertexCount = 0;
		_mesh->dataIndex = 0;
		_mesh->indicesIndex = 0;

		//TODO: In the future, find a better way then mem-copying all of buffers.data. Maybe malloc would be better
		memcpy(_mesh->dataBuffer, buffers.data, sizeof(buffers.data));
		memcpy(_mesh->indexBuffer, buffers.indices, sizeof(buffers.indices));

		// Unlock after use
		_meshMutex.unlock();
	}
}


/**
 * Generates a ChunkMesh. Run on worker thread.
 */
void Chunk::GenerateMesh()
{
	if (!_isDirty || _meshIsLoaded)
	{
		return;
	}

	if (_meshMutex.try_lock())
	{
		//TODO: make sure this is in the right order. This should maybe happen in the render function instead.
		
		// loop over blocks in chunk
		for (unsigned x = 0; x < CHUNK_WIDTH; x++)
		{
			for (unsigned y = 0; y < CHUNK_HEIGHT; y++)
			{
				for (unsigned z = 0; z < CHUNK_WIDTH; z++)
				{
					glm::ivec3 pos(x, y, z);
					glm::ivec3 wPos = pos + glm::ivec3(_chunkPos[0] * CHUNK_WIDTH, 0, _chunkPos[1] * CHUNK_WIDTH);
					unsigned int data = _data[PositionToIndex(x, y, z)];

					if (data != 0)
					{
						// loop over directions
						for (int d = 0; d < 6; d++)
						{
							glm::ivec3 dirVec = DIRECTION_VEC[d];
							glm::ivec3 neighbor = pos + dirVec;
							glm::ivec3 wNeighbor = wPos + dirVec;

							bool visible = false;

							if (BlockInChunkBounds(neighbor))
							{
								// determine if block is transparent (0 = transparent block)
								visible = (_data[PositionToIndex(neighbor.x, neighbor.y, neighbor.z)] == 0);
							}
							else
							{
								//TODO: I could refactor this to make it thread-safe by first checking if the wNeighbor chunk was loaded.
								//Otherwise, it will continue to error on GetBlockAtAbsPos because the chunk isn't loaded.
								visible = (_world->BlockInRenderDistance(wNeighbor) && _world->GetBlockAtAbsPos(wNeighbor) == 0);
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
		BufferMesh();
		
		_isDirty = false;
		_meshIsLoaded = true;
		_meshMutex.unlock();
	}
}

/**
 * Renders the ChunkMesh. MUST BE RUN ON MAIN THREAD
 */
void Chunk::RenderMesh()
{
	if (!_meshIsLoaded || !_meshMutex.try_lock())
	{
		return;
	}

	//
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _world->_textureID);
	
	//BufferMesh();
	

	Camera* camera = _world->GetCamera();
	glm::mat4 projection = glm::perspective(glm::radians(camera->_fov), 800.f / 600.f, 0.1f, 300.0f);

	_world->GetShader()->UniSetMat4f("view", camera->GetViewMatrix());
	_world->GetShader()->UniSetMat4f("projection", projection);
	
	//
	
	Shader* shader = _world->GetShader();
	shader->Use();
	glm::mat4 model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(_chunkPos[0] * CHUNK_WIDTH, 0, _chunkPos[1] * CHUNK_WIDTH));

	shader->UniSetMat4f("model", model);
	shader->UniSetFloat("redColor", (_chunkPos[0] + 6) / 12.f);
	shader->UniSetFloat("greenColor", (_chunkPos[1] + 6) / 12.f);

	glActiveTexture(GL_TEXTURE0); //
	glBindTexture(GL_TEXTURE_2D, _world->_textureID); //
	
	glBindVertexArray(_mesh->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, _mesh->VBO);
	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _mesh->EBO);
	glDrawElements(GL_TRIANGLES, _mesh->indicesIndex, GL_UNSIGNED_SHORT, 0);

	glBindVertexArray(0);
	
	_meshMutex.unlock();
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
		if (_chunkPos[0] == 0 && _chunkPos[1] == 0 && blockPos.x == 1 && blockPos.y == 1 && blockPos.z == 0 && direction == 2)
		{
			std::cout << vertex[0] << ", " << vertex[1] << ", " << vertex[2] << std::endl;
		}
	}

	for (int i = 0; i < 6; i++)
	{
		_mesh->indexBuffer[_mesh->indicesIndex++] = _mesh->vertexCount + FACE_INDICES[i];
	}

	_mesh->vertexCount += 4;
}

void Chunk::BufferMesh()
{
	glBindBuffer(GL_ARRAY_BUFFER, _mesh->VBO);
	glBufferData(GL_ARRAY_BUFFER, _mesh->dataIndex * sizeof(float), _mesh->dataBuffer, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _mesh->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, _mesh->indicesIndex * sizeof(uint16_t), _mesh->indexBuffer, GL_STATIC_DRAW);
}


unsigned Chunk::GetDataAtPosition(glm::vec3 pos)
{
	return _data[PositionToIndex(pos)];
}
