#include "Player.h"
#include "Camera.h"
#include "Physics.h"
#include "World.h"

Player::Player(glm::vec3 pos) : _worldPos(pos)
{
	_camera = new Camera(pos);
    _nextAllowedLeftClick = std::chrono::high_resolution_clock::now();
    _nextAllowedRightClick = std::chrono::high_resolution_clock::now();
}

void Player::ProcessKeyBoard(CameraDirection direction, float velocity, float dt)
{
    if (_world == NULL) return;
	
    if (direction == CameraDirection::Forward)
    {
        _worldPos += _camera->_front * velocity * dt;
    }
    if (direction == CameraDirection::Backward)
    {
        _worldPos -= _camera->_front * velocity * dt;
    }
    if (direction == CameraDirection::Left)
    {
        _worldPos -= _camera->_right * velocity * dt;
    }
    if (direction == CameraDirection::Right)
    {
        _worldPos += _camera->_right * velocity * dt;
    }

    _camera->UpdatePos(_worldPos);

    assert(_world != nullptr);
    _world->SetCenter(_worldPos);
}

void Player::ProcessLeftMouseClick()
{
    if (_world == NULL) return;

    auto now = std::chrono::high_resolution_clock::now();
    if (now < _nextAllowedLeftClick) return;

    glm::ivec3 blockToBreak;
	//if (RayCast(10, blockToBreak))
	//{
 //       //std::cout << "break block @ " << blockToBreak.x << "," << blockToBreak.y << "," << blockToBreak.z << std::endl;
 //       _world->UpdateBlockAtPos(blockToBreak, 0);
 //       _nextAllowedLeftClick += std::chrono::milliseconds(300);
	//}
    RayCastHit hit;
	if (Physics::RayCast(_camera->_position, _camera->_front, hit, 10, _world))
	{
        _world->UpdateBlockAtPos(hit.blockPosition, 0);
        _nextAllowedLeftClick = now + std::chrono::milliseconds(300);
	}
}

void Player::ProcessRightMouseClick()
{
    if (_world == NULL) return;

    auto now = std::chrono::high_resolution_clock::now();
    if (now < _nextAllowedRightClick) return;
	
    RayCastHit hit;
    if (Physics::RayCast(_camera->_position, _camera->_front, hit, 10, _world))
    {
        _world->UpdateBlockAtPos(hit.blockPosition + hit.normal, 1);
        _nextAllowedRightClick = now + std::chrono::milliseconds(300);
    }
}
