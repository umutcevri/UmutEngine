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

float ShadowCalculation(vec4 _fragPosLightSpace)
{
    vec3 projCoords = _fragPosLightSpace.xyz / _fragPosLightSpace.w;
    vec2 shadow_map_coord = projCoords.xy * 0.5 + 0.5;
    float closestDepth = texture(texSampler[255], shadow_map_coord).r;
    float currentDepth = projCoords.z;
    
    vec3 normal = normalize(inNormal);
    vec3 lightDir = normalize(inLightPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(texSampler[255], 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(texSampler[255], shadow_map_coord.xy + vec2(x, y) * texelSize).r; 
            shadow += (currentDepth + bias < pcfDepth)  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}


void main() {
    vec3 ambient = vec3(0.1);
    vec3 norm = normalize(inNormal);
    vec3 lightDir = normalize(inLightPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1,1,1) * 2.f;
    float bias = max(0.05 * (1.0 - dot(inNormal, lightDir)), 0.005);

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