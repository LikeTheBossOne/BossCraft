#pragma once

struct ChunkMesh
{
	unsigned int vertexCount;
	unsigned int dataIndex;
	unsigned int indicesIndex;
	unsigned int VAO;
	unsigned int VBO;
	unsigned int EBO;
	float* dataBuffer;
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

const float CUBE_VERTICES[] = {
	0, 0, 0,
	1, 0, 0,
	1, 1, 0,
	0, 1, 0,

	0, 0, 1,
	1, 0, 1,
	1, 1, 1,
	0, 1, 1
};

const float CUBE_UVS[] = {
	1, 0,
	0, 0,
	0, 1,
	1, 1
};