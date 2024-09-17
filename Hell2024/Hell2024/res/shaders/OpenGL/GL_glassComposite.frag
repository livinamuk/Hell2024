#version 460

layout (location = 0) out vec4 FragColor;
layout (binding = 0) uniform sampler2D baseColorTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D rmaTexture;
layout (binding = 3) uniform sampler2D depthTexture;
layout (binding = 5) uniform samplerCube shadowMap[16];

in vec2 TexCoords;

uniform mat4 inverseProjection;
uniform mat4 inverseView;

uniform float clipSpaceXMin;
uniform float clipSpaceXMax;
uniform float clipSpaceYMin;
uniform float clipSpaceYMax;

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

layout(std430, binding = 2) readonly buffer Lights {
    Light lights[];
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

vec3 GetDirectLighting(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic) {
	float fresnelReflect = 1.0; // 0.5 is what they used for box, 1.0 for demon

	vec3 viewPos = inverseView[3].xyz;
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

float ShadowCalculation(samplerCube depthTex, vec3 lightPos, vec3 fragPos, vec3 viewPos, vec3 Normal) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float far_plane = 20.0; // you probably wanna send this as a uniform, or at least check it matches what you have in c++
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
}

float map(float value, float min1, float max1, float min2, float max2) {
	float perc = (value - min1) / (max1 - min1);
	return perc * (max2 - min2) + min2;
}


void main() {
	
	vec3 baseColor = texture(baseColorTexture, TexCoords).rgb;
	baseColor.rgb = pow(baseColor.rgb, vec3(2.2));
	vec3 normal = texture(normalTexture, TexCoords).rgb;
	vec3 rma = texture(rmaTexture, TexCoords).rgb;
	float roughness = rma.r;
	float metallic = rma.g;
		
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
	
	vec3 viewPos = inverseView[3].xyz;


	vec3 directLighting = vec3(0);

	for (int i = 0; i < 8; i++) {
		Light light = lights[i];	
		vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
		vec3 lightColor = vec3(light.colorR, light.colorG, light.colorB);
		float lightRadius = light.radius;
		float lightStrength = light.strength;
		const vec3 lightDirection = normalize(lightPosition - WorldPos); 
		vec3 ligthting = GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal, WorldPos, baseColor.rgb, roughness, metallic);
		float shadow = ShadowCalculation(shadowMap[i], lightPosition, WorldPos, viewPos, normal);
		directLighting += ligthting * vec3(shadow);
	}


	FragColor.rgb = directLighting;




	// FragColor.rgb = indirectLighting;    
	//float d = distance(viewPos, WorldPos);
	//float alpha = getFogFactor(d);
	//vec3 FogColor = vec3(0.0);
	//FragColor.rgb = mix(FragColor.rgb, FogColor, alpha);
	FragColor.rgb = mix(FragColor.rgb, Tonemap_ACES(FragColor.rgb), 1.0);
	FragColor.rgb = mix(FragColor.rgb, Tonemap_ACES(FragColor.rgb), 0.35);	
	FragColor.rgb = pow(FragColor.rgb, vec3(1.0/2.2)); 

	// Vignette         
	vec2 uv = TexCoords;
	uv *=  1.0 - uv.yx;           
	float vig = uv.x*uv.y * 15.0;	// multiply with sth for intensity    
	vig = pow(vig, 0.05);			// change pow for modifying the extend of the  vignette    
	FragColor.rgb *= vec3(vig);

	// Some more YOLO tone mapping
	FragColor.rgb = mix(FragColor.rgb, Tonemap_ACES(FragColor.rgb), 0.995);	
	
	// Add the noise
	// TO DO: FragColor.rgb = FragColor.rgb + (x * -noiseFactor) + (noiseFactor / 2);

	// Contrast
	float contrast = 1.15;
	vec3 finalColor = FragColor.rgb;
	FragColor.rgb = FragColor.rgb * contrast;

	// Brightness
	FragColor.rgb -= vec3(0.010);


	//FragColor.rgb = vec3(TexCoords, 0);

	
//	FragColor.rgb = vec3( baseColor);  
//	FragColor.rgb = vec3( WorldPos);  
	//FragColor.rgb = vec3( 1,1,0);  

//	FragColor.rgb = vec3( shadow);  
	//FragColor.g = 0;  
//	FragColor.b = 0;  
	FragColor.a = 1; 

}