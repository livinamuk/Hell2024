#include "MuzzleFlash.h"

MuzzleFlash::MuzzleFlash() {
}

void MuzzleFlash::SetFrameByTime(float time)
{
	auto dt = AnimationSeconds / static_cast<float>(CountRaw * CountColumn - 1);

	m_FrameIndex = (int)std::floorf(time / dt);
	m_Interpolate = (time - m_FrameIndex * dt) / dt;
	m_CurrentTime = time;
}

void MuzzleFlash::Init()
{
	CountRaw = 5;
	CountColumn = 4;
	AnimationSeconds = 1.0f;

	float vertices[] = {
		-0.5f, +0.5f, +0.0f, 0.0f, 0.0f, 0, 0, 1,
		+0.5f, +0.5f, +0.0f, 1.0f, 0.0f, 0, 0, 1,
		+0.5f, -0.5f, +0.0f, 1.0f, 1.0f, 0, 0, 1,
		-0.5f, -0.5f, +0.0f, 0.0f, 1.0f, 0, 0, 1
	};
	uint32_t indices[] = { 0, 1, 2, 0, 2, 3 };

	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	glGenBuffers(1, &m_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(float)));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(float)));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glGenBuffers(1, &m_IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glBindVertexArray(0);
}

void MuzzleFlash::CreateFlash(glm::vec3 worldPosition) {
	m_worldPos = worldPosition;
	m_CurrentTime = 0;
}

/*void MuzzleFlash::Update(float deltaTime)
{
	auto dt = AnimationSeconds / static_cast<float>(CountRaw * CountColumn - 1);

	m_FrameIndex = std::floorf(m_CurrentTime / dt);
	m_Interpolate = (m_CurrentTime - m_FrameIndex * dt) / dt;
	m_CurrentTime += deltaTime * 10;// 7.5f;
}*/

void MuzzleFlash::Draw(Shader* shader, Transform& global, float rotation)
{
	if (m_VAO == 0)
		this->Init();

	if (m_CurrentTime >= AnimationSeconds * 0.5)
		return;

	glDepthMask(false);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	shader->Use();
	shader->SetInt("u_FrameIndex", m_FrameIndex);
	shader->SetInt("u_CountRaw", CountRaw);
	shader->SetInt("u_CountColumn", CountColumn);
	shader->SetFloat("u_TimeLerp", m_Interpolate);

	//Transform rot;
	//rot.rotation.y = HELL_PI / 2;
	//rot.rotation.x = -HELL_PI / 2;

	Transform scale;
	scale.rotation.z = rotation;
	scale.scale = glm::vec3(0.25f, 0.125f, 1);
	scale.scale.x *= 0.75;
	scale.scale.y *= 0.75;

	//Transform rotTrans;
	//rotTrans.rotation.z = Util::RandomFloat(0, 3.14f);

	shader->SetMat4("u_MatrixWorld", global.to_mat4() * scale.to_mat4() );

	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glDepthMask(true);
	glDisable(GL_BLEND);
}