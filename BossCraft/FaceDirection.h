#pragma once

enum FaceDirection
{
	NORTH = 0,
	SOUTH,
	EAST,
	WEST,
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