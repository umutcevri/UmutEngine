#pragma once

#include "Renderer.h"

#include "FreeCamera.h"

#include "InputManager.h"

#include "SceneManager.h"

#include <iostream>

class UEngine
{
	VkExtent2D windowExtent{ 1280 , 720 };
	SDL_Window* window;
	SDL_Event e;

	URenderer renderer;

	FreeCamera* freeCamera;

	bool bQuit = false;

	float deltaTime = 0.0f;
	float lastFrame = 0.0f;

public:
	void Run()
	{
		InitWindow();

		renderer.SetWindow(window);

		freeCamera = new FreeCamera(glm::vec3(0,0,0));

		renderer.SetCamera(freeCamera);

		LoadAssets();

		renderer.Init();

		lastFrame = (float)SDL_GetTicks64() / 1000.0f;

		while (!bQuit)
		{
			MainLoop();
		}

		renderer.Cleanup();

		SDL_DestroyWindow(window);
	}

private:
	void InitWindow()
	{
		SDL_Init(SDL_INIT_VIDEO);

		SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

		window = SDL_CreateWindow(
			"Vulkan Engine",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			windowExtent.width,
			windowExtent.height,
			window_flags);

		SDL_StartTextInput();
	}

	void MainLoop()
	{
		float currentFrame = (float)SDL_GetTicks64() / 1000.0f;
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		InputManager::Get().Update();

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
					renderer.renderDebugQuad = !renderer.renderDebugQuad;
				}
			}

		}

		UPhysics::Get().Update(deltaTime);

		freeCamera->Update(deltaTime);

		renderer.Draw(deltaTime);
	}

	void LoadAssets()
	{
		SceneManager::Get().LoadScene("scenes/Scene1.json");

		SceneManager::Get().texturePaths.push_back("assets/image.jpg");

		std::cout << "Assets loaded" << std::endl;
	}
};
