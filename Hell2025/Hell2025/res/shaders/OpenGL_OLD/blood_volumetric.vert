#version 430 core

layout (location = 0) in vec3 a_Position;
layout (location = 2) in vec2 a_Texcoord;

layout(location = 0) uniform mat4 u_MatrixProjection;
layout(location = 1) uniform mat4 u_MatrixView;
layout(location = 2) uniform mat4 u_MatrixWorld;
layout(location = 4) uniform float u_Time;

layout (binding = 0) uniform sampler2D u_PosTex;
layout (binding = 1) uniform sampler2D u_NormTex;

out vec3 v_WorldNormal;
out vec3 v_ViewDir;
out vec3 v_fragPos;


float LinearToGammaSpaceExact (float value) {
    if (value <= 0.0F)
        return 0.0F;
    else if (value <= 0.0031308F)
        return 12.92F * value;
    else if (value < 1.0F)
        return 1.055F * pow(value, 0.4166667F) - 0.055F;
    else
        return pow(value, 0.45454545F);
}

vec3 LinearToGammaSpace (vec3 linRGB) {
   return vec3(LinearToGammaSpaceExact(linRGB.r), LinearToGammaSpaceExact(linRGB.g), LinearToGammaSpaceExact(linRGB.b));
}

void main() {

    int u_NumOfFrames = 81;
    int u_Speed = 35;
    int u_BoundingMax = 144;
    int u_BoundingMin = 116;
    vec3 u_HeightOffset = vec3(-45.4, -26.17, 12.7);

    u_BoundingMax = 1;
    u_BoundingMin = -1;
    u_HeightOffset = vec3(0, 0, 0);

    float currentSpeed = 1.0f / (u_NumOfFrames / u_Speed);
    float timeInFrames = ((ceil(fract(-u_Time * currentSpeed) * u_NumOfFrames)) / u_NumOfFrames) + (1.0 / u_NumOfFrames);

    vec3 v = a_Position;
    vec2 uv = a_Texcoord;
    
    timeInFrames = 0.0;
	timeInFrames = u_Time;


    vec4 texturePos = textureLod(u_PosTex, vec2(uv.x, (timeInFrames + uv.y)), 0);
    vec4 textureNorm = textureLod(u_NormTex, vec2(uv.x, (timeInFrames + uv.y)), 0);

    v_WorldNormal = textureNorm.xzy * 2.0 - 1.0;  
    mat4 modelMatrix = inverse(transpose(u_MatrixWorld));

    mat3 m = mat3(u_MatrixWorld);
    mat3 t = mat3(cross(m[1], m[2]), cross(m[2], m[0]), cross(m[0], m[1])); // adjoint matrix
    v_WorldNormal = t * v_WorldNormal;

    mat3 normalMatrix = mat3(u_MatrixWorld);
    normalMatrix = inverse(normalMatrix);
    normalMatrix = transpose(normalMatrix);

    gl_Position =  u_MatrixProjection * u_MatrixView * u_MatrixWorld * vec4(texturePos.xzy, 1.0);

}