#version 450
out vec4 out_colour;
in vec2 blurTextureCoords[9];
layout (binding = 0) uniform sampler2D image;

void main(void){
	out_colour = vec4(0.0);
	out_colour += texture(image, blurTextureCoords[0]) * 0.02853226260337099;
    out_colour += texture(image, blurTextureCoords[1]) * 0.06723453549491201;
    out_colour += texture(image, blurTextureCoords[2]) * 0.1240093299792275;
    out_colour += texture(image, blurTextureCoords[3]) * 0.1790438646174162;
    out_colour += texture(image, blurTextureCoords[4]) * 0.2023600146101466;
    out_colour += texture(image, blurTextureCoords[5]) * 0.1790438646174162;
    out_colour += texture(image, blurTextureCoords[6]) * 0.1240093299792275;
    out_colour += texture(image, blurTextureCoords[7]) * 0.06723453549491201;
    out_colour += texture(image, blurTextureCoords[8]) * 0.02853226260337099;
}

/*
void main2(void){
	out_colour = vec4(0.0);
	out_colour += texture(image, blurTextureCoords[0]) * 0.0093;
    out_colour += texture(image, blurTextureCoords[1]) * 0.028002;
    out_colour += texture(image, blurTextureCoords[2]) * 0.065984;
    out_colour += texture(image, blurTextureCoords[3]) * 0.121703;
    out_colour += texture(image, blurTextureCoords[4]) * 0.175713;
    out_colour += texture(image, blurTextureCoords[5]) * 0.198596;
    out_colour += texture(image, blurTextureCoords[6]) * 0.175713;
    out_colour += texture(image, blurTextureCoords[7]) * 0.121703;
    out_colour += texture(image, blurTextureCoords[8]) * 0.065984;
    out_colour += texture(image, blurTextureCoords[9]) * 0.028002;
    out_colour += texture(image, blurTextureCoords[10]) * 0.0093;
}*/