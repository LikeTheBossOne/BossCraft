#pragma once

struct ChunkMesh
{
	ChunkMesh()
	{
		vertexCount = 0;
		dataIndex = 0;
		indicesIndex = 0;

		dataBuffer = new uint32_t[(CHUNK_VOLUME) * 4 * 6];
		indexBuffer = new uint16_t[(CHUNK_VOLUME) * 6 * 6];
	}

	ChunkMesh(ChunkMesh& other)
	{
		vertexCount = other.vertexCount;
		dataIndex = other.dataIndex;
		indicesIndex = other.indicesIndex;

		dataBuffer = new uint32_t[(CHUNK_VOLUME) * 4 * 6];
		memcpy(dataBuffer, other.dataBuffer, CHUNK_VOLUME * 4 * 6 * sizeof(uint32_t));
		indexBuffer = new uint16_t[(CHUNK_VOLUME) * 6 * 6];
		memcpy(indexBuffer, other.indexBuffer, CHUNK_VOLUME * 6 * 6 * sizeof(uint16_t));
	}

	~ChunkMesh()
	{
		delete[] dataBuffer;
		delete[] indexBuffer;
	}
	
	unsigned int vertexCount;
	unsigned int dataIndex;
	unsigned int indicesIndex;
	uint32_t* dataBuffer;
	uint16_t* indexBuffer;
};

const int FACE_INDICES[] = { 1, 0, 3, 1, 3, 2 };
const int UNIQUE_INDICES[] = { 1, 0, 5, 2 };
const int CUBE_INDICES[] = {
	1, 0, 3, 1, 3, 2, // north (-z)
	4, 5, 6, 4, 6, 7, // south (+z)
	5, 1, 2, 5, 2, 6, // east (+x)
	0, 4, 7, 0, 7, 3, // west (-x)
	2, 3, 7, 2, 7, 6, // top (+y)
	5, 4, 0, 5, 0, 1, // bottom (-y)
};

const uint8_t CUBE_VERTICES[] = {
	0, 0, 0,
	1, 0, 0,
	1, 1, 0,
	0, 1, 0,

	0, 0, 1,
	1, 0, 1,
	1, 1, 1,
	0, 1, 1
};

const uint8_t CUBE_UVS[] = {
	0, 1,
	1, 1,
	1, 0,
	0, 0
};