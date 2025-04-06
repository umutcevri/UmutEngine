#pragma once

#include "Renderer.h"

#include "FreeCamera.h"

#include "ThirdPersonCamera.h"

#include "InputManager.h"

#include "SceneManager.h"

#include <iostream>

#include "json.hpp"

using json = nlohmann::json;

class UEngine
{
	VkExtent2D windowExtent{ 1920 , 1080 };
	SDL_Window* window;

	URenderer renderer;

	bool bQuit = false;

	float deltaTime = 0.0f;
	float lastFrame = 0.0f;

public:
	void Run()
	{
		std::ifstream f("config/Config.json");
		json data = json::parse(f);

		windowExtent.width = data["resolution"][0];
		windowExtent.height = data["resolution"][1];

		InitWindow();

		renderer.SetWindow(window);

		LoadAssets();

		renderer.Init();

		lastFrame = static_cast<float>(SDL_GetTicks64()) / 1000.0f;

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
		float currentFrame = static_cast<float>(SDL_GetTicks64()) / 1000.0f;
		deltaTime = currentFrame - lastFrame;

		if (deltaTime <= 0)
			deltaTime = 0.001f;

		lastFrame = currentFrame;

		InputManager::Get().Update();
		
		bQuit = InputManager::Get().bQuit;

		renderer.renderDebugQuad = InputManager::Get().renderDebugQuad;

		renderer.cameraIndex += InputManager::Get().increaseCameraIndex;
			
		renderer.debugQuadTextureIndex += InputManager::Get().increaseRenderDebugQuad;

		if (renderer.debugQuadTextureIndex >= NUM_CASCADES)
			renderer.debugQuadTextureIndex = 0;

		UPhysics::Get().Update(deltaTime);

		renderer.Draw(deltaTime);
	}

	void LoadAssets()
	{
		SceneManager::Get().LoadScene("scenes/Scene1.json");

		SceneManager::Get().texturePaths.push_back("assets/image.jpg");

		std::cout << "Assets loaded" << std::endl;
	}
};
