#version 420 core

out vec4 FragColor;

layout (binding = 0) uniform sampler3D propgationGridTexture;

flat in int x;
flat in int y;
flat in int z;

void main() {
    vec3 probeColor = texelFetch(propgationGridTexture, ivec3(x,y,z), 0).rgb;

    if (length(probeColor) < 0.1) {
    //    discard;
        //probeColor = vec3(0,1,0);
       // probeColor += vec3(1.5);
    } 
    if (probeColor == vec3(-1)) {
    
        probeColor = vec3(0,1,0);
    }
    
      //  probeColor = vec3(0,1,0);
    
    FragColor = vec4(probeColor * 5, 1.0);
  //  FragColor = vec4(probeColor, 1.0);
}