#version 420

layout (location = 0) out vec4 FragColor;

//layout (binding = 0) uniform sampler2D inputTexture;
//layout (binding = 1) uniform sampler2D positionTexture;
//layout (binding = 2) uniform sampler2D previousPositionTexture;
//layout (binding = 3) uniform sampler2D positionTextureMotionBlur;
//layout (binding = 4) uniform sampler2D gbuffer2Tex;

//uniform sampler2D u_tColorTex; // In MotionBlurShader
layout (binding = 0) uniform sampler2D tex;
layout (binding = 1) uniform sampler2D velocityMapTex;
layout (binding = 2) uniform sampler2D depthTexture;

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
   float fragmentDepth = texture(depthTexture, TexCoords).r;
   vec3 velocityMap = texture(velocityMapTex, TexCoords).rgb;
 
    FragColor.rgb = vec3(1,0,0);
    FragColor.rgb = vec3(pos.rgb);
    //FragColor.rgb = vec3(TexCoords, 0);

    
    vec2 scale = vec2(0.005);
    vec3 screenSpaceVelocity = gaussianBlur(velocityMapTex, TexCoords, scale).rgb * 1;
 //   screenSpaceVelocity = velocityMap * 3;

    //float weight = 0.0;
    vec3 sum = vec3(0.0, 0.0, 0.0);
    float sampleCount = 32.0;
    float count = 0;

    for(int k = 0; k < sampleCount; ++k) {
        float offset = float(k) / (sampleCount - 1.0);
        vec2 sampleTexCoords = TexCoords + (screenSpaceVelocity.xy * offset);
        float sampleDepth = texture(depthTexture, sampleTexCoords).r;

        // vec4 vSample = texture(tex, sampleTexCoords);
         //   weight += vSample.a;
        //    sum += (vSample.rgb * vSample.a);

        
        //if (sampleDepth > fragmentDepth) {
        
       // if (abs(sampleDepth - fragmentDepth) < 0.0001) {
           
             sum += texture(tex, sampleTexCoords).rgb;
             count++;
      // }
    }

    
    vec4 input = texture(tex, TexCoords);

    if (count > 0.0) {
        sum /= count;
    }
    else {
       // sum = input.rgb;
    }

    if (velocityMap == vec3(0)) {    
        sum = input.rgb;
    }
            
     
  //  if (weight > 0.0) {
  //      sum /= weight;
 //   }
            
    FragColor.rgb = mix(sum, input.rgb, 0.35);
    FragColor.a = 1;


  //  FragColor.rgb = vec3(velocityMap);
   
}