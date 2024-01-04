#include "NumberBlitter.h"
#include <algorithm>
#include <iostream>
#include "../Core/GL.h"
#include "../Core/AssetManager.h"


unsigned int VAO = 0;
unsigned int VBO = 0;
std::string _NumSheet = "1234567890/";

void NumberBlitter::Draw(const char* text, int xScreenCoord, int yScreenCoord, int renderWidth, int renderHeight, float scale, Justification justification) {

/*	float renderTargetWidth = (float)viewportWidth;
	float renderTargetHeight = (float)viewPortHeight;
	float width = (1.0f / renderTargetWidth) * quadWidth * scale;
	float height = (1.0f / renderTargetHeight) * quadHeight * scale;
	float ndcX = ((xPosition + (quadWidth / 2.0f)) / renderTargetWidth) * 2 - 1;
	float ndcY = ((yPosition + (quadHeight / 2.0f)) / renderTargetHeight) * 2 - 1;
*/


	//float screenWidth = 1920.0f * 1.0f;
	//float screenHeight = 1080.0f * 1.0f;
	float screenWidth = renderWidth * 1.0f;
	float screenHeight = renderHeight * 1.0f;
	float textureWidth = 161;
	//static float textScale = 1;
	float cursor_X = 0;
	float cursor_Y = -1;

	cursor_X = (xScreenCoord / screenWidth) * 2 - 1;
	cursor_Y = (yScreenCoord / screenHeight) * 2 - 1;
	cursor_Y *= -1;

	std::vector<Vertex> vertices;
	char character;
	float charWidth, charBegin;


	for (int i = 0; i < strlen(text); i++)
	{
		if (justification == Justification::LEFT) {
			character = text[i];
		}
		else {
			character = text[strlen(text) - 1 - i];
		}

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

		float w = charWidth / (screenWidth / 2) * (screenWidth / renderWidth);
		float h = charHeight / (screenHeight / 2) * (screenHeight / renderHeight);

		if (justification != Justification::LEFT) {
			cursor_X -= w;
		}

		Vertex v1 = { glm::vec3(cursor_X, cursor_Y, 0), NRM_Y_UP, glm::vec2(tex_coord_L, 0) };
		Vertex v2 = { glm::vec3(cursor_X, cursor_Y - h, 0), NRM_Y_UP, glm::vec2(tex_coord_L, 1) };
		Vertex v3 = { glm::vec3(cursor_X + w, cursor_Y, 0), NRM_Y_UP, glm::vec2(tex_coord_R, 0) };
		Vertex v4 = { glm::vec3(cursor_X + w, cursor_Y - h, 0), NRM_Y_UP, glm::vec2(tex_coord_R, 1) };

		vertices.push_back(v1);
		vertices.push_back(v2);
		vertices.push_back(v3);
		vertices.push_back(v4);

		if (justification == Justification::LEFT) {
			cursor_X += w;
		}
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

	glBindVertexArray(VAO);
	glEnable(GL_BLEND);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei)vertices.size());
	glBindVertexArray(0);
}
