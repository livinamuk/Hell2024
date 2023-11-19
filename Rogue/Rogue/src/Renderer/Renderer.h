#pragma once
#include "Texture3D.h"
#include "../Common.h"

namespace Renderer {

	void Init();
	void RenderFrame();
	void RenderEditorFrame();
	void RenderUI();
	void HotloadShaders();
	void WipeShadowMaps();

	void QueueLineForDrawing(Line line);
	void QueuePointForDrawing(Point point);
	void QueueTriangleForLineRendering(Triangle& triangle);
	void QueueTriangleForSolidRendering(Triangle& triangle);
	void QueueUIForRendering(std::string textureName, int screenX, int screenY, bool centered);
	void QueueUIForRendering(UIRenderInfo renderInfo);

	void ToggleDrawingLights();
	void ToggleDrawingProbes();
	void ToggleDrawingLines();

	int GetRenderWidth();
	int GetRenderHeight();
	float GetPointCloudSpacing();
	void NextMode();
	void PreviousMode();
	void RecreateFrameBuffers();
	void CreatePointCloudBuffer();
	void CreateTriangleWorldVertexBuffer();

	inline int _method = 1;
	inline bool _shadowMapsAreDirty = true;
}