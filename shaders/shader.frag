#version 450

layout(location = 0) in vec3 fragColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) flat in int inDiffuseTextureID;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler[256];

void main() {
    if(inDiffuseTextureID != -1)
    {
        //float depthValue = texture(texSampler[255], inUV).r;
        //outColor = vec4(vec3(depthValue), 1.0);
        outColor = texture(texSampler[inDiffuseTextureID], inUV);
    }
    else
    {
		outColor = vec4(fragColor, 1.0);
	}
    
}