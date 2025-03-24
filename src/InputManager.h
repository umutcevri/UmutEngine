#pragma once

#include <glm/glm.hpp>

#include "SDL/SDL.h"

class InputManager
{
public:
	glm::vec2 moveInput;
	glm::vec2 lookInput;

	bool jump = false;

	static InputManager& Get()
	{
		static InputManager instance;
		return instance;
	}

	void Update();

private:
	void KeyboardInput();

	void MouseInput();

	Uint8 previousKeyStates[SDL_NUM_SCANCODES] = { 0 };

};