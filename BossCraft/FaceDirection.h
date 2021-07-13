#pragma once
#include "glm/vec3.hpp"

enum FaceDirection
{
	NORTH = 0,	// -z
	SOUTH,		// +z
	EAST,		// +x
	WEST,		// -x
	UP,
	DOWN
};

const glm::vec3 DIRECTION_VEC[6] = {
	{  0,  0, -1 },
	{  0,  0,  1 },
	{  1,  0,  0 },
	{ -1,  0,  0 },
	{  0,  1,  0 },
	{  0, -1,  0 },
};