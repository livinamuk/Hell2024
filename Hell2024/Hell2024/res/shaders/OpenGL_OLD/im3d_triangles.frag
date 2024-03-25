#version 420

struct VertexData {
	float m_edgeDistance;
	float m_size;
	vec4 m_color;
};

in VertexData vData;
	
layout(location=0) out vec4 fResult;
	
void main() 
{
	fResult = vData.m_color;
		
	//#if   defined(LINES)
	//	float d = abs(vData.m_edgeDistance) / vData.m_size;
	//	d = smoothstep(1.0, 1.0 - (kAntialiasing / vData.m_size), d);
//		fResult.a *= d;
			
	//#elif defined(POINTS)
	//	float d = length(gl_PointCoord.xy - vec2(0.5));
	//	d = smoothstep(0.5, 0.5 - (kAntialiasing / vData.m_size), d);
//		fResult.a *= d;
//			
//	#endif		
}