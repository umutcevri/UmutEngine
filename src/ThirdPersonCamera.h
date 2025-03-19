#pragma once

#include "Camera.h"

class ThirdPersonCamera : public Camera
{
	float armLength = 3.f;

public:
	glm::mat4 GetViewMatrix() override
    {
		glm::vec3 cameraPosition = Position - Front * armLength;

        return glm::lookAt(cameraPosition, cameraPosition + Front, Up);
    }
};
