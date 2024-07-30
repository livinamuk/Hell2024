#version 460 core
in vec3 FragPos;

uniform vec3 lightPosition;
uniform float far_plane;

void main() {
    vec3 fragToLight = FragPos.xyz - lightPosition;
    float lightDistance = length(fragToLight);
    lightDistance = lightDistance / far_plane;
    gl_FragDepth = lightDistance;
}