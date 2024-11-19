#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec3 fragColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) flat out int outDiffuseTextureID;

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec3 color;
    int diffuseTextureID;
};

layout(binding = 0, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

layout( push_constant ) uniform constants
{	
	mat4 transform;
} PushConstants;

void main() {
    Vertex v = vertices[gl_VertexIndex];
    gl_Position = PushConstants.transform * vec4(v.position, 1.0);
    outUV.x = v.uv_x;
	outUV.y = v.uv_y;
    fragColor = v.color;
    outDiffuseTextureID = v.diffuseTextureID;
  
}