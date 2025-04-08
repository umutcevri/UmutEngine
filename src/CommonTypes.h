#pragma once

#include <sstream>
#include <fstream>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/string_cast.hpp>

#include <unordered_map>
#include <vector>

static std::string ReadFileStr(const std::string& filename)
{
    std::ifstream file(filename, std::ios::in | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file!");
    }

    std::ostringstream stream;
    stream << file.rdbuf();
    file.close();

    return stream.str();
}

struct Mesh
{
	uint32_t startIndex = 0;
	uint32_t indexCount = 0;
};

struct Vertex {
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec3 color;
    int diffuseTextureID = -1;

	glm::ivec4 boneIndices = glm::vec4(-1);

	glm::vec4 boneWeights = glm::vec4(0);

	glm::mat4 globalTransform = glm::mat4(1.0f);
};
