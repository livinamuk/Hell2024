#pragma once
#include "HellCommon.h"
#include "../API/OpenGL/Types/GL_shader.h"
#include "../Renderer/Types/Model.hpp"

struct VolumetricBloodSplatter {

	float m_CurrentTime = 0.0f;
	Transform m_transform;
    glm::vec3 m_front;

	int m_type = 9;

	VolumetricBloodSplatter(glm::vec3 position, glm::vec3 rotation, glm::vec3 front);
	void Update(float deltaTime);
	glm::mat4 GetModelMatrix();

    static GLuint s_buffer_mode_matrices;
	static GLuint s_vao;

	static unsigned int s_counter;
};
