#pragma once
#include <array>
#include <functional>
#include <glm/vec2.hpp>

enum FaceDirection;

enum class BlockTypes
{
	Undef,
	SnowGrass,
	
};

class BlockInfo
{
public:
	std::function<glm::vec2(FaceDirection direction)> GetTexture;
};

class BlockProvider
{
private:
	static std::array<BlockInfo, 256> _blocks;
	
public:
	static void Init();
	static glm::vec2 GetBlockTextureLocation(uint8_t block, FaceDirection direction);
};

