#pragma once
#include "../Common.h"
#include "../API/OpenGL/Types/GL_shader.h"
#include "../Core/Player.h"
#include "../BackEnd/BackEnd.h"

namespace Renderer_OLD {



	void InitMinimumGL(); // remove once you can
	void Init();

	void RenderLoadingScreen();
	void RenderFrame(Player* player);
	void RenderFloorplanFrame();
	void RenderDebugMenu();
	void RenderUI(float viewportWidth, float viewportHeight);
	void HotloadShaders();
	void WipeShadowMaps();
	void RenderEditorMode();
	void QueueLineForDrawing(Line line);
	void QueuePointForDrawing(Point point);
	void QueueTriangleForLineRendering(Triangle& triangle);
	void QueueTriangleForSolidRendering(Triangle& triangle);
	void QueueUIForRendering(std::string textureName, int screenX, int screenY, bool centered, glm::vec3 color);
	void QueueUIForRendering(UIRenderInfo renderInfo);
	void ToggleDrawingLights();
	void ToggleDrawingProbes();
	void ToggleDrawingLines();
	void ToggleDrawingRagdolls();
	void ToggleDebugText();
    void DrawInstancedBloodDecals(Shader* shader, Player* player);
	int GetRenderWidth();
	int GetRenderHeight();
	float GetPointCloudSpacing();
	void NextMode();
	void PreviousMode();
	void NextDebugLineRenderMode();
	void RecreateFrameBuffers(int currentPlayer);
	void CreatePointCloudBuffer();
	void CreateTriangleWorldVertexBuffer();
    void EnteredEditorMode();
}