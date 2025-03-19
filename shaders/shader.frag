#version 450

layout(location = 0) in vec3 fragColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) flat in int inDiffuseTextureID;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inFragPos;
layout (location = 5) in vec3 inLightPos;
layout (location = 6) in vec4 fragPosLightSpace;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler[256];

layout(binding = 5) uniform sampler2DShadow shadowMap;

float ShadowCalculation(vec4 _fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    vec2 shadowMapCoord = projCoords.xy * 0.5 + 0.5;

    if(shadowMapCoord.x < 0.0 || shadowMapCoord.x > 1.0 ||
       shadowMapCoord.y < 0.0 || shadowMapCoord.y > 1.0)
    {
        return 0;
    }

    float shadow = texture(shadowMap, vec3(shadowMapCoord, projCoords.z));
        
    return shadow;
}


void main() {
    vec3 ambient = vec3(0.1);
    vec3 norm = normalize(inNormal);
    vec3 lightDir = normalize(inLightPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1,1,1) * 2.f;

    float shadow = ShadowCalculation(fragPosLightSpace);

    if(inDiffuseTextureID != -1)
    {
        vec3 result = ((1 - shadow) * diffuse + ambient) * texture(texSampler[inDiffuseTextureID], inUV).rgb;
        if(texture(texSampler[inDiffuseTextureID], inUV).a < 0.2)
        {
            discard;
        }

        outColor = vec4(result, texture(texSampler[inDiffuseTextureID], inUV).a);
    }
    else
    {
        vec3 result = ((1 - shadow) * diffuse + ambient) * fragColor;
        outColor = vec4(result, 1.0);
	}
    
}