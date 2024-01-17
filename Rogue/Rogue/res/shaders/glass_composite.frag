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
layout (binding = 2) uniform sampler2D blurTexture0;
layout (binding = 3) uniform sampler2D blurTexture1;
layout (binding = 4) uniform sampler2D blurTexture2;
layout (binding = 5) uniform sampler2D blurTexture3;
layout (binding = 6) uniform sampler2D depthTexture;

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

float LinearizeDepth(vec2 uv) {
	float n = 1.0; // camera z near
	float f = 100.0; // camera z far
	float z = texture2D(depthTexture, uv).x;
	return (2.0 * n) / (f + n - z * (f - n));
}

void main() {

	
    vec3 lightingColor = texture(mainImageTexture, TexCoords).rgb;
    vec3 glassColor = texture(glassTexture, TexCoords).rgb;


	vec3 final = lightingColor;

	vec3 emissive = vec3(0);
	emissive += texture(blurTexture0, TexCoords).rgb;
	emissive += texture(blurTexture1, TexCoords).rgb;
	emissive += texture(blurTexture2, TexCoords).rgb;
	emissive += texture(blurTexture3, TexCoords).rgb;
	 

	if (glassColor != vec3(0,0,0)) {
		final = glassColor;
	}
	
	final += emissive;
	FragColor = vec4(final, 1);

	//float linearDepth = LinearizeDepth(TexCoords);
	//FragColor = vec4(vec3(linearDepth), 1);
   
}