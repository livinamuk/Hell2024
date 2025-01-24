#version 420 core
layout(early_fragment_tests) in;

layout (location = 0) out vec4 BaseColorOut;
//layout (location = 1) out vec4 NormalsOut;
layout (location = 2) out vec4 RMAOut;

layout (binding = 2) uniform sampler2D BLOOD_TEXTURE;

in vec2 TexCoords;

void main() {    

	vec4 bloodTexture  = texture(BLOOD_TEXTURE, vec2(TexCoords.s, TexCoords.t));
    
    if (bloodTexture.a < 0.1)
        discard;

    vec3 _TintColor = vec3(0.48, 0, 0);
    BaseColorOut.r = mix(_TintColor.r, _TintColor.r * 0.2, bloodTexture.b * 0.375) ;
    BaseColorOut.g = 0;
    BaseColorOut.b = 0;
    BaseColorOut.a = bloodTexture.a;
    
	//NormalsOut = vec4(0, 1, 0, 0);

	  BaseColorOut.r += 0.05;

    RMAOut.rgb = vec3(0.015 , 0.4, 1);

}