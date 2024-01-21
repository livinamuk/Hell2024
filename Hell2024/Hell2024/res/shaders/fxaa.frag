#version 420

layout (location = 0) out vec4 FragColor;
layout (binding = 0) uniform sampler2D inputTexture;

uniform float viewportWidth;
uniform float viewportHeight;

in vec2 TexCoords;
in vec2 offsetTL;
in vec2 offsetTR;
in vec2 offsetBL;
in vec2 offsetBR;
in vec2 texCoordOffset;

vec3 Fxaa(sampler2D tex) {

	float fxaaSpanMax = 8.0f;
	float fxaaReduceMin = 1.0f/128.0f;
	float fxaaReduceMul = 1.0f/8.0f;

	float scale = 0.05;
	fxaaSpanMax *= scale;
	fxaaReduceMin *= scale;
	fxaaReduceMul *= scale;

	vec3 luma = vec3(0.299, 0.587, 0.114);	
	float lumaTL = dot(luma, texture2D(tex, offsetTL).xyz);
	float lumaTR =  dot(luma, texture2D(tex, offsetTR).xyz);
	float lumaBL = dot(luma, texture2D(tex, offsetBL).xyz);
	float lumaBR = dot(luma, texture2D(tex, offsetBR).xyz);
	float lumaM  = dot(luma, texture2D(tex, TexCoords).xyz);

	vec2 dir;
	dir.x = -((lumaTL + lumaTR) - (lumaBL + lumaBR));
	dir.y = ((lumaTL + lumaBL) - (lumaTR + lumaBR));
	
	float dirReduce = max((lumaTL + lumaTR + lumaBL + lumaBR) * (fxaaReduceMul * 0.25), fxaaReduceMin);
	float inverseDirAdjustment = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
	
	dir = min(vec2(fxaaSpanMax, fxaaSpanMax), 
		max(vec2(-fxaaSpanMax, -fxaaSpanMax), dir * inverseDirAdjustment)) * texCoordOffset;

	vec3 result1 = (1.0/2.0) * (
		texture2D(inputTexture, TexCoords + (dir * vec2(1.0/3.0 - 0.5))).xyz +
		texture2D(inputTexture, TexCoords + (dir * vec2(2.0/3.0 - 0.5))).xyz);

	vec3 result2 = result1 * (1.0/2.0) + (1.0/4.0) * (
		texture2D(inputTexture, TexCoords + (dir * vec2(0.0/3.0 - 0.5))).xyz +
		texture2D(inputTexture, TexCoords + (dir * vec2(3.0/3.0 - 0.5))).xyz);

	float lumaMin = min(lumaM, min(min(lumaTL, lumaTR), min(lumaBL, lumaBR)));
	float lumaMax = max(lumaM, max(max(lumaTL, lumaTR), max(lumaBL, lumaBR)));
	float lumaResult2 = dot(luma, result2);
	
	if(lumaResult2 < lumaMin || lumaResult2 > lumaMax)
		return result1;
	else
		return result2;
}

void main() {
	vec3 result = Fxaa(inputTexture);
	//vec3 result = texture2D(inputTexture, TexCoords).xyz;
	FragColor = vec4(result, 1.0);
}