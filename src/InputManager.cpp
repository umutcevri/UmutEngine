#include "InputManager.h"

#include <iostream>

void InputManager::Update()
{
	bQuit = false;
	increaseRenderDebugQuad = false;
	increaseCameraIndex = false;
	attack = false;


	while (SDL_PollEvent(&e) != 0)
	{
		if (e.type == SDL_QUIT)
			bQuit = true;

		if (e.type == SDL_KEYDOWN)
		{
			if (e.key.keysym.sym == SDLK_ESCAPE)
			{
				bQuit = true;
			}
			else if (e.key.keysym.sym == SDLK_m)
			{
				if (SDL_GetRelativeMouseMode() == SDL_TRUE)
					SDL_SetRelativeMouseMode(SDL_FALSE);
				else
					SDL_SetRelativeMouseMode(SDL_TRUE);
			}
			else if (e.key.keysym.sym == SDLK_o)
			{
				renderDebugQuad = !renderDebugQuad;
			}
			else if (e.key.keysym.sym == SDLK_n)
			{
				increaseCameraIndex = true;
			}
			else if (e.key.keysym.sym == SDLK_b)
			{
				increaseRenderDebugQuad = true;
			}
		}

		if (e.type == SDL_MOUSEBUTTONDOWN)
		{
			if (e.button.button == SDL_BUTTON_LEFT)
			{
				attack = true;
			}
		}

	}

	KeyboardInput();
	MouseInput();
}

void InputManager::KeyboardInput()
{
	const Uint8* keystate = SDL_GetKeyboardState(NULL);

	moveInput = glm::vec2(0);

	//press once
	jump = keystate[SDL_SCANCODE_SPACE] && !previousKeyStates[SDL_SCANCODE_SPACE];

	// adjust accordingly
	if (keystate[SDL_SCANCODE_W])
		moveInput.y += 1;
	if (keystate[SDL_SCANCODE_S])
		moveInput.y -= 1;
	if (keystate[SDL_SCANCODE_A])
		moveInput.x -= 1;
	if (keystate[SDL_SCANCODE_D])
		moveInput.x += 1;

	memcpy(previousKeyStates, keystate, SDL_NUM_SCANCODES);
}

void InputManager::MouseInput()
{
	int xPos, yPos;
	SDL_GetRelativeMouseState(&xPos, &yPos);

	lookInput = glm::vec2(xPos, yPos);
}