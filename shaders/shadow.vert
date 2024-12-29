#version 450

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

layout(binding = 3, std430) readonly buffer ObjectInstanceBuffer{
     ObjectInstance objectInstances[];
};

void main() {
    Vertex v = vertices[gl_VertexIndex];
    gl_Position = shadowData.lightSpaceMatrix * objectInstances[gl_InstanceIndex].model * vec4(v.position, 1.0);
}

