#include "Camera.h"
#include "World.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) :
    _front(glm::vec3(0.0f, 0.0f, -1.0f)), _mouseSensitivity(DEFAULT_SENSITIVITY), _fov(DEFAULT_FOV)
{
    _position = position;
    _worldUp = up;
    _yaw = yaw;
    _pitch = pitch;
    UpdateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) :
    _front(glm::vec3(0.0f, 0.0f, -1.0f)), _mouseSensitivity(DEFAULT_SENSITIVITY), _fov(DEFAULT_FOV)
{
    _position = glm::vec3(posX, posY, posZ);
    _worldUp = glm::vec3(upX, upY, upZ);
    _yaw = yaw;
    _pitch = pitch;
    UpdateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix()
{
    // Own View Matrix calculation
    return glm::lookAt(_position, _position + _front, _up);
}

void Camera::UpdatePos(glm::vec3 newPos)
{
    _position = newPos;
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset, GLboolean constrainPitch)
{
    xOffset *= _mouseSensitivity;
    yOffset *= _mouseSensitivity;

    _yaw += xOffset;
    _pitch += yOffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
    {
        if (_pitch > 89.0f)
        {
            _pitch = 89.0f;
        }
        if (_pitch < -89.0f)
        {
            _pitch = -89.0f;
        }
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yOffset)
{
    _fov -= static_cast<float>(yOffset);
    if (_fov < 1.0f)
    {
        _fov = 1.0f;
    }
    if (_fov > 45.0f)
    {
        _fov = 45.0f;
    }
}

void Camera::UpdateCameraVectors()
{
    // Calc front vector
    glm::vec3 front;
    front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    front.y = sin(glm::radians(_pitch));
    front.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    _front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    _right = glm::normalize(glm::cross(_front, _worldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    _up = glm::normalize(glm::cross(_right, _front));
}