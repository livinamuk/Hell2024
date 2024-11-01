#version 460 core

layout (location = 0) out vec4 FragOut;

in vec3 Color;

void main() {
	if (Color == vec3(0 ,0, 0)) {
		discard;
	}
    FragOut.rgb = Color;
	FragOut.a = 1.0;

	
	//if (Color == vec3(0 ,0, 0)) {
	//	FragOut.rgb = vec3(1, 1, 1);
	//}
}
