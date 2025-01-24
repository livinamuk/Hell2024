#version 400 core
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

in vec2 TexCoordVOut[3];
in vec3 attrNormalVOut[3];
in vec3 attrTangentVOut[3];
in vec3 attrBiTangentVOut[3];
out vec2 TexCoord;
out vec3 attrNormal;
out vec3 attrTangent;
out vec3 attrBiTangent;
out vec3 WorldPos;

uniform mat4 captureViewMatrix[6];

void main() {
    

    for(int face = 0; face < 6; ++face) {
        gl_Layer = face; 
        for(int i = 0; i < 3; ++i){
            WorldPos = gl_in[i].gl_Position.rgb;
            gl_Position = captureViewMatrix[face] * vec4(WorldPos, 1);
            attrNormal = attrNormalVOut[i];
            attrTangent = attrTangentVOut[i];
            attrBiTangent = attrBiTangentVOut[i];
            TexCoord = TexCoordVOut[i];
            EmitVertex();
        }    
        EndPrimitive();
    }
} 