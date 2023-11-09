#pragma once

#include "../Common.h"
#include "../Renderer/Shader.h"

class MuzzleFlash
{
public: // fields
	uint32_t                CountRaw;
	uint32_t                CountColumn;
	float                   AnimationSeconds;

	int32_t                 m_FrameIndex = 0;
	float                   m_Interpolate = 0.0f;
	float                   m_CurrentTime = 0.0f;

public: // methods
	MuzzleFlash();
	void Draw(Shader* shader, Transform& global, float rotation);
	//void Update(float deltaTime);
	void SetFrameByTime(float time);
	void Init();
	void CreateFlash(glm::vec3 worldPosition);
	glm::vec3 m_worldPos;

	GLuint m_VAO = 0;
	GLuint m_VBO;
	GLuint m_IBO;
};