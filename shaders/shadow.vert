#version 450

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec3 color;
    int diffuseTextureID;
    ivec4 boneIndices;
	vec4 boneWeights;
    mat4 globalTransform;
};

struct EntityInstance
{
	mat4 model;
    int boneTransformBufferIndex;
};

struct BoneTransformData
{
    mat4 boneTransforms[200];
};

struct ShadowData
{
    mat4 model;
};

layout(binding = 0, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

layout(binding = 1) uniform ShadowBuffer{
	ShadowData shadowData;
};

layout(binding = 2, std430) readonly buffer EntityInstanceBuffer{
     EntityInstance entityInstances[];
};

layout(binding = 3, std430) readonly buffer BoneTransformBuffer{
     BoneTransformData boneTransforms[];
};

layout(push_constant) uniform PushConstant{
    mat4 lightSpaceMatrix;
} pc;

void main() {
    Vertex v = vertices[gl_VertexIndex];

    vec4 totalPosition = vec4(0,0,0,0);

    if(entityInstances[gl_InstanceIndex].boneTransformBufferIndex == -1)
	{
		totalPosition = vec4(v.position, 1.0f);
	}
    else
    {
        for(int i = 0 ; i < 4 ; i++)
        {
            if(v.boneIndices[i] == -1 || v.boneWeights[i] == 0.0f)
            {
                continue;
            }

            mat4 boneTransform = boneTransforms[entityInstances[gl_InstanceIndex].boneTransformBufferIndex].boneTransforms[v.boneIndices[i]];
           
            vec4 localPosition = boneTransform * vec4(v.position, 1.0f);
            totalPosition += localPosition * v.boneWeights[i];
        }
    }

     if(totalPosition.w == 0.0) {
        totalPosition.w = 1.0;
    }

    gl_Position = pc.lightSpaceMatrix * entityInstances[gl_InstanceIndex].model * v.globalTransform * totalPosition;
}

