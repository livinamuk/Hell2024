#version 420

layout (location = 0) out vec4 FragColor;

//layout (binding = 0) uniform sampler2D inputTexture;
//layout (binding = 1) uniform sampler2D positionTexture;
//layout (binding = 2) uniform sampler2D previousPositionTexture;
//layout (binding = 3) uniform sampler2D positionTextureMotionBlur;
//layout (binding = 4) uniform sampler2D gbuffer2Tex;

//uniform sampler2D u_tColorTex; // In MotionBlurShader
layout (binding = 0) uniform sampler2D tex;
layout (binding = 1) uniform sampler2D previousFrameTex;
layout (binding = 2) uniform sampler2D velocityMapTex;

in vec2 TexCoords;

const int Samples = 64; //multiple of 2
float Intensity = 0.02;


vec4 DirectionalBlur(in vec2 UV, in vec2 Direction, in float Intensity, in sampler2D Texture)
{
    vec4 Color = vec4(0.0);  
    
    for (int i=1; i<=Samples/2; i++) {
        Color += texture(Texture, UV+float(i)*Intensity/float(Samples/2)*Direction);
        Color += texture(Texture, UV-float(i)*Intensity/float(Samples/2)*Direction);
    }
    return Color/float(Samples);    
}


const int samples = 35,
          LOD = 2,         // gaussian done on MIPmap at scale LOD
          sLOD = 1 << LOD; // tile size = 2^LOD
const float sigma = float(samples) * .25;

float gaussian(vec2 i) {
    return exp( -.5* dot(i/=sigma,i) ) / ( 6.28 * sigma*sigma );
}

vec4 gaussianBlur(sampler2D sp, vec2 U, vec2 scale) {
    vec4 O = vec4(0);  
    int s = samples/sLOD;    
    for ( int i = 0; i < s*s; i++ ) {
        vec2 d = vec2(i%s, i/s)*float(sLOD) - float(samples)/2.;
        O += gaussian(d) * textureLod( sp, U + scale * d , float(LOD) );
    }    
    return O / O.a;
}

void main() {

   
   vec4 pos = texture(tex, TexCoords);
   vec4 previousFrame = texture(previousFrameTex, TexCoords);
   vec4 velocityMap = texture(velocityMapTex, TexCoords);
 
    FragColor.rgb = vec3(1,0,0);
    FragColor.rgb = vec3(pos.rgb);
    //FragColor.rgb = vec3(TexCoords, 0);


    
    vec2 scale = vec2(0.005);
    vec3 screenSpaceVelocity = gaussianBlur(previousFrameTex, TexCoords, scale).rgb * 3;

    float weight = 0.0;
    vec3 sum = vec3(0.0, 0.0, 0.0);
    float sampleCount = 32.0;

    for(int k = 0; k < sampleCount; ++k) {
        float offset = float(k) / (sampleCount - 1.0);
        vec4 vSample = texture(tex, TexCoords + (screenSpaceVelocity.xy * offset));
        sum += (vSample.rgb * vSample.a);
        weight += vSample.a;
    }
     
    if (weight > 0.0) {
        sum /= weight;
    }
            
    vec4 input = texture(tex, TexCoords);
    FragColor.rgb = mix(sum, input.rgb, 0.35);
    FragColor.a = 1;
}