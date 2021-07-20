#pragma once
#include <memory>
#include <string>
#include <glm/vec2.hpp>
class Chunk;

class ChunkResources
{
private:
	static std::string _saveFolder;
	
public:
	static void Init(std::string saveFolder);
	
	static void SaveChunk(std::shared_ptr<Chunk> chunk);

	uint8_t* LoadChunk(glm::ivec2 chunkPos);
};

