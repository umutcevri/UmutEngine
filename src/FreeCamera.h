#pragma once

#include "Camera.h"

class FreeCamera : public Camera
{
	
public:
	using Camera::Camera;

	void Update(float deltaTime) override
	{
		Camera::Update(deltaTime);

		float velocity = MovementSpeed * deltaTime;

		Position += Right * InputManager::Get().moveInput.x * velocity;
		Position += Front * InputManager::Get().moveInput.y * velocity;
	}
};
