#version 460 core

layout (location = 0) out vec4 FragOut;
layout (binding = 0) uniform sampler2D previousDepthTexture;

in vec4 WorldPos;
uniform mat4 view;
uniform float nearPlane;
uniform float farPlane;
uniform float viewportWidth;
uniform float viewportHeight;

void main() {
 
    vec2 uv_screenspace = gl_FragCoord.xy / vec2(viewportWidth, viewportHeight);
    float previousDepth = texture2D(previousDepthTexture, uv_screenspace).r;

    float viewspaceDepth = (view * WorldPos).z;
    float normalizedDepth = (viewspaceDepth - (-farPlane)) / ((-nearPlane) - (-farPlane));    

    if (normalizedDepth >= previousDepth) {
        discard;
    }

    FragOut.r = normalizedDepth;
}
