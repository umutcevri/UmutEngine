#version 450

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec3 color;
    int diffuseTextureID;
    vec4 boneIDs;
    vec4 boneWeights;
};

struct ObjectInstance
{
	mat4 model;
    mat4 boneTransforms[100];
    int currentAnimation;
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

    vec4 totalPosition = vec4(0.0f);

    if(objectInstances[gl_InstanceIndex].currentAnimation < 0)
	{
		totalPosition = vec4(v.position, 1.0f);
	}
    else
    {
        for(int i = 0 ; i < 4 ; i++)
        {
            if(v.boneIDs[i] == -1) 
                continue;

            if(v.boneIDs[i] >= 100) 
            {
                totalPosition = vec4(v.position, 1.0f);
                break;
            }

            vec4 localPosition = objectInstances[gl_InstanceIndex].boneTransforms[int(v.boneIDs[i])] * vec4(v.position, 1.0f);
            totalPosition += localPosition * v.boneWeights[i];
        }
    }

    gl_Position = shadowData.lightSpaceMatrix * objectInstances[gl_InstanceIndex].model * totalPosition;
}

