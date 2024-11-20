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

struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    glm::mat4 model;
};

struct AssetData
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
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