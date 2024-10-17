#version 460

layout (location = 0) out vec4 FragColor;
layout (binding = 0) uniform sampler2D baseColorTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D rmaTexture;
layout (binding = 3) uniform sampler2D depthTexture;
layout (binding = 4) uniform sampler2D emissiveTexture;
layout (binding = 5) uniform sampler3D probeGridTexture;
//layout (binding = 6) uniform samplerCube shadowMap[16];
layout (binding = 6) uniform samplerCubeArray shadowMapArray;


uniform int probeSpaceWidth;
uniform int probeSpaceHeight;
uniform int probeSpaceDepth;
uniform float probeSpacing;
uniform vec3 lightVolumePosition;
uniform float time;
uniform int renderMode;
uniform int lightCount;

in vec2 TexCoords;

//uniform mat4 inverseProjection;
//uniform mat4 inverseView;

//uniform float clipSpaceXMin;
//uniform float clipSpaceXMax;
//uniform float clipSpaceYMin;
//uniform float clipSpaceYMax;

const float PI = 3.14159265359;

struct Light {
    float posX;
    float posY;
    float posZ;
    float colorR;
    float colorG;
    float colorB;
    float strength;
    float radius;
    int shadowMapIndex;
    int contributesToGI;
    float padding0;
    float padding1;
};

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

layout(std430, binding = 2) readonly buffer Lights {
    Light lights[];
};


layout(std430, binding = 16) readonly buffer CameraDataArray {
    CameraData cameraDataArray[];
};

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

/*
vec3 filmPixel(vec2 uv) {
    mat2x3 uvs = mat2x3(uv.xxx, uv.yyy) + mat2x3(vec3(0, 0.1, 0.2), vec3(0, 0.3, 0.4));
    return fract(sin(uvs * vec2(12.9898, 78.233) * time) * 43758.5453);
}*/

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

vec3 GetDirectLighting(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
	float fresnelReflect = 1.0; // 0.5 is what they used for box, 1.0 for demon

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

////////////////////////
//                    //
//   Shadow mapping   //

vec3 gridSamplingDisk[20] = vec3[](
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1),
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

/*float ShadowCalculation(samplerCube depthTex, vec3 lightPos, float lightRadius, vec3 fragPos, vec3 viewPos, vec3 Normal) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float far_plane = lightRadius; // far plane was hardcoded 20, you're gonna need to look into this at some point.
    float shadow = 0.0;
    vec3 lightDir = fragPos - lightPos;
    float bias = max(0.0125 * (1.0 - dot(Normal, lightDir)), 0.00125);
    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 200.0;
    for(int i = 0; i < samples; ++i)     {
        float closestDepth = texture(depthTex, fragToLight + gridSamplingDisk[i] * diskRadius).r;
        closestDepth *= far_plane;  // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
    return 1 - shadow;
}*/

float ShadowCalculation(int lightIndex, vec3 lightPos, float lightRadius, vec3 fragPos, vec3 viewPos, vec3 Normal) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float far_plane = lightRadius; // far plane was hardcoded to 20, you can pass it as an argument if needed
    float shadow = 0.0;
    vec3 lightDir = fragPos - lightPos;
    float bias = max(0.0125 * (1.0 - dot(Normal, normalize(lightDir))), 0.00125);  // Added normalize to lightDir
    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 200.0;
    // Sample the cubemap array for shadows
    for (int i = 0; i < samples; ++i) {
        // Sample the cubemap array with the light index and the current sampling offset
        float closestDepth = texture(shadowMapArray, vec4(fragToLight + gridSamplingDisk[i] * diskRadius, lightIndex)).r;
        closestDepth *= far_plane;  // Undo mapping [0;1]        
        // Apply bias and check if the fragment is in shadow
        if (currentDepth - bias > closestDepth) {
            shadow += 1.0;
        }
    }
    // Average the shadow results
    shadow /= float(samples);    
    // Return the final shadow factor (1 means fully lit, 0 means fully in shadow)
    return 1.0 - shadow;
}

float map(float value, float min1, float max1, float min2, float max2) {
	float perc = (value - min1) / (max1 - min1);
	return perc * (max2 - min2) + min2;
}


///////////////////////////
//                       //
//   Indirect Lighting   //

vec3 GetProbe(vec3 fragWorldPos, ivec3 offset, out float weight, vec3 Normal) {

	/*
	float probeWorldX = (x * probeSpacing) + lightVolumePosition.x;
	float probeWorldY = (y * probeSpacing) + lightVolumePosition.y;
	float probeWorldZ = (z * probeSpacing) + lightVolumePosition.z;
	vec3 probePosition = vec3(probeWorldX, probeWorldY, probeWorldZ);
	*/

	fragWorldPos.x -= lightVolumePosition.x;
	fragWorldPos.y -= lightVolumePosition.y;
	fragWorldPos.z -= lightVolumePosition.z;

    vec3 gridCoords = fragWorldPos / probeSpacing;
    ivec3 base = ivec3(floor(gridCoords));
    vec3 a = gridCoords - base;
    vec3 probe_color = texelFetch(probeGridTexture, base + offset, 0).rgb;
    vec3 probe_worldPos = (base + offset) * probeSpacing;
    //vec3 v = normalize(probe_worldPos - fragWorldPos); // TODO: no need to normalize if only checking sign
    vec3 v = (probe_worldPos - fragWorldPos);
    float vdotn = dot(v, Normal);
    vec3 weights = mix(1. - a, a, offset);
    if(vdotn > 0 && probe_color != vec3(0))
        weight = weights.x * weights.y * weights.z;
    else
        weight = 0.;
    return probe_color;
}

vec3 GetIndirectLighting(vec3 WorldPos, vec3 Normal) { // Interpolate visible probes
    float w;
    vec3 light;
    float sumW = 0.;
    vec3 indirectLighting = vec3(0.);
    light = GetProbe(WorldPos, ivec3(0, 0, 0), w, Normal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(0, 0, 1), w, Normal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(0, 1, 0), w, Normal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(0, 1, 1), w, Normal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(1, 0, 0), w, Normal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(1, 0, 1), w, Normal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(1, 1, 0), w, Normal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(1, 1, 1), w, Normal);
    indirectLighting += w * light;
    sumW += w;
    indirectLighting /= sumW;
    return indirectLighting;
}

//////////////
//          //
//   Noise  //

vec3 filmPixel(vec2 uv) {
    mat2x3 uvs = mat2x3(uv.xxx, uv.yyy) + mat2x3(vec3(0, 0.1, 0.2), vec3(0, 0.3, 0.4));
    return fract(sin(uvs * vec2(12.9898, 78.233) * time) * 43758.5453);
}

void main2() {

	vec3 normal = texture(normalTexture, TexCoords).rgb;
	FragColor.rgb = vec3( normal * 0.5);
}

bool IsPointInsideAABB(vec3 point, vec3 minExtent, vec3 maxExtent) {
    return all(greaterThanEqual(point, minExtent)) && all(lessThanEqual(point, maxExtent));
}

void main() {

	//int playerIndex = 1;
	int playerIndex = int(texture(normalTexture, TexCoords).a * 4 + 0.5);

	mat4 inverseProjection = cameraDataArray[playerIndex].projectionInverse;
	mat4 inverseView = cameraDataArray[playerIndex].viewInverse;
	vec3 viewPos = cameraDataArray[playerIndex].viewInverse[3].xyz;
	float clipSpaceXMin = cameraDataArray[playerIndex].clipSpaceXMin;
	float clipSpaceXMax = cameraDataArray[playerIndex].clipSpaceXMax;
	float clipSpaceYMin = cameraDataArray[playerIndex].clipSpaceYMin;
	float clipSpaceYMax = cameraDataArray[playerIndex].clipSpaceYMax;

	/*
	int playerIndex = int(texture(normalTexture, texCoord).a * 4 + 0.5);

	mat4 proj = cameraData.data[playerIndex].projection;
	mat4 view = cameraData.data[playerIndex].view;
	mat4 projectionInverse = cameraData.data[playerIndex].projectionInverse;
	mat4 viewInverse = cameraData.data[playerIndex].viewInverse;
	vec3 viewPos = cameraData.data[playerIndex].viewInverse[3].xyz;

	float clipSpaceXMin = cameraData.data[playerIndex].clipSpaceXMin;
	float clipSpaceXMax = cameraData.data[playerIndex].clipSpaceXMax;
	float clipSpaceYMin = cameraData.data[playerIndex].clipSpaceYMin;
	float clipSpaceYMax = cameraData.data[playerIndex].clipSpaceYMax;*/


	vec3 baseColor = texture(baseColorTexture, TexCoords).rgb;
	baseColor.rgb = pow(baseColor.rgb, vec3(2.2));
	vec3 normal = texture(normalTexture, TexCoords).rgb;
	vec3 rma = texture(rmaTexture, TexCoords).rgb;
	float roughness = rma.r;
	float metallic = rma.g;

	float test = texture(normalTexture, TexCoords).a;


	// Position from depth reconsturction
	float z = texture(depthTexture, TexCoords).x;
    vec2 clipSpaceTexCoord = TexCoords;
	clipSpaceTexCoord.x = map(clipSpaceTexCoord.x, clipSpaceXMin, clipSpaceXMax, 0.0, 1.0);
	clipSpaceTexCoord.y = map(clipSpaceTexCoord.y, clipSpaceYMin, clipSpaceYMax, 0.0, 1.0);
	vec4 clipSpacePosition = vec4(clipSpaceTexCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = inverseProjection * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    vec4 worldSpacePosition = inverseView * viewSpacePosition;
    vec3 WorldPos = worldSpacePosition.xyz;



	vec3 directLighting = vec3(0);

	for (int i = 0; i < lightCount; i++) {
		Light light = lights[i];
		vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
		vec3 lightColor = vec3(light.colorR, light.colorG, light.colorB);
		float lightRadius = light.radius;
		float lightStrength = light.strength;
		const vec3 lightDirection = normalize(lightPosition - WorldPos);
		vec3 ligthting = GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal, WorldPos, baseColor.rgb, roughness, metallic, viewPos);
		
		float shadow = 1;
		if (light.shadowMapIndex != -1) {
			shadow = ShadowCalculation(light.shadowMapIndex, lightPosition, lightRadius, WorldPos, viewPos, normal);
		}
		directLighting += ligthting * vec3(shadow);
	}


	FragColor.rgb = directLighting;

	// Indirect light
	if (renderMode == 0) {
		vec3 baseColor2 = texture(baseColorTexture, TexCoords).rgb;
		vec3 WorldPos2 = WorldPos + (normal * 0.01);
		vec3 indirectLighting = GetIndirectLighting(WorldPos2, normal);
		vec3 adjustedIndirectLighting = indirectLighting;
		float factor = min(1, roughness * 1.0);
		float factor2 = min(1, 1 - metallic * 1.0);
		float factor3 = min (factor, factor2);
		//adjustedIndirectLighting *= (0.45) * vec3(factor2);
		adjustedIndirectLighting *= (0.35) * vec3(factor2);
		adjustedIndirectLighting = max(adjustedIndirectLighting, vec3(0));
		adjustedIndirectLighting *= baseColor2.rgb * 1.0;
		FragColor.rgb += adjustedIndirectLighting * 0.1;
	}

	// Point cloud only
	if (renderMode == 2) {
		FragColor.rgb = vec3(0);
	}

	FragColor.rgb = mix(FragColor.rgb, Tonemap_ACES(FragColor.rgb), 1.0);
	FragColor.rgb = pow(FragColor.rgb, vec3(1.0/2.2));

	FragColor.rgb = mix(FragColor.rgb, Tonemap_ACES(FragColor.rgb), 0.1235);
	// Vignette
	vec2 uv = TexCoords;
	uv *=  1.0 - uv.yx;
	float vig = uv.x*uv.y * 15.0;	// multiply with sth for intensity
	vig = pow(vig, 0.05);			// change pow for modifying the extend of the  vignette
	FragColor.rgb *= vec3(vig);

	// Noise
	vec2 viewportSize = textureSize(baseColorTexture, 0) * 1.0;
	//vec2 uv = gl_FragCoord.xy / viewportSize;
	vec2 filmRes = viewportSize;
	vec2 coord = gl_FragCoord.xy;
	vec2 rest = modf(uv * filmRes, coord);
	vec3 noise00 = filmPixel(coord / filmRes);
	vec3 noise01 = filmPixel((coord + vec2(0, 1)) / filmRes);
	vec3 noise10 = filmPixel((coord + vec2(1, 0)) / filmRes);
	vec3 noise11 = filmPixel((coord + vec2(1, 1)) / filmRes);
	vec3 noise = mix(mix(noise00, noise01, rest.y), mix(noise10, noise11, rest.y), rest.x) * vec3(0.7, 0.6, 0.8);
	float noiseSpeed = 15.0;
	float x = rand(uv + rand(vec2(int(time * noiseSpeed), int(-time * noiseSpeed))));
	float noiseFactor = 0.05;



	// Some more YOLO tone mapping
	FragColor.rgb = mix(FragColor.rgb, Tonemap_ACES(FragColor.rgb), 0.95);

	// Add the noise
	FragColor.rgb = FragColor.rgb + (x * -noiseFactor) + (noiseFactor / 2);

	// Contrast
	float contrast = 1.15;
	vec3 finalColor = FragColor.rgb;
	FragColor.rgb = FragColor.rgb * contrast;

	// Brightness
	FragColor.rgb -= vec3(0.020);





//	FragColor.rgb = vec3( TexCoords, 0); ]
	//FragColor.rgb = vec3( texture(normalTexture, TexCoords).a);

	vec3 emissiveColor = texture(emissiveTexture, TexCoords).rgb;
//	FragColor.rgb =emissiveColor;

	FragColor.a = 1;

	float NOISE =  (x * -noiseFactor) + (noiseFactor / 2);
	//FragColor.rgb = vec3( NOISE);

	//FragColor.rgb = vec3( TexCoords, 0);
	
	//FragColor.rgb = vec3( normal);
	
	}