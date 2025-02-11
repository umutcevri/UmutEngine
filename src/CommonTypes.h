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
#include <sstream>
#include <chrono>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

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

static std::string ReadFileStr(const std::string& filename)
{
    std::ifstream file(filename, std::ios::in | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file!");
    }

    std::ostringstream stream;
    stream << file.rdbuf();  // Read file into stream
    file.close();

    return stream.str();  // Return file content as a string
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
    glm::vec4 boneIDs = glm::vec4(-1);
	glm::vec4 boneWeights;
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

struct ObjectInstance
{
    glm::mat4 model;
	glm::mat4 boneTransforms[100];
	int currentAnimation = -1;
	float currentAnimationTime = 0.0f;
};

struct ObjectInstanceData
{
	glm::mat4 model;
	glm::mat4 boneTransforms[100];
	int currentAnimation = -1;
	int padding[3];
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

std::vector<Vertex> cubeVertices = {
    // Front face
    {
        glm::vec3(-0.5f, -0.5f,  0.5f), // position
        0.0f,                            // uv_x
        glm::vec3(0.0f,  0.0f,  1.0f),  // normal
        0.0f,                            // uv_y
        glm::vec3(0.5f, 0.5f, 0.5f),    // color
        -1                               // diffuseTextureID
    },
    {
        glm::vec3(0.5f, -0.5f,  0.5f),
        1.0f,
        glm::vec3(0.0f,  0.0f,  1.0f),
        0.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(0.5f,  0.5f,  0.5f),
        1.0f,
        glm::vec3(0.0f,  0.0f,  1.0f),
        1.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(-0.5f,  0.5f,  0.5f),
        0.0f,
        glm::vec3(0.0f,  0.0f,  1.0f),
        1.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },

    // Back face
    {
        glm::vec3(0.5f, -0.5f, -0.5f),
        0.0f,
        glm::vec3(0.0f,  0.0f, -1.0f),
        0.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(-0.5f, -0.5f, -0.5f),
        1.0f,
        glm::vec3(0.0f,  0.0f, -1.0f),
        0.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(-0.5f,  0.5f, -0.5f),
        1.0f,
        glm::vec3(0.0f,  0.0f, -1.0f),
        1.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(0.5f,  0.5f, -0.5f),
        0.0f,
        glm::vec3(0.0f,  0.0f, -1.0f),
        1.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },

    // Left face
    {
        glm::vec3(-0.5f, -0.5f, -0.5f),
        0.0f,
        glm::vec3(-1.0f,  0.0f,  0.0f),
        0.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(-0.5f, -0.5f,  0.5f),
        1.0f,
        glm::vec3(-1.0f,  0.0f,  0.0f),
        0.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(-0.5f,  0.5f,  0.5f),
        1.0f,
        glm::vec3(-1.0f,  0.0f,  0.0f),
        1.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(-0.5f,  0.5f, -0.5f),
        0.0f,
        glm::vec3(-1.0f,  0.0f,  0.0f),
        1.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },

    // Right face
    {
        glm::vec3(0.5f, -0.5f,  0.5f),
        0.0f,
        glm::vec3(1.0f,  0.0f,  0.0f),
        0.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(0.5f, -0.5f, -0.5f),
        1.0f,
        glm::vec3(1.0f,  0.0f,  0.0f),
        0.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(0.5f,  0.5f, -0.5f),
        1.0f,
        glm::vec3(1.0f,  0.0f,  0.0f),
        1.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(0.5f,  0.5f,  0.5f),
        0.0f,
        glm::vec3(1.0f,  0.0f,  0.0f),
        1.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },

    // Top face
    {
        glm::vec3(-0.5f,  0.5f,  0.5f),
        0.0f,
        glm::vec3(0.0f,  1.0f,  0.0f),
        0.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(0.5f,  0.5f,  0.5f),
        1.0f,
        glm::vec3(0.0f,  1.0f,  0.0f),
        0.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(0.5f,  0.5f, -0.5f),
        1.0f,
        glm::vec3(0.0f,  1.0f,  0.0f),
        1.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(-0.5f,  0.5f, -0.5f),
        0.0f,
        glm::vec3(0.0f,  1.0f,  0.0f),
        1.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },

    // Bottom face
    {
        glm::vec3(-0.5f, -0.5f, -0.5f),
        0.0f,
        glm::vec3(0.0f, -1.0f,  0.0f),
        0.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(0.5f, -0.5f, -0.5f),
        1.0f,
        glm::vec3(0.0f, -1.0f,  0.0f),
        0.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(0.5f, -0.5f,  0.5f),
        1.0f,
        glm::vec3(0.0f, -1.0f,  0.0f),
        1.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    },
    {
        glm::vec3(-0.5f, -0.5f,  0.5f),
        0.0f,
        glm::vec3(0.0f, -1.0f,  0.0f),
        1.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        -1
    }
};

std::vector<uint32_t> cubeIndices = {
    // Front face
    0, 1, 2, 2, 3, 0,

    // Back face
    4, 5, 6, 6, 7, 4,

    // Left face
    8, 9,10,10,11, 8,

    // Right face
   12,13,14,14,15,12,

   // Top face
  16,17,18,18,19,16,

  // Bottom face
 20,21,22,22,23,20
};