#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec4 outFragColor;

layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 2, binding = 0) uniform sampler2D baseColorTexture;
layout(set = 2, binding = 1) uniform sampler2D normalTexture;
layout(set = 2, binding = 2) uniform sampler2D rmaTexture;
layout(set = 2, binding = 3) uniform sampler2D depthTexture;
layout(set = 2, binding = 4) uniform sampler2D raytracingOutput;
layout(set = 2, binding = 5) uniform sampler2D positionTexture;
layout(set = 2, binding = 7) uniform sampler2D glassTexture;
layout(set = 2, binding = 8) uniform sampler2D emissiveTexture;



struct CameraData {
    mat4 projection;
    mat4 projectionInverse;
    mat4 view;
    mat4 viewInverse;
	float viewportWidth;
	float viewportHeight;
    float viewportOffsetX;
    float viewportOffsetY;
	float clipSpaceXMin;
    float clipSpaceXMax;
    float clipSpaceYMin;
    float clipSpaceYMax;
	float finalImageColorContrast;
    float finalImageColorR;
    float finalImageColorG;
    float finalImageColorB;
};

layout(set = 0, binding = 0) readonly buffer A {
    CameraData[4] data;
} cameraData;

const float PI = 3.14159265359;

const float time = 0;


struct Light {
	float posX;
	float posY;
	float posZ;
	float colorR;
	float colorG;
	float colorB;
	float strength;
	float radius;
};

layout(set = 0, binding = 4) readonly buffer B {Light data[];} lights;


/////////////
//         //
//   Fog   //

float getFogFactor(float d) {
    const float FogMax = 25.0;
    const float FogMin = 1.0;

    if (d>=FogMax) return 1;
    if (d<=FogMin) return 0;

    return 1 - (FogMax - d) / (FogMax - FogMin);
}

//////////////////////
//                  //
//   Tone mapping   //

vec3 filmic(vec3 x) {
  vec3 X = max(vec3(0.0), x - 0.004);
  vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
  return pow(result, vec3(2.2));
}

float filmic(float x) {
  float X = max(0.0, x - 0.004);
  float result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
  return pow(result, 2.2);
}

vec3 Tonemap_ACES(const vec3 x) { // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

//////////////
//          //
//   Noise  //

vec3 filmPixel(vec2 uv) {
    mat2x3 uvs = mat2x3(uv.xxx, uv.yyy) + mat2x3(vec3(0, 0.1, 0.2), vec3(0, 0.3, 0.4));
    return fract(sin(uvs * vec2(12.9898, 78.233) * time) * 43758.5453);
}

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

/////////////////////////
//                     //
//   Direct Lighting   //


float D_GGX(float NoH, float roughness) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
  float NoH2 = NoH * NoH;
  float b = (NoH2 * (alpha2 - 1.0) + 1.0);
  return alpha2 / (PI * b * b);
}

float G1_GGX_Schlick(float NdotV, float roughness) {
  //float r = roughness; // original
  float r = 0.5 + 0.5 * roughness; // Disney remapping
  float k = (r * r) / 2.0;
  float denom = NdotV * (1.0 - k) + k;
  return NdotV / denom;
}

float G_Smith(float NoV, float NoL, float roughness) {
  float g1_l = G1_GGX_Schlick(NoL, roughness);
  float g1_v = G1_GGX_Schlick(NoV, roughness);
  return g1_l * g1_v;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 microfacetBRDF(in vec3 L, in vec3 V, in vec3 N, in vec3 baseColor, in float metallicness, in float fresnelReflect, in float roughness, in vec3 WorldPos) {
  vec3 H = normalize(V + L); // half vector
  // all required dot products
  float NoV = clamp(dot(N, V), 0.0, 1.0);
  float NoL = clamp(dot(N, L), 0.0, 1.0);
  float NoH = clamp(dot(N, H), 0.0, 1.0);
  float VoH = clamp(dot(V, H), 0.0, 1.0);
  // F0 for dielectics in range [0.0, 0.16]
  // default FO is (0.16 * 0.5^2) = 0.04
  vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect));
  // f0 = vec3(0.125);
  // in case of metals, baseColor contains F0
  f0 = mix(f0, baseColor, metallicness);
  // specular microfacet (cook-torrance) BRDF
  vec3 F = fresnelSchlick(VoH, f0);
  float D = D_GGX(NoH, roughness);
  float G = G_Smith(NoV, NoL, roughness);
  vec3 spec = (D * G * F) / max(4.0 * NoV * NoL, 0.001);

  // diffuse
  vec3 notSpec = vec3(1.0) - F; // if not specular, use as diffuse
  notSpec *= 1.0 - metallicness; // no diffuse for metals
  vec3 diff = notSpec * baseColor / PI;
  spec *= 1.05;
  vec3 result = diff + spec;

  return result;
}

vec3 GetDirectLighting(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic) {
	float fresnelReflect = 1.0; // 0.5 is what they used for box, 1.0 for demon

	vec3 viewPos = cameraData.data[0].viewInverse[3].xyz;

	vec3 viewDir = normalize(viewPos - WorldPos);
	float lightRadiance = strength * 1;// * 1.25;
	vec3 lightDir = normalize(lightPos - WorldPos);
	float lightAttenuation = smoothstep(radius, 0, length(lightPos - WorldPos));
	// lightAttenuation = clamp(lightAttenuation, 0.0, 0.9); // THIS IS WRONG, but does stop super bright region around light source and doesn't seem to affect anything else...
	float irradiance = max(dot(lightDir, Normal), 0.0) ;
	irradiance *= lightAttenuation * lightRadiance;
	vec3 brdf = microfacetBRDF(lightDir, viewDir, Normal, baseColor, metallic, fresnelReflect, roughness, WorldPos);
	return brdf * irradiance * clamp(lightColor, 0, 1);
}





void main() {

	mat4 proj = cameraData.data[0].projection;
	mat4 view = cameraData.data[0].view;
	mat4 projectionInverse = cameraData.data[0].projectionInverse;
	mat4 viewInverse = cameraData.data[0].viewInverse;

	// Sample render targets
	vec3 baseColor = texture(baseColorTexture, texCoord).rgb;
	baseColor.rgb = pow(baseColor.rgb, vec3(2.2));
	vec3 normal = texture(normalTexture, texCoord).rgb;
	vec3 rma = texture(rmaTexture, texCoord).rgb;
	float z = texture(depthTexture, texCoord).x;// * 2.0f - 1.0f;
    vec3 raytracingOutputColor = texture(raytracingOutput, texCoord).rgb;

	// Reconstruct position from depth
	const mat4 correction = mat4(1.0,  0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0,  0.0, 0.5, 0.0, 0.0,  0.0, 0.5, 1.0);
	vec2 clipSpaceTexCoord = texCoord;
	vec4 clipSpacePosition = vec4(texCoord * 2.0 - 1.0, z, 1.0);
	vec4 viewSpacePosition = projectionInverse * clipSpacePosition;
	viewSpacePosition /= viewSpacePosition.w;
	vec4 worldSpacePosition = viewInverse * viewSpacePosition;
	vec3 WorldPos = worldSpacePosition.xyz;






	//Light light = lights.data[i];

	const vec3 lightPosition = vec3(3.2, 2.2, 3.5);
	const vec3 lightDirection = normalize(lightPosition - WorldPos);
	float NdotL = max(dot(normal, lightDirection), 0.0);
	vec3 color = vec3(NdotL);

	const vec3 lightColor = vec3(1.0, 0.7799999713897705, 0.5289999842643738);
	const float lightStrength = 1.2;
	const float lightRadius = 5.6;
	const float roughness = rma.r;
	const float metallic = rma.g;

	vec3 directLighting = GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal, WorldPos, baseColor, roughness, metallic);

	outFragColor.rgb = directLighting;

	vec3 viewPos = viewInverse[3].xyz;
	float d = distance(viewPos, WorldPos);
	float alpha = getFogFactor(d);
	vec3 FogColor = vec3(0.0);
	outFragColor.rgb = mix(outFragColor.rgb, FogColor, alpha);
	outFragColor.rgb = mix(outFragColor.rgb, Tonemap_ACES(outFragColor.rgb), 1.0);
	outFragColor.rgb = mix(outFragColor.rgb, Tonemap_ACES(outFragColor.rgb), 0.35);
	outFragColor.rgb = pow(outFragColor.rgb, vec3(1.0/2.2));

	// Noise
	/*vec2 uv = gl_FragCoord.xy / vec2(screenWidth, screenHeight);
	vec2 filmRes = vec2(screenWidth, screenHeight);
	vec2 coord = gl_FragCoord.xy;
	vec2 rest = modf(uv * filmRes, coord);
	vec3 noise00 = filmPixel(coord / filmRes);
	vec3 noise01 = filmPixel((coord + vec2(0, 1)) / filmRes);
	vec3 noise10 = filmPixel((coord + vec2(1, 0)) / filmRes);
	vec3 noise11 = filmPixel((coord + vec2(1, 1)) / filmRes);
	vec3 noise = mix(mix(noise00, noise01, rest.y), mix(noise10, noise11, rest.y), rest.x) * vec3(0.7, 0.6, 0.8);
	float noiseSpeed = 30.0;
	float x = rand(uv + rand(vec2(int(time * noiseSpeed), int(-time * noiseSpeed))));
	float noiseFactor = 0.04;

	// Vignette
	uv = gl_FragCoord.xy / vec2(screenWidth * 1, screenHeight * 1);
	uv *=  1.0 - uv.yx;
	float vig = uv.x*uv.y * 15.0;	// multiply with sth for intensity
	vig = pow(vig, 0.05);			// change pow for modifying the extend of the  vignette
	outFragColor.rgb *= vec3(vig);
	*/
	// Some more YOLO tone mapping

	outFragColor.rgb = mix(outFragColor.rgb, Tonemap_ACES(outFragColor.rgb), 0.995);

	// Add the noise
	//outFragColor.rgb = outFragColor.rgb + (x * -noiseFactor) + (noiseFactor / 2);

	// Contrast
	float contrast = 1.15;
	vec3 finalColor = outFragColor.rgb;
	outFragColor.rgb = outFragColor.rgb * contrast;

	// Brightness
	outFragColor.rgb -= vec3(0.010);

	//outFragColor.a = baseColor.a;

    outFragColor.a = 1;


	//outFragColor.rgb = baseColor;

	outFragColor.rgb = vec3(raytracingOutputColor);
	//outFragColor.rgb = vec3(raytracingOutputColor) * WorldPos;

	//outFragColor.r *= outFragColor.r * 3.5;
	//outFragColor.g *= 0.00;
	//outFragColor.b *= 0.00;
//	outFragColor.rgb = WorldPos;

//	outFragColor.rgb = baseColor.rgb;

	if (normal == vec3(0,0,0)) {
		outFragColor.rgb = vec3(0,0,0);
	}

	outFragColor.rgb = outFragColor.rgb;



	vec3 glassColor = texture(glassTexture, texCoord).rgb;



	vec3 emissiveColor = texture(emissiveTexture, texCoord).rgb;
	outFragColor.rgb += emissiveColor.rgb;

	//outFragColor.rgb = baseColor.rgb;
	outFragColor.rgb += glassColor.rgb;




	//outFragColor.rgb = vec3(baseColor);
}