#pragma once
#include <memory>
#include <string>
#include <glm/vec2.hpp>

#include "Chunk.h"
class Chunk;

class ChunkResources
{
private:
	static std::string _saveFolder;
	
public:
	static void Init(std::string saveFolder);
	
	static void SaveChunk(std::shared_ptr<Chunk> chunk);

	static bool LoadChunk(glm::ivec2 chunkPos, std::array<uint8_t, CHUNK_VOLUME>* data);
};

