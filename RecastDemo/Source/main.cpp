//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
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

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <iomanip>
#include <cstdlib>

#ifdef WIN32
#	define snprintf _snprintf
#	define putenv _putenv
#endif

struct SampleItem
{
	Sample* (*create)();
	const char* name;
};

Sample* createSolo() { return new Sample_SoloMesh(); }
Sample* createTile() { return new Sample_TileMesh(); }
Sample* createTempObstacle() { return new Sample_TempObstacles(); }
Sample* createDebug() { return new Sample_Debug(); }

static SampleItem g_samples[] =
{
	{ createSolo, "Solo Mesh" },
	{ createTile, "Tile Mesh" },
	{ createTempObstacle, "Temp Obstacles" },
	//	{ createDebug, "Debug" },
};
static const int g_nsamples = sizeof(g_samples) / sizeof(SampleItem);
int width = 1440, height = 900;
// Function forward-declares
int run(int width, int height, bool presentationMode);
void drawMarker(float markerPosition[3], GLdouble projectionMatrix[16], GLdouble modelviewMatrix[16], GLint viewport[4]);
void resetCameraAndFog(const std::unique_ptr<InputGeom>& geom, const std::unique_ptr<Sample>& sample, float& camx, float& camy, float& camz, float& camr, float& rx, float& ry);

bool showProperties = true;
bool showLog = false;
bool showTools = true;
bool showLevels = false;
bool showSample = false;
bool showTestCases = false;

bool mouseOverMenu = false;

char sampleName[64] = "Choose Sample...";
char meshName[128] = "Choose Mesh...";

InputGeom* geom = 0;
Sample* sample = 0;
TestCase* test = 0;

BuildContext ctx;

FileList files;

int selectedSample = 0;

float camx = 0, camy = 0, camz = 0, camr = 1000;
float rx = 45;
float ry = -45;

void PropertiesWindow()
{
	if (showProperties)
	{
		ImGui::SetNextWindowPos(ImVec2(width - 250 - 10, 10));
		ImGui::Begin("Properties", &showProperties, ImVec2(250, height - 20), 0.7, ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);

		ImGui::Checkbox("Show Log", &showLog);
		ImGui::Checkbox("Show Tools", &showTools);

		ImGui::Separator();
		ImGui::Text("Sample");
		if (ImGui::Button(sampleName))
		{
			if (showSample)
			{
				showSample = false;
			}
			else
			{
				showSample = true;
				showLevels = false;
				showTestCases = false;
			}
		}

		ImGui::Separator();
		ImGui::Text("Input Mesh");
		if (ImGui::Button(meshName))
		{
			if (showLevels)
			{
				showLevels = false;
			}
			else
			{
				showSample = false;
				showTestCases = false;
				showLevels = true;
				scanDirectory("Meshes", ".obj", files);
			}
		}

		if (geom)
		{
			char text[64];
			snprintf(text, 64, "Verts: %.1fk  Tris: %.1fk",
				geom->getMesh()->getVertCount() / 1000.0f,
				geom->getMesh()->getTriCount() / 1000.0f);
			ImGui::Text(text);
		}

		ImGui::Separator();

		if (geom && sample)
		{
			ImGui::Separator();

			sample->handleSettings();

			if (ImGui::Button("Build"))
			{
				ctx.resetLog();
				if (!sample->handleBuild())
				{
					showLog = true;
				}
				ctx.dumpLog("Build log %s:", meshName);

				// Clear test.
				delete test;
				test = 0;
			}
			ImGui::Separator();
		}

		if (sample)
		{
			ImGui::Separator();
			sample->handleDebugMode();
		}
		ImGui::End();
	}
}

void LogWindow() {
	if (showLog)
	{
		ImGui::SetNextWindowPos(ImVec2(250 + 20, 10));
		ImGui::Begin("Log", &showLog, ImVec2(width - 300 - 250, 200), 0.5, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings);

		for (int i = 0; i < ctx.getLogCount(); ++i)
			ImGui::Text(ctx.getLogText(i));

		ImGui::End();
	}
}

void ToolsWindow()
{
	// Tools
	if (!showTestCases && showTools)
	{
		ImGui::SetNextWindowPos(ImVec2(10, 10));
		ImGui::Begin("Tools", &showTools, ImVec2(250, height - 20), 0.7, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings);

		if (sample)
			sample->handleTools();

		ImGui::End();
	}
}

void TestCaseWindow()
{
	// Test cases
	if (showTestCases)
	{
		ImGui::SetNextWindowPos(ImVec2(10, 10));
		ImGui::Begin("Tools", &showTools, ImVec2(250, height - 20), 0.7, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings);

		int testToLoad = -1;

		const char** items = const_cast<const char**>(files.files);
		ImGui::ListBox("Choose Test to Run", &testToLoad, items, files.size, 3);

		if (testToLoad != -1)
		{
			char path[256];
			strcpy(path, "Tests/");
			strcat(path, files.files[testToLoad]);
			test = new TestCase;
			if (test)
			{
				// Load the test.
				if (!test->load(path))
				{
					delete test;
					test = 0;
				}

				// Create sample
				Sample* newSample = 0;
				for (int i = 0; i < g_nsamples; ++i)
				{
					if (strcmp(g_samples[i].name, test->getSampleName()) == 0)
					{
						newSample = g_samples[i].create();
						if (newSample) strcpy(sampleName, g_samples[i].name);
					}
				}
				if (newSample)
				{
					delete sample;
					sample = newSample;
					sample->setContext(&ctx);
					showSample = false;
				}

				// Load geom.
				strcpy(meshName, test->getGeomFileName());
				meshName[sizeof(meshName) - 1] = '\0';

				delete geom;
				geom = 0;

				strcpy(path, "Meshes/");
				strcat(path, meshName);

				geom = new InputGeom;
				if (!geom || !geom->loadMesh(&ctx, path))
				{
					delete geom;
					geom = 0;
					showLog = true;
					ctx.dumpLog("Geom load log %s:", meshName);
				}
				if (sample && geom)
				{
					sample->handleMeshChanged(geom);
				}

				// This will ensure that tile & poly bits are updated in tiled sample.
				if (sample)
					sample->handleSettings();

				ctx.resetLog();
				if (sample && !sample->handleBuild())
				{
					ctx.dumpLog("Build log %s:", meshName);
				}

				if (geom || sample)
				{
					const float* bmin = 0;
					const float* bmax = 0;
					if (sample)
					{
						bmin = sample->getBoundsMin();
						bmax = sample->getBoundsMax();
					}
					else if (geom)
					{
						bmin = geom->getMeshBoundsMin();
						bmax = geom->getMeshBoundsMax();
					}
					// Reset camera and fog to match the mesh bounds.
					if (bmin && bmax)
					{
						camr = sqrtf(rcSqr(bmax[0] - bmin[0]) +
							rcSqr(bmax[1] - bmin[1]) +
							rcSqr(bmax[2] - bmin[2])) / 2;
						camx = (bmax[0] + bmin[0]) / 2 + camr;
						camy = (bmax[1] + bmin[1]) / 2 + camr;
						camz = (bmax[2] + bmin[2]) / 2 + camr;
						camr *= 3;
					}
					rx = 45;
					ry = -45;
					glFogf(GL_FOG_START, camr*0.2f);
					glFogf(GL_FOG_END, camr*1.25f);
				}

				// Do the tests.
				if (sample)
					test->doTests(sample->getNavMesh(), sample->getNavMeshQuery());
			}
		}

		ImGui::End();
	}

}

void SampleSelection()
{
	if (showSample)
	{
		ImGui::SetNextWindowPos(ImVec2(width - 10 - 250 - 10 - 200, height - 10 - 250));
		ImGui::Begin("Choose Sample", &showSample, ImVec2(200, 250), 0.7, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings);

		Sample* newSample = 0;
		if (ImGui::ListBox("Choose Sample", &selectedSample, &g_samples[0].name, g_nsamples, 3))
		{
			newSample = g_samples[selectedSample].create();
			if (newSample)
				strcpy(sampleName, g_samples[selectedSample].name);
		}

		if (newSample)
		{
			delete sample;
			sample = newSample;
			sample->setContext(&ctx);
			if (geom && sample)
			{
				sample->handleMeshChanged(geom);
			}
			showSample = false;
		}

		if (geom || sample)
		{
			const float* bmin = 0;
			const float* bmax = 0;
			if (sample)
			{
				bmin = sample->getBoundsMin();
				bmax = sample->getBoundsMax();
			}
			else if (geom)
			{
				bmin = geom->getMeshBoundsMin();
				bmax = geom->getMeshBoundsMax();
			}
			// Reset camera and fog to match the mesh bounds.
			if (bmin && bmax)
			{
				camr = sqrtf(rcSqr(bmax[0] - bmin[0]) +
					rcSqr(bmax[1] - bmin[1]) +
					rcSqr(bmax[2] - bmin[2])) / 2;
				camx = (bmax[0] + bmin[0]) / 2 + camr;
				camy = (bmax[1] + bmin[1]) / 2 + camr;
				camz = (bmax[2] + bmin[2]) / 2 + camr;
				camr *= 3;
			}
			rx = 45;
			ry = -45;
			glFogf(GL_FOG_START, camr*0.1f);
			glFogf(GL_FOG_END, camr*1.25f);
		}
		ImGui::End();
	}
}

void LevelSelection()
{
	if (showLevels)
	{
		ImGui::SetNextWindowPos(ImVec2(width - 10 - 250 - 10 - 200, height - 10 - 250));
		ImGui::Begin("Choose Level", &showLevels, ImVec2(200, 250), 0.7, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings);

		int levelToLoad = -1;
		const char** fileNames = const_cast<const char**>(files.files);
		ImGui::ListBox("Choose Level", &levelToLoad, fileNames, files.size, 3);

		if (levelToLoad != -1)
		{
			strncpy(meshName, files.files[levelToLoad], sizeof(meshName));
			meshName[sizeof(meshName) - 1] = '\0';
			showLevels = false;

			delete geom;
			geom = 0;

			char path[256];
			strcpy(path, "Meshes/");
			strcat(path, meshName);

			geom = new InputGeom;
			if (!geom || !geom->loadMesh(&ctx, path))
			{
				delete geom;
				geom = 0;

				showLog = true;
				ctx.dumpLog("Geom load log %s:", meshName);
			}
			if (sample && geom)
			{
				sample->handleMeshChanged(geom);
			}

			if (geom || sample)
			{
				const float* bmin = 0;
				const float* bmax = 0;
				if (sample)
				{
					bmin = sample->getBoundsMin();
					bmax = sample->getBoundsMax();
				}
				else if (geom)
				{
					bmin = geom->getMeshBoundsMin();
					bmax = geom->getMeshBoundsMax();
				}
				// Reset camera and fog to match the mesh bounds.
				if (bmin && bmax)
				{
					camr = sqrtf(rcSqr(bmax[0] - bmin[0]) +
						rcSqr(bmax[1] - bmin[1]) +
						rcSqr(bmax[2] - bmin[2])) / 2;
					camx = (bmax[0] + bmin[0]) / 2 + camr;
					camy = (bmax[1] + bmin[1]) / 2 + camr;
					camz = (bmax[2] + bmin[2]) / 2 + camr;
					camr *= 3;
				}
				rx = 45;
				ry = -45;
				glFogf(GL_FOG_START, camr*0.1f);
				glFogf(GL_FOG_END, camr*1.25f);
			}
		}
		ImGui::End();
	}
}

int main(int argc, char* argv[])
{
	// Init SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		std::cerr << "Could not initialise SDL\n";
		return -1;
	}

	// Center window
	char centerWindowEnv[] = "SDL_VIDEO_CENTERED=1";
	putenv(centerWindowEnv);

	// Init OpenGL
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
//#ifndef WIN32
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	//#endif

	/*const SDL_RendererInfo* vi = SDL_GetRendererInfo();*/

	bool presentationMode = false;

	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);


	SDL_Window *window;
	window = SDL_CreateWindow("RecastDemo", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);


	if (!window)
	{
		std::cerr << "Could not create window: %s\n", SDL_GetError();
		SDL_Quit();
		return 1;
	}
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);

	if (!glcontext)
	{
		std::cerr << "Could not create context: %s\n", SDL_GetError();
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 2;
	}

	SDL_Renderer *renderer;
	renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	if (!renderer)
	{
		std::cerr << "Could not create Renderer: %s\n", SDL_GetError();
		SDL_GL_DeleteContext(glcontext);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 3;
	}

	glEnable(GL_MULTISAMPLE);

	if (!ImGui_ImplSdl_Init(window))
	{
		std::cerr << "Could not init GUI renderer.\n";
		SDL_DestroyRenderer(renderer);
		SDL_GL_DeleteContext(glcontext);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 4;
	}

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();
	io.Fonts->AddFontFromFileTTF("DroidSans.ttf", 15.0);
}

int run( int width, int height, bool presentationMode) {
	float totalTime = 0.0f;
	float timeAcc = 0.0f;
	Uint32 lastTime = SDL_GetTicks();
	int mx = 0, my = 0;

	float moveW = 0, moveS = 0, moveA = 0, moveD = 0;

	float origrx = 0, origry = 0;
	int origx = 0, origy = 0;
	float scrollZoom = 0;
	bool rotate = false;
	bool movedDuringRotate = false;
	float rays[3], raye[3];

	bool showMenu = !presentationMode;

	int propScroll = 0;
	int toolsScroll = 0;

	float markerPosition[3] = { 0,0,0 };
	bool markerPositionSet = false;

	SlideShow slideShow("slides/");


	glEnable(GL_CULL_FACE);

	float fogColor[4] = { 0.32f, 0.31f, 0.30f, 1.0f };
	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, camr * 0.1f);
	glFogf(GL_FOG_END, camr * 1.25f);
	glFogfv(GL_FOG_COLOR, fogColor);

	glDepthFunc(GL_LEQUAL);

	bool done = false;
	while (!done)
	{
		// Handle input events.
		int mouseScroll = 0;
		bool processHitTest = false;
		bool processHitTestShift = false;
		SDL_Event event;

		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSdl_ProcessEvent(&event);
			switch (event.type)
			{
				case SDL_KEYDOWN:
					// Handle any key presses here.
					if (event.key.keysym.sym == SDLK_ESCAPE)
					{
						done = true;
					}
					else if (event.key.keysym.sym == SDLK_t)
					{
						showLevels = false;
						showSample = false;
						showTestCases = true;
						scanDirectory("Tests", ".txt", files);
					}
					else if (event.key.keysym.sym == SDLK_TAB)
					{
						showMenu = !showMenu;
					}
					else if (event.key.keysym.sym == SDLK_SPACE)
					{
						if (sample)
							sample->handleToggle();
					}
					else if (event.key.keysym.sym == SDLK_1)
					{
						if (sample)
							sample->handleStep();
					}
					else if (event.key.keysym.sym == SDLK_9)
					{
						if (geom)
							geom->save("geomset.txt");
					}
					else if (event.key.keysym.sym == SDLK_0)
					{
					geom = std::unique_ptr<InputGeom>(new InputGeom);
					if (!geom->load(&ctx, "geomset.txt"))
					{
						geom = nullptr;
							
							showLog = true;
							ctx.dumpLog("Geom load log %s:", meshName.c_str());
						}
						if (sample && geom)
						{
							sample->handleMeshChanged(geom.get());
						}
						if (geom || sample)
						{
							const float* bmin = 0;
							const float* bmax = 0;
							if (sample)
							{
								bmin = sample->getBoundsMin();
								bmax = sample->getBoundsMax();
							}
							else if (geom)
							{
								bmin = geom->getMeshBoundsMin();
								bmax = geom->getMeshBoundsMax();
							}
							// Reset camera and fog to match the mesh bounds.
							if (bmin && bmax)
							{
								camr = sqrtf(rcSqr(bmax[0] - bmin[0]) +
											 rcSqr(bmax[1] - bmin[1]) +
											 rcSqr(bmax[2] - bmin[2])) / 2;
								camx = (bmax[0] + bmin[0]) / 2 + camr;
								camy = (bmax[1] + bmin[1]) / 2 + camr;
								camz = (bmax[2] + bmin[2]) / 2 + camr;
								camr *= 3;
							}
							rx = 45;
							ry = -45;
							glFogf(GL_FOG_START, camr*0.2f);
							glFogf(GL_FOG_END, camr*1.25f);
						}
					}
					else if (event.key.keysym.sym == SDLK_RIGHT)
					{
						slideShow.nextSlide();
					}
					else if (event.key.keysym.sym == SDLK_LEFT)
					{
						slideShow.prevSlide();
					}
					break;
					
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_RIGHT)
					{
						if (!mouseOverMenu)
						{
							// Rotate view
							rotate = true;
							movedDuringRotate = false;
							origx = mx;
							origy = my;
							origrx = rx;
							origry = ry;
						}
					}
					break;
					
				case SDL_MOUSEBUTTONUP:
					// Handle mouse clicks here.
					if (event.button.button == SDL_BUTTON_RIGHT)
					{
						rotate = false;
						if (!mouseOverMenu && !movedDuringRotate)
						{
								processHitTest = true;
								processHitTestShift = true;
						}
					}
					else if (event.button.button == SDL_BUTTON_LEFT)
					{
						if (!mouseOverMenu)
						{
							processHitTest = true;
							processHitTestShift = (SDL_GetModState() & KMOD_SHIFT) ? true : false;
						}
					}
					
					break;
					
				case SDL_MOUSEMOTION:
					mx = event.motion.x;
					my = height - 1 - event.motion.y;
					if (rotate)
					{
						int dx = mx - origx;
						int dy = my - origy;
						rx = origrx - dy * 0.25f;
						ry = origry + dx * 0.25f;
						if (dx * dx + dy * dy > 3 * 3)
							movedDuringRotate = true;
					}
					break;
					
				case SDL_QUIT:
					done = true;
					break;
					
				default:
					break;
			}
		}

		Uint32	time = SDL_GetTicks();
		float	dt = (time - lastTime) / 1000.0f;
		lastTime = time;

		totalTime += dt;

		// Hit test mesh.
		if (processHitTest && geom && sample)
		{
			float hitTime;
			if (geom->raycastMesh(rays, raye, hitTime))
			{
				if (SDL_GetModState() & KMOD_CTRL)
				{
					// Marker
					markerPositionSet = true;
					markerPosition[0] = rays[0] + (raye[0] - rays[0]) * hitTime;
					markerPosition[1] = rays[1] + (raye[1] - rays[1]) * hitTime;
					markerPosition[2] = rays[2] + (raye[2] - rays[2]) * hitTime;
				}
				else
				{
					float pos[3] = {
						rays[0] + (raye[0] - rays[0]) * hitTime,
						rays[1] + (raye[1] - rays[1]) * hitTime,
						rays[2] + (raye[2] - rays[2]) * hitTime
					};
					sample->handleClick(rays, pos, processHitTestShift);
				}
			}
			else if (SDL_GetModState() & KMOD_CTRL)
			{
				// Marker
				markerPositionSet = false;
			}
		}

		// Update sample simulation.
		const float SIM_RATE = 20;
		const float DELTA_TIME = 1.0f / SIM_RATE;
		timeAcc = rcClamp(timeAcc + dt, -1.0f, 1.0f);
		int simIter = 0;
		while (timeAcc > DELTA_TIME)
		{
			timeAcc -= DELTA_TIME;
			if (simIter < 5 && sample)
				sample->handleUpdate(DELTA_TIME);
			simIter++;
		}

		// Clamp the framerate so that we do not hog all the CPU.
		const float MIN_FRAME_TIME = 1.0f / 40.0f;
		if (dt < MIN_FRAME_TIME)
		{
			int ms = (int)((MIN_FRAME_TIME - dt) * 1000.0f);
			if (ms > 10) ms = 10;
			if (ms >= 0) SDL_Delay(ms);
		}

		// Update and render
		glViewport(0, 0, width, height);
		glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_TEXTURE_2D);

		// Render 3d
		glEnable(GL_DEPTH_TEST);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(50.0f, (float)width / (float)height, 1.0f, camr);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(rx, 1, 0, 0);
		glRotatef(ry, 0, 1, 0);
		glTranslatef(-camx, -camy, -camz);

		// Get hit ray position and direction.
		GLdouble projectionMatrix[16];
		GLdouble modelviewMatrix[16];
		GLint viewport[4];
		glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);
		glGetDoublev(GL_MODELVIEW_MATRIX, modelviewMatrix);
		glGetIntegerv(GL_VIEWPORT, viewport);
		GLdouble x, y, z;
		gluUnProject(mx, my, 0.0f, modelviewMatrix, projectionMatrix, viewport, &x, &y, &z);
		rays[0] = (float)x; rays[1] = (float)y; rays[2] = (float)z;
		gluUnProject(mx, my, 1.0f, modelviewMatrix, projectionMatrix, viewport, &x, &y, &z);
		raye[0] = (float)x; raye[1] = (float)y; raye[2] = (float)z;

		// Handle keyboard movement.
		const Uint8* keystate = SDL_GetKeyboardState(NULL);
		moveW = rcClamp(moveW + dt * 4 * (keystate[SDL_SCANCODE_W] ? 1 : -1), 0.0f, 1.0f);
		moveS = rcClamp(moveS + dt * 4 * (keystate[SDL_SCANCODE_S] ? 1 : -1), 0.0f, 1.0f);
		moveA = rcClamp(moveA + dt * 4 * (keystate[SDL_SCANCODE_A] ? 1 : -1), 0.0f, 1.0f);
		moveD = rcClamp(moveD + dt * 4 * (keystate[SDL_SCANCODE_D] ? 1 : -1), 0.0f, 1.0f);

		float keybSpeed = 22.0f;
		if (SDL_GetModState() & KMOD_SHIFT)
			keybSpeed *= 4.0f;

		float movex = (moveD - moveA) * keybSpeed * dt;
		float movey = (moveS - moveW) * keybSpeed * dt + scrollZoom * 2.0f;
		scrollZoom = 0;

		camx += movex * (float)modelviewMatrix[0];
		camy += movex * (float)modelviewMatrix[4];
		camz += movex * (float)modelviewMatrix[8];

		camx += movey * (float)modelviewMatrix[2];
		camy += movey * (float)modelviewMatrix[6];
		camz += movey * (float)modelviewMatrix[10];

		glEnable(GL_FOG);

		if (sample)
			sample->handleRender();
		if (test)
			test->handleRender();

		glDisable(GL_FOG);

		// Render GUI
		glDisable(GL_DEPTH_TEST);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0, width, 0, height);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		mouseOverMenu = false;

		ImGui_ImplSdl_NewFrame(window);
		ImGui::BeginTooltip();

		if (sample)
		{
			sample->handleRenderOverlay((double*)projectionMatrix, (double*)modelviewMatrix, (int*)viewport);
		}
		if (test && test->handleRenderOverlay((double*)projectionMatrix, (double*)modelviewMatrix, (int*)viewport))
		{
				mouseOverMenu = true;
		}

		// Help text.
		if (showMenu)
		{
			const char msg[] = "W/S/A/D: Move  RMB: Rotate";
			ImGui::Text(msg);
			//imguiDrawText(280, height - 20, IMGUI_ALIGN_LEFT, msg, imguiRGBA(255, 255, 255, 128));
		}

		PropertiesWindow();
		ToolsWindow();
		TestCaseWindow();
		SampleSelection();
		LevelSelection();

		
		LogWindow();

		slideShow.updateAndDraw(dt, (float)width, (float)height);

		// Marker
		if (mposSet && gluProject((GLdouble)mpos[0], (GLdouble)mpos[1], (GLdouble)mpos[2],
			model, proj, view, &x, &y, &z))
		{
			// Draw marker circle
			glLineWidth(5.0f);
			glColor4ub(240, 220, 0, 196);
			glBegin(GL_LINE_LOOP);
			const float r = 25.0f;
			for (int i = 0; i < 20; ++i)
			{
				const float a = (float)i / 20.0f * RC_PI * 2;
				const float fx = (float)x + cosf(a)*r;
				const float fy = (float)y + sinf(a)*r;
				glVertex2f(fx, fy);
			}
			glEnd();
			glLineWidth(1.0f);
		}

		glEnable(GL_DEPTH_TEST);
		ImGui::EndTooltip();
		ImGui::Render();
		SDL_GL_SwapWindow(window);
	}

	ImGui::Shutdown();

	SDL_DestroyRenderer(renderer);
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	delete sample;
	delete geom;

	return 0;
}

void resetCameraAndFog(const std::unique_ptr<InputGeom>& geom, const std::unique_ptr<Sample>& sample,
					   float& camx, float& camy, float& camz, float& camr, float& rx, float& ry) {
	if (geom || sample)
	{
		const float* bmin = 0;
		const float* bmax = 0;
		if (sample)
		{
			bmin = sample->getBoundsMin();
			bmax = sample->getBoundsMax();
		}
		else if (geom)
		{
			bmin = geom->getMeshBoundsMin();
			bmax = geom->getMeshBoundsMax();
		}
		// Reset camera and fog to match the mesh bounds.
		if (bmin && bmax)
		{
			camr = sqrtf(rcSqr(bmax[0] - bmin[0]) +
						 rcSqr(bmax[1] - bmin[1]) +
						 rcSqr(bmax[2] - bmin[2])) / 2;
			camx = (bmax[0] + bmin[0]) / 2 + camr;
			camy = (bmax[1] + bmin[1]) / 2 + camr;
			camz = (bmax[2] + bmin[2]) / 2 + camr;
			camr *= 3;
		}
		rx = 45;
		ry = -45;
		glFogf(GL_FOG_START, camr*0.1f);
		glFogf(GL_FOG_END, camr*1.25f);
	}
}

void drawMarker(float markerPosition[3], GLdouble projectionMatrix[16], GLdouble modelviewMatrix[16], GLint viewport[4])
{
	GLdouble windowCoords[3];
	if (gluProject((GLdouble)markerPosition[0], (GLdouble)markerPosition[1], (GLdouble)markerPosition[2],
		modelviewMatrix, projectionMatrix, viewport, &windowCoords[0], &windowCoords[1], &windowCoords[2]))
	{
		// Draw marker circle
		glLineWidth(5.0f);
		glColor4ub(240, 220, 0, 196);
		glBegin(GL_LINE_LOOP);
		const float radius = 25.0f;
		for (int i = 0; i < 20; ++i)
		{
			const float angle = (float)i / 20.0f * RC_PI * 2;
			const float fx = (float)windowCoords[0] + cosf(angle) * radius;
			const float fy = (float)windowCoords[1] + sinf(angle) * radius;
			glVertex2f(fx, fy);
		}
		glEnd();
		glLineWidth(1.0f);
	}
}
