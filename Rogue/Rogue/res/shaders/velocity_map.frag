#version 420 core

in vec3 WorldPos;

out vec4 FragColor;

in vec4 v_vCorrectedPosScreenSpace;
in vec4 v_vCurrentPosScreenSpace;
in vec4 v_vPreviousPosScreenSpace;
//in vec3 Normal;

void main() {
    
    FragColor.rgb = vec3(WorldPos);
    FragColor.a = 1.0; 


    vec2 vWindowPosition = v_vCorrectedPosScreenSpace.xy / v_vCorrectedPosScreenSpace.w;
    vWindowPosition *= 0.5;
    vWindowPosition += 0.5;

    vec3 vScreenSpaceVelocity =
        ((v_vCurrentPosScreenSpace.xyz/v_vCurrentPosScreenSpace.w) -
         (v_vPreviousPosScreenSpace.xyz/v_vPreviousPosScreenSpace.w)) * 0.5;
         
    FragColor.rgb = vScreenSpaceVelocity * 2;
   // FragColor.rgb = Normal * 1;
   // FragColor.rgb = v_vCurrentPosScreenSpace.rgb;
   
   // FragColor.rgb = vec3(WorldPos);
}
