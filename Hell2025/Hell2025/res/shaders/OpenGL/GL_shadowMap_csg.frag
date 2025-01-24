#version 460 core
in vec3 FragPos;

uniform vec3 lightPosition;
uniform float farPlane;

void main() {
    vec3 fragToLight = FragPos.xyz - lightPosition;
    float lightDistance = length(fragToLight);
    lightDistance = lightDistance / farPlane;
    gl_FragDepth = lightDistance;
}