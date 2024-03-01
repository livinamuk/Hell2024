#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;

out vec3 TexCoords;

void main()
{
    TexCoords = aPos;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}