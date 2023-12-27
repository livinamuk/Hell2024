#version 420

struct Light {
	vec3 position;
	vec3 color;
	float strength;
	float radius;
};

layout (location = 0) out vec4 FragColor;
in vec2 TexCoords;

layout (binding = 0) uniform sampler2D mainImageTexture;
layout (binding = 1) uniform sampler2D glassTexture;

uniform mat4 projectionScene;
uniform mat4 projectionWeapon;
uniform mat4 inverseProjectionScene;
uniform mat4 inverseProjectionWeapon;
uniform mat4 view;
uniform mat4 inverseView;
uniform vec3 viewPos;
uniform Light lights[16];
uniform int lightsCount;
uniform float screenWidth;
uniform float screenHeight;
uniform float time;
uniform int mode;
uniform float propogationGridSpacing;



void main() {

	
    vec3 lightingColor = texture(mainImageTexture, TexCoords).rgb;
    vec3 glassColor = texture(glassTexture, TexCoords).rgb;

	vec3 final = lightingColor;

	if (glassColor != vec3(0,0,0)) {
		final = glassColor;
	}

	FragColor = vec4(final, 1);
   
}