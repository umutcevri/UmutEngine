#version 450

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

layout(binding = 3, std430) readonly buffer EntityInstanceBuffer{
     EntityInstance entityInstances[];
};

layout(binding = 4, std430) readonly buffer BoneTransformBuffer{
     BoneTransformData boneTransforms[];
};

layout(binding = 2) uniform SceneDataUniformBuffer{
	 SceneData sceneData;
};

void main() {

    Vertex v = vertices[gl_VertexIndex];

    vec4 totalPosition = vec4(0,0,0,0);
    vec3 skinnedNormal = vec3(0.0);

    if(entityInstances[gl_InstanceIndex].boneTransformBufferIndex == -1)
	{
		totalPosition = vec4(v.position, 1.0f);
        skinnedNormal = v.normal;
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

            mat3 boneMatrix3x3 = mat3(boneTransform);
            vec3 localNormal = boneMatrix3x3 * v.normal;
            skinnedNormal += localNormal * v.boneWeights[i];
        }
    }

     if(totalPosition.w == 0.0) {
        totalPosition.w = 1.0;
    }

    gl_Position = sceneData.projection * sceneData.view * entityInstances[gl_InstanceIndex].model * totalPosition;
    outUV.x = v.uv_x;
	outUV.y = v.uv_y;
    fragColor = v.color;
    outDiffuseTextureID = v.diffuseTextureID;
    outNormal = transpose(inverse(mat3(entityInstances[gl_InstanceIndex].model))) * skinnedNormal;
    fragPos = vec3(entityInstances[gl_InstanceIndex].model * totalPosition);
    lightPos = vec3(sceneData.lightPos);
    fragPosLightSpace = sceneData.lightSpaceMatrix * vec4(fragPos, 1.0);

} 