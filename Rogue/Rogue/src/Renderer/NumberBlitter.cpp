#include "NumberBlitter.h"
#include <algorithm>
#include "../Core/GL.h"
//#include "Helpers/Util.h"
//#include "Helpers/AssetManager.h"

unsigned int NumberBlitter::VAO = 0;
unsigned int NumberBlitter::VBO = 0;
unsigned int NumberBlitter::currentCharIndex = 0;
float NumberBlitter::s_blitTimer = 0;
float NumberBlitter::s_blitSpeed = 50;
float NumberBlitter::s_waitTimer = 0;
float NumberBlitter::s_timeToWait = 2;
bool NumberBlitter::s_centerText = true;
std::string NumberBlitter::s_NumSheet = "1234567890/";
std::string NumberBlitter::s_textToBlit = "1234";

void NumberBlitter::UpdateBlitter(float deltaTime)
{
	// Main timer
	/*s_blitTimer += deltaTime * s_blitSpeed;
	currentCharIndex = s_blitTimer;
	currentCharIndex = std::min(currentCharIndex, (unsigned int)s_textToBlit.size());

	if (s_blitSpeed == -1) {
		currentCharIndex = (unsigned int)s_textToBlit.size();
		return;
	}

	if (s_timeToWait == -1)
		return;

	// Wait time
	if (currentCharIndex == (unsigned int)s_textToBlit.size())
		s_waitTimer += deltaTime;
	if (s_waitTimer > s_timeToWait)
		s_textToBlit = "";*/
}

void NumberBlitter::DrawTextBlit(const char* text, int xScreenCoord, int yScreenCoord, int renderWidth, int renderHeight, float scale, glm::vec3 color, bool leftJustified)
{
	float screenWidth = GL::GetWindowWidth();// 1920.0f * 1.5;// *(CoreGL::s_currentWidth / renderWidth); // Have to hard code this coz otherwise the text is the same size in pixels regardless of resolution.
	float screenHeight = GL::GetWindowHeight();// 1080.0f * 1.5;
	//float widths[11] = { 9, 15, 15, 17, 15, 16, 15, 16, 15, 16, 12};
	float textureWidth = 161;
	static float textScale = 1;
	float cursor_X = 0;
	float cursor_Y = -1;

	cursor_X = (xScreenCoord / screenWidth) * 2 - 1;
	cursor_Y = (yScreenCoord / screenHeight) * 2 - 1;

	cursor_Y *= -1;

	//struct Vertex2D {
	//	glm::vec2 position;
	//	glm::vec2 uv;
	//};

	std::vector<Vertex> vertices;
	char character;
	float charWidth, charBegin;

	for (int i = 0; i < strlen(text); i++)
	{
		if (leftJustified)
			character = text[i];
		else
			character = text[strlen(text)-1-i];

		float charHeight = 34;

		if (character == '1') {
			charWidth = 9;
			charBegin = 0;
		}
		else if (character == '2') {
			charWidth = 15;
			charBegin = 9;
		}
		else if (character == '3') {
			charWidth = 15;
			charBegin = 24;
		}
		else if (character == '4') {
			charWidth = 17;
			charBegin = 39;
		}
		else if (character == '5') {
			charWidth = 15;
			charBegin = 56;
		}
		else if (character == '6') {
			charWidth = 16;
			charBegin = 71;
		}
		else if (character == '7') {
			charWidth = 15;
			charBegin = 87;
		}
		else if (character == '8') {
			charWidth = 16;
			charBegin = 102;
		}
		else if (character == '9') {
			charWidth = 15;
			charBegin = 118;
		}
		else if (character == '0') {
			charWidth = 16;
			charBegin = 133;
		}
		else {
			charWidth = 12;
			charBegin = 149;
		}

		float tex_coord_L = (charBegin / textureWidth);
		float tex_coord_R = ((charBegin + charWidth) / textureWidth);

		//std::cout << i << ": " << character << " " << tex_coord_L << ", " << tex_coord_R << "\n";

		charWidth *= scale;
		charHeight *= scale;
		charBegin *= scale;

		float w = charWidth / (screenWidth / 2) * (GL::GetWindowWidth() / renderWidth);
		float h = charHeight / (screenHeight / 2) * (GL::GetWindowHeight() / renderHeight);

		if (!leftJustified)
			cursor_X -= w;

		Vertex v1 = { glm::vec3(cursor_X, cursor_Y, 0), NRM_Y_UP, glm::vec2(tex_coord_L, 0) };
		Vertex v2 = { glm::vec3(cursor_X, cursor_Y - h, 0), NRM_Y_UP, glm::vec2(tex_coord_L, 1) };
		Vertex v3 = { glm::vec3(cursor_X + w, cursor_Y, 0), NRM_Y_UP, glm::vec2(tex_coord_R, 0) };
		Vertex v4 = { glm::vec3(cursor_X + w, cursor_Y - h, 0), NRM_Y_UP, glm::vec2(tex_coord_R, 1) };

		vertices.push_back(v1);
		vertices.push_back(v2);
		vertices.push_back(v3);
		vertices.push_back(v4);

		if (leftJustified)
			cursor_X += w;
	}

	if (VAO == 0) {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		std::cout << "Initialized Number Blitter.\n";
	}

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	////glEnableVertexAttribArray(1);
	//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	//glm::mat4 modelMatrix = glm::mat4(1.0f);
	//shader->setMat4("model", modelMatrix);
	//shader->setVec3("colorTint", color);

	glBindVertexArray(VAO);
	glEnable(GL_BLEND);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei)vertices.size());
	glBindVertexArray(0);
}

void NumberBlitter::TypeText(std::string text, bool centered)
{
	s_blitTimer = 0;
	s_waitTimer = 0;
	s_blitSpeed = 50;
	s_textToBlit = text;
	s_centerText = centered;
	s_timeToWait = 2;
}

void NumberBlitter::BlitText(std::string text, bool centered)
{
	currentCharIndex = (int)text.length();
	s_blitTimer = 0;
	s_waitTimer = 0;
	s_blitSpeed = -1;
	s_textToBlit = text;
	s_centerText = centered;
	s_timeToWait = -1;
}

void NumberBlitter::ResetBlitter()
{
	currentCharIndex = 0;
	s_blitTimer = 0;
	s_waitTimer = 0;
	s_blitSpeed = -1;
	s_textToBlit = "";
	s_centerText = false;
	s_timeToWait = -1;
}
