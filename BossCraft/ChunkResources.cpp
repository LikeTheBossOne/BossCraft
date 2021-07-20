#include "ChunkResources.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <Windows.h>

#include "Chunk.h"

std::string ChunkResources::_saveFolder;

void ChunkResources::Init(std::string saveFolder)
{
	_saveFolder = saveFolder;
	if (!CreateDirectoryA(saveFolder.c_str(), NULL))
	{
		int err = GetLastError();
		if (err == ERROR_PATH_NOT_FOUND)
		{
			std::cout << "CHUNK-SAVE ERROR: " << saveFolder << "not found!" << std::endl;
			return;
		}
	}
}

void ChunkResources::SaveChunk(std::shared_ptr<Chunk> chunk)
{
	assert(chunk != NULL);
	
	std::stringstream fileName;
	fileName << "chunk" << chunk->_chunkPos[0] << "-" << chunk->_chunkPos[1];
	{
		std::ofstream outStream(_saveFolder + "/" + fileName.str(), std::ios::binary);
		if (outStream.good())
		{
			outStream.write(reinterpret_cast<char*>(chunk->_data.data()), sizeof(char) * CHUNK_VOLUME);
		}
		
	}
}

uint8_t* ChunkResources::LoadChunk(glm::ivec2 chunkPos)
{
	std::stringstream fileName;
	fileName << "chunk" << chunkPos[0] << "-" << chunkPos[1];
	{
		std::ifstream inStream(_saveFolder + "/" + fileName.str(), std::ios::binary);

		if (!inStream.good())
		{
			return NULL;
		}
		
		uint8_t* data = new uint8_t[CHUNK_VOLUME];
		inStream.read(reinterpret_cast<char*>(data), sizeof(char) * CHUNK_VOLUME);
		return data;
	}
}
