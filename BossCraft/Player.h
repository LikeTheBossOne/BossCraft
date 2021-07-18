#pragma once
#include <chrono>
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
	std::chrono::steady_clock::time_point _nextAllowedLeftClick;
	std::chrono::steady_clock::time_point _nextAllowedRightClick;

	Player(glm::vec3 pos);

	void ProcessKeyBoard(CameraDirection direction, float velocity, float dt);
	void ProcessLeftMouseClick();
	void ProcessRightMouseClick();
};

