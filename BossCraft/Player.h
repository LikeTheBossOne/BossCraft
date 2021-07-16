#pragma once
#include <glm/vec3.hpp>
#include "CameraDirection.h"

class World;
class Camera;

class Player
{
public:
	Camera* _camera;
	World* _world;
	glm::vec3 _worldPos;

	Player(glm::vec3 pos);

	void ProcessKeyBoard(CameraDirection direction, float velocity, float dt);
	void ProcessLeftMouseClick();
	void ProcessRightMouseClick();

private:
	bool RayCast(float dist, glm::ivec3& outBlockPos);
};

