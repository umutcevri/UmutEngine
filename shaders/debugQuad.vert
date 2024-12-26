#version 450
#extension GL_KHR_vulkan_glsl : enable

// No inputs or outputs necessary from Vulkan for the quad itself.
layout(location = 0) out vec2 fragUV;

void main() {
    // Hardcoded vertex positions and UVs for a full-screen quad
    vec2 positions[4] = vec2[](
        vec2(-1.0, -1.0), // Bottom-left
        vec2( 1.0, -1.0), // Bottom-right
        vec2(-1.0,  1.0), // Top-left
        vec2( 1.0,  1.0)  // Top-right
    );
    
    vec2 uvs[4] = vec2[](
        vec2(0.0, 0.0), // Bottom-left
        vec2(1.0, 0.0), // Bottom-right
        vec2(0.0, 1.0), // Top-left
        vec2(1.0, 1.0)  // Top-right
    );

    // gl_VertexID determines which vertex to output
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragUV = uvs[gl_VertexIndex];
}