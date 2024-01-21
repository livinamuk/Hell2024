#version 420 core

in vec2 TexCoords;

layout (binding = 0) uniform sampler2D renderTexture;
layout (binding = 1) uniform sampler2D depthTexture;

layout (location = 0) out vec3 FragColor;	

uniform float screenWidth;
uniform float screenHeight;

#define PI  3.14159265

float focalDepth = 1;  //focal distance value in meters, but you may use autofocus option below
float focalLength = 10; //focal length in mm
float fstop = 5; //f-stop value
uniform bool showFocus;// = false; //show debug focus point and focal range (red = focal point, green = focal range)

uniform float znear ; //camera clipping start
uniform float zfar; //camera clipping end

//------------------------------------------
//user variables

int samples = 2;// = 3; //samples on the first ring
int rings = 2;// = 3; //ring count

bool manualdof = false; //manual dof calculation
float ndofstart = 0.15; //near dof blur start
float ndofdist = 2.0; //near dof blur falloff distance
float fdofstart = 1.0; //far dof blur start
float fdofdist = 3.0; //far dof blur falloff distance

float CoC = 0.03;//circle of confusion size in mm (35mm film = 0.03mm)

uniform bool vignetting;// = true; //use optical lens vignetting?
uniform float vignout;// = 1.5; //vignetting outer border
uniform float vignin;// = 0.0; //vignetting inner border
uniform float vignfade;// = 122.0; //f-stops till vignete fades

bool autofocus = true; //use autofocus in shader? disable if you use external focalDepth value
in vec2 focus; // autofocus point on screen (0.0,0.0 - left lower corner, 1.0,1.0 - upper right)
//uniform float maxblur;// = 1.0; //clamp value of max blur (0.0 = no blur,1.0 default)
float maxblur = 0.5;//1.0;

uniform float threshold;// = 0.5; //highlight threshold;
//uniform float gain;// = 2.0; //highlight gain;

uniform float bias;// = 0.5; //bokeh edge bias
uniform float fringe;// = 0.7; //bokeh chromatic aberration/fringing

bool noise = true; //use noise instead of pattern for sample dithering
float namount = 0.0001; //dither amount


/*
next part is experimental
not looking good with small sample and ring count
looks okay starting from samples = 4, rings = 4
*/

float feather = 0.4; //pentagon shape feather

//------------------------------------------


vec3 lottes(vec3 x) {
  const vec3 a = vec3(1.6);
  const vec3 d = vec3(0.977);
  const vec3 hdrMax = vec3(8.0);
  const vec3 midIn = vec3(0.18);
  const vec3 midOut = vec3(0.267);

  const vec3 b =
      (-pow(midIn, a) + pow(hdrMax, a) * midOut) /
      ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);
  const vec3 c =
      (pow(hdrMax, a * d) * pow(midIn, a) - pow(hdrMax, a) * pow(midIn, a * d) * midOut) /
      ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);

  return pow(x, a) / (pow(x, a * d) * b + c);
}



float penta(vec2 coords) //pentagonal shape
{
	float scale = float(rings) - 1.3;
	vec4  HS0 = vec4( 1.0,         0.0,         0.0,  1.0);
	vec4  HS1 = vec4( 0.309016994, 0.951056516, 0.0,  1.0);
	vec4  HS2 = vec4(-0.809016994, 0.587785252, 0.0,  1.0);
	vec4  HS3 = vec4(-0.809016994,-0.587785252, 0.0,  1.0);
	vec4  HS4 = vec4( 0.309016994,-0.951056516, 0.0,  1.0);
	vec4  HS5 = vec4( 0.0        ,0.0         , 1.0,  1.0);
	
	vec4  one = vec4( 1.0 );
	
	vec4 P = vec4((coords),vec2(scale, scale)); 
	
	vec4 dist = vec4(0.0);
	float inorout = -4.0;
	
	dist.x = dot( P, HS0 );
	dist.y = dot( P, HS1 );
	dist.z = dot( P, HS2 );
	dist.w = dot( P, HS3 );
	
	dist = smoothstep( -feather, feather, dist );
	
	inorout += dot( dist, one );
	
	dist.x = dot( P, HS4 );
	dist.y = HS5.w - abs( P.z );
	
	dist = smoothstep( -feather, feather, dist );
	inorout += dist.x;
	
	return clamp( inorout, 0.0, 1.0 );
}



vec3 color(vec2 coords,float blur) //processing the sample
{
	vec2 texel = vec2(1.0/screenWidth,1.0/screenHeight);
	vec3 col = vec3(0.0);
	
	col.r = texture2D(renderTexture,coords + vec2(0.0,1.0)*texel*fringe*blur).r;
	col.g = texture2D(renderTexture,coords + vec2(-0.866,-0.5)*texel*fringe*blur).g;
	col.b = texture2D(renderTexture,coords + vec2(0.866,-0.5)*texel*fringe*blur).b;
	
	vec3 lumcoeff = vec3(0.299,0.587,0.114);
	float lum = dot(col.rgb, lumcoeff);
	//float thresh = max((lum-threshold)*gain, 0.0);
	float thresh = max((lum-threshold), 0.0); // ^^^ YOU REMOVED THE GAIN. IT MADE A WEIRD DARKER AREA.
	return col+mix(vec3(0.0),col,0);
}

vec2 rand(vec2 coord) //generating noise/pattern texture for dithering
{
	vec2 texel = vec2(1.0/screenWidth,1.0/screenHeight);
	float noiseX = ((fract(1.0-coord.s*(screenWidth/2.0))*0.25)+(fract(coord.t*(screenHeight/2.0))*0.75))*2.0-1.0;
	float noiseY = ((fract(1.0-coord.s*(screenWidth/2.0))*0.75)+(fract(coord.t*(screenHeight/2.0))*0.25))*2.0-1.0;
	
	if (noise) {
		noiseX = clamp(fract(sin(dot(coord ,vec2(12.9898,78.233))) * 43758.5453),0.0,1.0)*2.0-1.0;
		noiseY = clamp(fract(sin(dot(coord ,vec2(12.9898,78.233)*2.0)) * 43758.5453),0.0,1.0)*2.0-1.0;
	}
	return vec2(noiseX,noiseY);
}

vec3 debugFocus(vec3 col, float blur, float depth)
{
	float edge = 0.002*depth; //distance based edge smoothing
	float m = clamp(smoothstep(0.0,edge,blur),0.0,1.0);
	float e = clamp(smoothstep(1.0-edge,1.0,blur),0.0,1.0);
	
	col = mix(col,vec3(1.0,0.5,0.0),(1.0-m)*0.6);
	col = mix(col,vec3(0.0,0.5,1.0),((1.0-e)-(1.0-m))*0.2);

	return col;
}

float linearize(float depth)
{
	return -zfar * znear / (depth * (zfar - znear) - zfar);
}



void main() 
{
	
	vec2 texel = vec2(1.0/screenWidth,1.0/screenHeight);
	
	float depth = linearize(texture2D(depthTexture, TexCoords).x);
	
	//focal plane calculation
	
	float fDepth = linearize(texture2D(depthTexture,focus).x);
	
	//dof blur factor calculation	
	float blur = 0.0;	
	float f = focalLength; //focal length in mm
	float d = fDepth*1000.0; //focal plane in mm
	float o = depth*1000.0; //depth in mm
		
	float a = (o*f)/(o-f); 
	float b = (d*f)/(d-f); 
	float c = (d-f)/(d*fstop*CoC); 
		
	blur = abs(a-b)*c;	
	blur = clamp(blur,0.0,1.0);
	
	// calculation of pattern for ditering	
	vec2 noise = rand(TexCoords)*namount*blur;
	
	// getting blur x and y step factor	
	float w = (1.0/screenWidth)*blur*maxblur+noise.x;
	float h = (1.0/screenHeight)*blur*maxblur+noise.y;
	
	// calculation of final color	
	vec3 col = vec3(0.0);
	
	if(blur < 0.05) {
		col = texture2D(renderTexture, TexCoords).rgb;
	}
	
	else {
		col = texture2D(renderTexture, TexCoords).rgb;
		float s = 1.0;
		int ringsamples;		
		for (int i = 1; i <= rings; i += 1) {   
			ringsamples = i * samples;			
			for (int j = 0 ; j < ringsamples ; j += 1) {
				float step = PI*2.0 / float(ringsamples);
				float pw = (cos(float(j)*step)*float(i));
				float ph = (sin(float(j)*step)*float(i));
			
				col += color(TexCoords + vec2(pw*w,ph*h),blur)*mix(1.0,(float(i))/(float(rings)),bias);  
				s += 1.0*mix(1.0,(float(i))/(float(rings)),bias);   
			}
		}
		col /= s; //divide by sample count
	}
	
	FragColor.rgb = col;
	//FragColor.rgb = debugFocus(col, blur, depth);
}