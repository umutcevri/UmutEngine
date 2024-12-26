#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec3 fragColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) flat out int outDiffuseTextureID;
layout (location = 3) out vec3 outNormal;
layout (location = 4) out vec3 fragPos;
layout (location = 5) out vec3 lightPos;
layout (location = 6) out vec4 fragPosLightSpace;

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec3 color;
    int diffuseTextureID;
};

struct ObjectInstance
{
	mat4 model;
};

struct SceneData
{
    mat4 projection;
    mat4 view;
    mat4 model;
    mat4 lightSpaceMatrix;
    vec4 lightPos;
};

layout(binding = 0, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

layout(binding = 3, std430) readonly buffer ObjectInstanceBuffer{
     ObjectInstance objectInstances[];
};

layout(binding = 2) uniform SceneDataUniformBuffer{
	 SceneData sceneData;
};

void main() {
    Vertex v = vertices[gl_VertexIndex];
    gl_Position = sceneData.projection * sceneData.view * objectInstances[gl_InstanceIndex].model *  vec4(v.position, 1.0);
    outUV.x = v.uv_x;
	outUV.y = v.uv_y;
    fragColor = v.color;
    outDiffuseTextureID = v.diffuseTextureID;
    outNormal = mat3(transpose(inverse(sceneData.model))) * v.normal;
    fragPos = vec3(sceneData.model * vec4(v.position, 1.0));
    lightPos = vec3(sceneData.lightPos);
    fragPosLightSpace = sceneData.lightSpaceMatrix * vec4(fragPos, 1.0);
}