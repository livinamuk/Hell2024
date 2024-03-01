#version 420

struct VertexData {
	float m_edgeDistance;
	float m_size;
	vec4 m_color;
};

in VertexData vData;

#define kAntialiasing 2.0
	
layout(location=0) out vec4 fResult;
	
void main() {
	fResult = vData.m_color;
	float d = length(gl_PointCoord.xy - vec2(0.5));
	d = smoothstep(0.5, 0.5 - (kAntialiasing / vData.m_size), d);
	fResult.a *= d;	
}