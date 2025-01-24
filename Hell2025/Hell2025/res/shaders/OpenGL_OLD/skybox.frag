#version 420 core

layout (location = 0) out vec4 FragColor;
layout (binding = 0) uniform samplerCube cubeMap;

in vec3 TexCoords;

void contrastAdjust( inout vec4 color, in float c) {
    float t = 0.5 - c * 0.5; 
    color.rgb = color.rgb * c + t;
}

void main() {		
    FragColor.rgb = texture(cubeMap, TexCoords).rgb;
    FragColor.rgb *= 0.3;
    FragColor.g *= 0.5;
    FragColor.b *= 0.1;
    FragColor.a = 1.0;
//	FragColor.rgb = vec3(0,0,0);

//	contrastAdjust(FragColor, 2.0);
    //   FragColor.rgb = texture(cubeMap, TexCoords).rgb;
}
