#include "BlockProvider.h"
#include "FaceDirection.h"

std::array<BlockInfo, 256> BlockProvider::_blocks;

void BlockProvider::Init()
{
	// 0 doesn't matter because it is air lol
	_blocks[0] = {
		[](FaceDirection direction)
		{
			return glm::ivec2(0, 0);
		}
	};

	// Grass
	_blocks[1] = {
		[](FaceDirection direction)
		{
			switch (direction)
			{
			case FaceDirection::UP:
				return glm::ivec2(2, 0);
			case FaceDirection::DOWN:
				return glm::ivec2(18, 1);
			default:
				return glm::ivec2(3, 0);
			}
		}
	};

	// Dirt
	_blocks[2] = {
		[](FaceDirection direction)
		{
			{
			return glm::ivec2(18, 1);
			}
		}
	};

	// Stone
	_blocks[3] = {
		[](FaceDirection direction)
		{
			{
			return glm::ivec2(19, 0);
			}
		}
	};

	
}

glm::vec2 BlockProvider::GetBlockTextureLocation(uint8_t block, FaceDirection direction)
{
	return _blocks[block].GetTexture(direction);
}
