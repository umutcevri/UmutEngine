#pragma once

#include <iostream>
#include <vector>
#include <fstream>
#include <deque>
#include <functional>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <unordered_set>
#include <array>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

static std::vector<char> ReadFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);

    if(!file.read(buffer.data(), fileSize))
	{
		throw std::runtime_error("Failed to read file!");
	}

    file.close();

    return buffer;
}

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function)
    {
        deletors.push_back(function);
    }

    void flush()
    {
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
        {
            (*it)();
        }

        deletors.clear();
    }
};

struct DrawQueue
{
    std::deque<std::function<void()>> drawCalls;

    void push_function(std::function<void()>&& function)
    {
        drawCalls.push_back(function);
    }

    void flush()
    {
        for (auto it = drawCalls.rbegin(); it != drawCalls.rend(); it++)
        {
            (*it)();
        }
        drawCalls.clear();
    };

};

struct Vertex {
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec3 color;
    int diffuseTextureID;
};

struct SceneData
{
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
    glm::mat4 lightSpaceMatrix;
    glm::vec4 lightPos;
};

struct Mesh
{
    uint32_t startIndex = 0;
	uint32_t indexCount = 0;
	bool isTransparent = false;
};

struct Object
{
	std::vector<glm::mat4> instances;
	std::vector<Mesh> meshes;
};

struct ObjectInstance
{
	glm::mat4 model;
};

struct GPUPushConstants
{
	glm::mat4 transform;
    glm::mat4 lightSpaceMatrix;
};

struct ShadowData
{
    glm::mat4 lightSpaceMatrix;
    glm::mat4 model;
};

std::vector<glm::vec3> cubeVertices = {
    // Front face
    glm::vec3(-0.5f, -0.5f,  0.5f), // Bottom-left
    glm::vec3(0.5f, -0.5f,  0.5f), // Bottom-right
    glm::vec3(0.5f,  0.5f,  0.5f), // Top-right
    glm::vec3(-0.5f,  0.5f,  0.5f), // Top-left

    // Back face
    glm::vec3(-0.5f, -0.5f, -0.5f), // Bottom-left
    glm::vec3(0.5f, -0.5f, -0.5f), // Bottom-right
    glm::vec3(0.5f,  0.5f, -0.5f), // Top-right
    glm::vec3(-0.5f,  0.5f, -0.5f)  // Top-left
};

std::vector<uint32_t> cubeIndices = {
    // Front face
   0, 1, 2, 2, 3, 0,
   // Back face
   4, 5, 6, 6, 7, 4,
   // Left face
   4, 0, 3, 3, 7, 4,
   // Right face
   1, 5, 6, 6, 2, 1,
   // Top face
   3, 2, 6, 6, 7, 3,
   // Bottom face
   4, 5, 1, 1, 0, 4
};