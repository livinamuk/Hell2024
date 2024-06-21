#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) out vec3 color;
layout (location = 1) out vec3 WorldPos;

layout(set = 0, binding = 0) readonly buffer CameraData {
    mat4 projection;
    mat4 projectionInverse;
    mat4 view;
    mat4 viewInverse;
	float viewportWidth;
	float viewportHeight;   
    float viewportOffsetX;
    float viewportOffsetY; 
	float clipSpaceXMin;
    float clipSpaceXMax;
    float clipSpaceYMin;
    float clipSpaceYMax;
	float finalImageColorContrast;
    float finalImageColorR;
    float finalImageColorG;
    float finalImageColorB;
} cameraData;

struct CloudPoint {
    float posX;
    float posY;
    float posZ;
    float normX;

    float normY;
    float normZ;
    float colorX;
    float colorY;

    float colorZ;
	float padding0;
	float padding1;
	float padding2;
};

//layout(set = 1, binding = 0) buffer PointCloud { CloudPoint data[]; } pointCloud;


layout(set = 1, binding = 0) buffer PointCloud { CloudPoint data[]; } pointCloud;

void main() {	

	const mat4 correction = mat4(1.0,  0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0,  0.0, 0.5, 0.0, 0.0,  0.0, 0.5, 1.0);
	mat4 proj = correction * cameraData.projection;
	mat4 view = cameraData.view;

	int i =  gl_VertexIndex;
	vec3 position = vec3(pointCloud.data[i].posX, pointCloud.data[i].posY, pointCloud.data[i].posZ);
	vec3 normal = vec3(pointCloud.data[i].normX, pointCloud.data[i].normY, pointCloud.data[i].normZ);
	
	color = vec3(position);

	WorldPos = position;

	gl_Position = proj * view * vec4(position, 1.0);
    gl_PointSize = 4.0;

	
   // rayQueryEXT rayQuery;
}
