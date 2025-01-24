#version 450

layout (location = 0) out vec4 outFragColor;


//in vec2 blurTextureCoords[9];
//layout (binding = 0) uniform sampler2D image;

layout (location = 1) in vec2 texCoord[9];

void main(void){

	//outFragColor =  vec4(texCoord, 0, 1);

	outFragColor =  vec4(1, 0, 0, 1);
//	discard;

	/*
	out_colour = vec4(0.0);
	out_colour += texture(image, blurTextureCoords[0]) * 0.02853226260337099;
    out_colour += texture(image, blurTextureCoords[1]) * 0.06723453549491201;
    out_colour += texture(image, blurTextureCoords[2]) * 0.1240093299792275;
    out_colour += texture(image, blurTextureCoords[3]) * 0.1790438646174162;
    out_colour += texture(image, blurTextureCoords[4]) * 0.2023600146101466;
    out_colour += texture(image, blurTextureCoords[5]) * 0.1790438646174162;
    out_colour += texture(image, blurTextureCoords[6]) * 0.1240093299792275;
    out_colour += texture(image, blurTextureCoords[7]) * 0.06723453549491201;
    out_colour += texture(image, blurTextureCoords[8]) * 0.02853226260337099;*/
}