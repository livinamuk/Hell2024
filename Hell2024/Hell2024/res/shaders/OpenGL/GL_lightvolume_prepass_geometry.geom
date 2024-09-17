#version 400 core
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

out vec3 WorldPos;
uniform mat4 shadowMatrices[6];

void main() {   

    for(int face = 0; face < 6; ++face) {
        gl_Layer = face; 
        for(int i = 0; i < 3; ++i){
            WorldPos = gl_in[i].gl_Position.rgb;
            gl_Position = shadowMatrices[face] * vec4(WorldPos, 1);
            EmitVertex();
        }    
        EndPrimitive();
    }
} 