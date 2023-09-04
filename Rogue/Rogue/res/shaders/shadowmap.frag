#version 400 core
in vec4 FragPos;

uniform vec3 lightPosition;
uniform float far_plane;

void main()
{
    float lightDistance = length(FragPos.xyz - lightPosition);
    lightDistance = lightDistance / far_plane;
    gl_FragDepth = lightDistance;
}