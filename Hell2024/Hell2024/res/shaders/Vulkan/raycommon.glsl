
const float PI = 3.14159265359; 

struct CameraData {
	mat4 proj;
	mat4 view;
	mat4 projInverse;
	mat4 viewInverse;
	vec4 viewPos;	
    int sizeOfVertex;
	int frameIndex;
	int inventoryOpen;
	int wallpaperALBIndex;
};

#define HIT_TYPE_UNDEFINED      0
#define HIT_TYPE_SOLID          1
#define HIT_TYPE_TRANSULUCENT   2
#define HIT_TYPE_MIRROR         3
#define HIT_TYPE_GLASS          4
#define HIT_TYPE_MISS           5

struct RayPayload {
	int hitType;
	vec3 color;
	vec3 normal;
    vec3 nextFactor;
    vec3 nextRayOrigin;
    vec3 nextRayDirection;
    uint seed;
    int bounce;
    int writeToImageStore;
    vec3 vertexNormal;
    float meshIndex;
    float alpha;
};

struct MousepickPayload {
	uint instanceIndex;
	uint primitiveIndex;
};


struct Payload {
    vec3 nextRayOrigin;
};

const float time = 1;

// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

// Compound versions of the hashing algorithm I whipped together.
uint hash( uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
uint hash( uvec3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
uint hash( uvec4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}

// Pseudo-random value in half-open range [0:1].
//float random( float x ) { return floatConstruct(hash(floatBitsToUint(x))); }
float random2( vec2  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
//float random( vec3  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
//float random( vec4  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
// Takes our seed, updates it, and returns a pseudorandom float in [0..1]

mat3 getNormalSpace(in vec3 normal) {
   vec3 someVec = vec3(1.0, 0.0, 0.0);
   float dd = dot(someVec, normal);
   vec3 tangent = vec3(0.0, 1.0, 0.0);
   if(1.0 - abs(dd) > 1e-6) {
     tangent = normalize(cross(someVec, normal));
   }
   vec3 bitangent = cross(normal, tangent);
   return mat3(tangent, bitangent, normal);
}

float randomFloat(vec2 co) {
	return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float nextRand(inout uint s) {
    s = (1664525u * s + 1013904223u);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

float random(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 randomDirInCone(vec3 hitpos, vec3 lightPos) 
{
    vec3 lightVector = normalize(lightPos - hitpos);		
    vec2 rng = vec2(0);		
    rng.x = random(vec2(hitpos.x, hitpos.y));
    rng.y = random(vec2(hitpos.y, hitpos.x));		
    uint a = uint(hitpos.x * hitpos.y * hitpos.z * 1000);	
    uint b = uint(hitpos.z * hitpos.x * hitpos.z * 1000);	
    //rng.x = nextRand(a);
    //rng.y = nextRand(b);
    vec3 light_dir       = lightVector;
    vec3 light_tangent   = normalize(cross(light_dir, vec3(0.0, 1.0, 0.0)));
    vec3 light_bitangent = normalize(cross(light_tangent, light_dir));
    float radius = 0.03; // light bulb radius
    float point_radius = radius * sqrt(rng.x);
    float point_angle  = rng.y * 2.0 * PI;
    vec2  disk_point   = vec2(point_radius * cos(point_angle), point_radius * sin(point_angle));    
    vec3 Wi = normalize(light_dir + disk_point.x * light_tangent + disk_point.y * light_bitangent);
    return Wi;
}

vec3 randomDirInCone2(vec3 hitpos, vec3 lightPos, float randomSeed, float lightSize) 
{
    vec3 lightVector = normalize(lightPos - hitpos);		
    vec2 rng = vec2(0);		
    rng.x = random(vec2(hitpos.x * randomSeed, hitpos.y * -randomSeed));
    rng.y = random(vec2(hitpos.y * randomSeed, hitpos.x * -randomSeed));		
    //uint a = uint(hitpos.x * hitpos.y * hitpos.z * 1241 );	
    //uint b = uint(hitpos.z * hitpos.x * hitpos.z * 4321 * -randomSeed);	
    //rng.x = nextRand(a);
    //rng.y = nextRand(b);
    vec3 light_dir       = lightVector;
    vec3 light_tangent   = normalize(cross(light_dir, vec3(0.0, 1.0, 0.0)));
    vec3 light_bitangent = normalize(cross(light_tangent, light_dir));
    float radius = lightSize; // light bulb radius
    float point_radius = radius * sqrt(rng.x);
    float point_angle  = rng.y * 2.0 * PI;
    vec2  disk_point   = vec2(point_radius * cos(point_angle), point_radius * sin(point_angle));    
    vec3 Wi = normalize(light_dir + disk_point.x * light_tangent + disk_point.y * light_bitangent);
    return Wi;
}

// from http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// Hacker's Delight, Henry S. Warren, 2001
float radicalInverse(uint bits) {
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}


vec2 hammersley(uint n, uint N) {
    return vec2((float(n) + 0.5) / float(N), radicalInverse(n + 1u));
}

// Hash Functions for GPU Rendering, Jarzynski et al.
// http://www.jcgt.org/published/0009/03/02/
vec3 random_pcg3d(uvec3 v) {
  v = v * 1664525u + 1013904223u;
  v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
  v ^= v >> 16u;
  v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
  return vec3(v) * (1.0/float(0xffffffffu));
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
} 

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


vec3 microfacetBRDF(in vec3 L, in vec3 V, in vec3 N, 
              in vec3 baseColor, in float metallicness, 
              in float fresnelReflect, in float roughness, out vec3 specularContribution) {
     
  vec3 H = normalize(V + L); // half vector

  // all required dot products
  float NoV = clamp(dot(N, V), 0.0, 1.0);
  float NoL = clamp(dot(N, L), 0.0, 1.0);
  float NoH = clamp(dot(N, H), 0.0, 1.0);
  float VoH = clamp(dot(V, H), 0.0, 1.0);     
  
  // F0 for dielectics in range [0.0, 0.16] 
  // default FO is (0.16 * 0.5^2) = 0.04
  vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect)); 
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
  
  specularContribution = spec;
  return diff + spec;
}


vec3 sampleMicrofacetBRDF(in vec3 V, in vec3 N, 
              in vec3 baseColor, in float metallicness, 
              in float fresnelReflect, in float roughness, in vec3 random, out vec3 nextFactor) 
{
  if(random.z > 0.5) {
    // diffuse case
    
    // important sampling diffuse
    // pdf = cos(theta) * sin(theta) / PI
    float theta = asin(sqrt(random.y));
    float phi = 2.0 * PI * random.x;
    // sampled indirect diffuse direction in normal space
    vec3 localDiffuseDir = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
    vec3 L = getNormalSpace(N) * localDiffuseDir;  
    
     // half vector
    vec3 H = normalize(V + L);
    float VoH = clamp(dot(V, H), 0.0, 1.0);     
    
    // F0 for dielectics in range [0.0, 0.16] 
    // default FO is (0.16 * 0.5^2) = 0.04
    vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect)); 
    // in case of metals, baseColor contains F0
    f0 = mix(f0, baseColor, metallicness);    
    vec3 F = fresnelSchlick(VoH, f0);
    
    vec3 notSpec = vec3(1.0) - F; // if not specular, use as diffuse
    notSpec *= 1.0 - metallicness; // no diffuse for metals
    vec3 diff = notSpec * baseColor; 
  
    nextFactor = notSpec * baseColor;
    nextFactor *= 2.0; // compensate for splitting diffuse and specular
    return L;
    
  } else {
    // specular case
    
    // important sample GGX
    // pdf = D * cos(theta) * sin(theta)
    float a = roughness * roughness;
    float theta = acos(sqrt((1.0 - random.y) / (1.0 + (a * a - 1.0) * random.y)));
    float phi = 2.0 * PI * random.x;
    
    vec3 localH = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
    vec3 H = getNormalSpace(N) * localH;  
    vec3 L = reflect(-V, H);

    // all required dot products
    float NoV = clamp(dot(N, V), 0.0, 1.0);
    float NoL = clamp(dot(N, L), 0.0, 1.0);
    float NoH = clamp(dot(N, H), 0.0, 1.0);
    float VoH = clamp(dot(V, H), 0.0, 1.0);     
    
    // F0 for dielectics in range [0.0, 0.16] 
    // default FO is (0.16 * 0.5^2) = 0.04
    vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect)); 
    // in case of metals, baseColor contains F0
    f0 = mix(f0, baseColor, metallicness);
  
    // specular microfacet (cook-torrance) BRDF
    vec3 F = fresnelSchlick(VoH, f0);
    float D = D_GGX(NoH, roughness);
    float G = G_Smith(NoV, NoL, roughness);
    nextFactor =  F * G * VoH / max((NoH * NoV), 0.001);
    nextFactor *= 2.0; // compensate for splitting diffuse and specular
    return L;
  } 
}


vec3 Tonemap_ACES(const vec3 x) {
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

// linear to sRGB
float toneMap(float c)
{
    if (c == 0.0) { // to sRGB gamma via filmic-like Reinhard-esque combination approximation
	    c = c / (c + .1667); // actually cancelled out the gamma correction!  Probably cheaper than even sRGB approx by itself.
		}	// considering I can't tell the difference, using full ACES+sRGB together seems quite wasteful computationally.
  	   else if (c == 1.1) { // ACES+sRGB, which I approximated above
	    c = ((c*(2.51*c+.03))/(c*(2.43*c+.59)+.14));
	    c = pow(c, 1./2.2); // to sRGBish gamut
        }
  	  else {
	    c = pow(c, 1./2.2);
        }
    return c;
}


// combined exposure, tone map, gamma correction
vec3 toneMap(vec3 crgb, float exposure) // exposure = white point
{
    vec4 c = vec4(crgb, exposure);
    for (int i = 4; i-- > 0; ) c[i] = toneMap(c[i]);
    // must compute the tonemap operator of the exposure level, although optimizes out at compile time, 
    // do it all in vec4 and then divide by alpha.
    return c.rgb / c.a;
}

#define WithQuickAndDirtyLuminancePreservation        

const float LuminancePreservationFactor = 1.0;

const float PI2 = 6.2831853071;

// Valid from 1000 to 40000 K (and additionally 0 for pure full white)
vec3 colorTemperatureToRGB(const in float temperature){
  // Values from: http://blenderartists.org/forum/showthread.php?270332-OSL-Goodness&p=2268693&viewfull=1#post2268693   
  mat3 m = (temperature <= 6500.0) ? mat3(vec3(0.0, -2902.1955373783176, -8257.7997278925690),
	                                      vec3(0.0, 1669.5803561666639, 2575.2827530017594),
	                                      vec3(1.0, 1.3302673723350029, 1.8993753891711275)) : 
	 								 mat3(vec3(1745.0425298314172, 1216.6168361476490, -8257.7997278925690),
   	                                      vec3(-2666.3474220535695, -2173.1012343082230, 2575.2827530017594),
	                                      vec3(0.55995389139931482, 0.70381203140554553, 1.8993753891711275)); 
  return mix(clamp(vec3(m[0] / (vec3(clamp(temperature, 1000.0, 40000.0)) + m[1]) + m[2]), vec3(0.0), vec3(1.0)), vec3(1.0), smoothstep(1000.0, 0.0, temperature));
}

float fog_exp2(const float dist, const float density) {
  const float LOG2 = -1.442695;
  float d = density * dist;
  return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
}

vec3 calculateFog(vec3 inputColor, vec3 fogColor, float fogDensity, float fogDistance) {
    float fogAmount = fog_exp2(fogDistance, fogDensity);
    return mix(inputColor, fogColor, fogAmount);
}


float calculateDoomFactor(vec3 fragPos, vec3 camPos, float fallOff)
{
    float distanceFromCamera = distance(fragPos, camPos);
    float doomFactor = 1;	
    if (distanceFromCamera > fallOff) {
        distanceFromCamera -= fallOff;
        distanceFromCamera *= 0.13;
        distanceFromCamera *= 1.2;
        doomFactor = (1 - distanceFromCamera);
       // doomFactor = clamp(doomFactor, 0.23, 1.0);
        doomFactor = clamp(doomFactor, 0.1, 1.0);
    }
    return doomFactor;
}
