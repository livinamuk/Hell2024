#version 420

struct VertexData {
	float m_edgeDistance;
	float m_size;
	vec4 m_color;
};

#define kAntialiasing 2.0

in VertexData vDataOut;
	
layout(location=0) out vec4 fResult;
	
void main() {
	fResult = vDataOut.m_color;
	float d = abs(vDataOut.m_edgeDistance) / vDataOut.m_size;
	d = smoothstep(1.0, 1.0 - (kAntialiasing / vDataOut.m_size), d);
	fResult.a *= d;				
}