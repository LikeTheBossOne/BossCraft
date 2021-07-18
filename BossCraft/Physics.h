#pragma once
#include <glm/vec3.hpp>

#include "RayCastHit.h"

class World;

class Physics
{
public:
	static bool RayCast(glm::vec3 rayPos, glm::vec3 rayDir, RayCastHit& hit, float dist, World* world);
};

