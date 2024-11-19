#version 450
#extension GL_KHR_vulkan_glsl : enable

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec3 color;
    int diffuseTextureID;
};

struct ShadowData
{
    mat4 lightSpaceMatrix;
    mat4 model;
};

layout(binding = 0, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

layout(binding = 1) uniform ShadowBuffer{
	ShadowData shadowData;
};

void main() {
    Vertex v = vertices[gl_VertexIndex];
    gl_Position = shadowData.lightSpaceMatrix * shadowData.model * vec4(v.position, 1.0);
}

