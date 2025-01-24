#version 330

in vec3 in_position;
in vec2 in_texcoord;

uniform int u_splitscreen_mode;

out vec2 TexCoords;
out vec2 focus;

void main(void)
{
	TexCoords = in_texcoord;
	gl_Position = vec4(in_position, 1.0);

	focus = vec2(0.5,0.5);			// fullscreen
	
	/*if (u_splitscreen_mode == 1)	// splitscreen p1
		focus = vec2(0.5,0.75);
	if (u_splitscreen_mode == 2)	// splitscreen p2
		focus = vec2(0.5,0.25);

	if (u_splitscreen_mode == 3)	// quadscreen p1
		focus = vec2(0.25,0.75);
	if (u_splitscreen_mode == 4)	// quadscreen p2
		focus = vec2(0.75,0.75);
	if (u_splitscreen_mode == 5)	// quadscreen p3
		focus = vec2(0.25,0.25);
	if (u_splitscreen_mode == 6)	// quadscreen p4
		focus = vec2(0.75,0.25);*/

}	