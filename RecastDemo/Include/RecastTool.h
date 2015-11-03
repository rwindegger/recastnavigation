//
// Copyright (c) 2015-2016 Rene Windegger rene@windegger.wtf
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#include "Recast.h"
#include "RecastDebugDraw.h"
#include "InputGeom.h"
#include "TestCase.h"
#include "Filelist.h"
#include "SlideShow.h"

#include "Sample_SoloMesh.h"
#include "Sample_TileMesh.h"
#include "Sample_TempObstacles.h"
#include "Sample_Debug.h"

#include "SDL.h"
#include "SDL_opengl.h"
#include <gl/glu.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"

#include <stdio.h>
#include <math.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <stdexcept>
#include <functional>

#pragma once

class RecastTool
{
private:
	std::unique_ptr<SDL_Window, std::function<void(SDL_Window*)>> m_Window = nullptr;
	SDL_GLContext m_GLContext;
	std::unique_ptr<SDL_Renderer, std::function<void(SDL_Renderer*)>> m_Renderer = nullptr;

	int m_Width;
	int m_Height;
	bool m_PresentationMode;

	float m_TotalTime = 0.0f;
	float m_TimeAcc = 0.0f;
	Uint32 m_LastTime;

	int mx = 0, my = 0;

	float moveW = 0, moveS = 0, moveA = 0, moveD = 0;

	float origrx = 0, origry = 0;
	int origx = 0, origy = 0;
	float scrollZoom = 0;
	bool rotate = false;
	bool movedDuringRotate = false;
	float rays[3], raye[3];

	bool showMenu;

	int propScroll = 0;
	int toolsScroll = 0;

	float markerPosition[3] = { 0,0,0 };
	bool markerPositionSet = false;

	SlideShow slideShow;

	float fogColor[4] = { 0.32f, 0.31f, 0.30f, 1.0f };

	float camx = 0, camy = 0, camz = 0, camr = 1000;
	float rx = 45;
	float ry = -45;

	bool done = false;

	int forward = -1;
	int reverse = -1;
	int left = -1;
	int right = -1;

	bool showProperties = true;
	bool showLog = false;
	bool showTools = true;
	bool showLevels = false;
	bool showSample = false;
	bool showTestCases = false;

	bool processHitTest = false;
	bool processHitTestShift = false;

	vector<string> files;

	std::unique_ptr<InputGeom> geom = nullptr;
	std::shared_ptr<Sample> sample = nullptr;
	std::unique_ptr<TestCase> test = nullptr;

	BuildContext ctx;

	char sampleName[64] = "Choose Sample...";
	char meshName[128] = "Choose Mesh...";

	GLdouble projectionMatrix[16];
	GLdouble modelviewMatrix[16];
	GLint viewport[4];
	GLdouble x, y, z;

	void InitSDL2();
	void InitWindow(int width, int height);
	void InitContext();
	void InitRenderer();
	void InitImGui();
	void HandleInput();
	void ProcessHitTest();
	void UpdateSimulation(float dt);
	void ClampFrameRate(float dt);
	void RenderSampleSelection();
	void RenderLevelSelection();
	void RenderPropertiesWindow();
	void RenderToolsWindow();
	void RenderLogWindow();
	void RenderTestCaseWindow();
	void RenderGui(float dt);
	void Update(float dt);
	void Render(float dt);
	float UpdateTimeStamp();
	bool HandleKeyDown(SDL_Event *e);
	bool HandleKeyUp(SDL_Event *e);
	bool HandleMouseButtonDown(SDL_Event *e);
	bool HandleMouseButtonUp(SDL_Event *e);
	bool HandleMouseMotion(SDL_Event *e);
public:
	RecastTool(int width, int height, bool presentationMode);
	~RecastTool();
	void Run();
	void DrawMarker(float markerPosition[3], GLdouble projectionMatrix[16], GLdouble modelviewMatrix[16], GLint viewport[4]);
	void ResetCameraAndFog(const std::unique_ptr<InputGeom>& geom, const std::shared_ptr<Sample>& sample, float& camx, float& camy, float& camz, float& camr, float& rx, float& ry);
};

