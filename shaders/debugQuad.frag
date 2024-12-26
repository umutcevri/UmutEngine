#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

void main() {
    // Example: Render UVs as a gradient color
    //outColor = vec4(fragUV, 0.0, 1.0);
    float depthValue = texture(texSampler, fragUV).r;
    outColor = vec4(vec3(depthValue), 1.0);
}