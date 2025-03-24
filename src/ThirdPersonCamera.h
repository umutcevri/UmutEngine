#pragma once

#include "Camera.h"

class ThirdPersonCamera : public Camera
{
	float armLength;

	glm::vec3 offset;

public:
	using Camera::Camera;

	ThirdPersonCamera(float armLength, glm::vec3 offset)
	{
		this->armLength = armLength;
		this->offset = offset;
	}

	glm::mat4 GetViewMatrix() override
    {
		glm::vec3 cameraPosition = (Position + offset) - Front * armLength;

        return glm::lookAt(cameraPosition, cameraPosition + Front, Up);
    }
};
