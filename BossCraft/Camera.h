#pragma once
#include <glad/glad.h>
#include <glm/mat4x2.hpp>
#include <glm/ext/matrix_transform.hpp>

class World;

class Camera
{
public:
    constexpr static float DEFAULT_YAW = -90.f;
    constexpr static float DEFAULT_PITCH = 0.0f;
    constexpr static float DEFAULT_SENSITIVITY = 0.1f;
    constexpr static float DEFAULT_FOV = 45.0f;

    glm::vec3 _position;
    glm::vec3 _front;
    glm::vec3 _up;
    glm::vec3 _right;
    glm::vec3 _worldUp;

    float _yaw;
    float _pitch;
    float _mouseSensitivity;
    float _fov;
	
    Camera(glm::vec3 position = glm::vec3(0.f, 0.f, 0.f), glm::vec3 up = glm::vec3(0.f, 1.f, 0.f),
        float yaw = DEFAULT_YAW, float pitch = DEFAULT_PITCH);

    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

    glm::mat4 GetViewMatrix();
	
    void UpdatePos(glm::vec3 newPos);
	
    void ProcessMouseMovement(float xOffset, float yOffset, GLboolean constrainPitch);

    void ProcessMouseScroll(float yOffset);

private:
    void UpdateCameraVectors();
};
