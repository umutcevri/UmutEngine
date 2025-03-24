#include "InputManager.h"

void InputManager::Update()
{
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