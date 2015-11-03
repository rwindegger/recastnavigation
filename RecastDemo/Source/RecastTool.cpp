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
#include "RecastTool.h"


struct SampleItem
{
	std::shared_ptr<Sample>(*create)();
	const char* name;
};

std::shared_ptr<Sample> createSolo() { return std::shared_ptr<Sample>(new Sample_SoloMesh); }
std::shared_ptr<Sample> createTile() { return std::shared_ptr<Sample>(new Sample_TileMesh); }
std::shared_ptr<Sample> createTempObstacle() { return std::shared_ptr<Sample>(new Sample_TempObstacles); }
std::shared_ptr<Sample> createDebug() { return std::shared_ptr<Sample>(new Sample_Debug); }

static SampleItem g_samples[] =
{
	{ createSolo, "Solo Mesh" },
	{ createTile, "Tile Mesh" },
	{ createTempObstacle, "Temp Obstacles" },
	/*{ createDebug, "Debug" },*/
};

static const int g_nsamples = sizeof(g_samples) / sizeof(SampleItem);

RecastTool::RecastTool(int width, int height, bool presentationMode) :slideShow("slides/")
{
	m_Width = width;
	m_Height = height;
	m_PresentationMode = presentationMode;
	showMenu = !m_PresentationMode;

	this->InitSDL2();
	this->InitWindow(width, height);
	this->InitContext();
	this->InitRenderer();
	this->InitImGui();
}

RecastTool::~RecastTool()
{
	geom.release();
	ImGui::Shutdown();
	m_Renderer.release();
	SDL_GL_DeleteContext(m_GLContext);
	m_Window.release();
	SDL_Quit();
}

void RecastTool::InitSDL2()
{
	// Init SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		throw std::runtime_error("Could not initialise SDL");
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

	glEnable(GL_MULTISAMPLE);
}

void RecastTool::InitWindow(int width, int height)
{
	m_Window = std::unique_ptr<SDL_Window, std::function<void(SDL_Window*)>>(
		SDL_CreateWindow("RecastDemo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL),
		[](SDL_Window* f) { SDL_DestroyWindow(f); }
	);

	if (!m_Window)
	{
		std::stringstream s = std::stringstream();
		s << "Could not create window: %s\n", SDL_GetError();
		SDL_Quit();
		throw std::runtime_error(s.str());
	}
}

void RecastTool::InitContext()
{
	m_GLContext = SDL_GL_CreateContext(m_Window.get());

	if (!m_GLContext)
	{
		std::stringstream s = std::stringstream();
		s << "Could not create context: %s\n", SDL_GetError();
		m_Window.release();
		SDL_Quit();
		throw std::runtime_error(s.str());
	}
}

void RecastTool::InitRenderer()
{

	m_Renderer = std::unique_ptr<SDL_Renderer, std::function<void(SDL_Renderer*)>>(
		SDL_CreateRenderer(m_Window.get(), 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
		[](SDL_Renderer* f) { SDL_DestroyRenderer(f); }
	);

	if (!m_Renderer)
	{
		std::stringstream s = std::stringstream();
		s << "Could not create Renderer: %s\n", SDL_GetError();
		SDL_GL_DeleteContext(m_GLContext);
		m_Window.release();
		SDL_Quit();
		throw std::runtime_error(s.str());
	}
}

void RecastTool::InitImGui()
{
	if (!ImGui_ImplSdl_Init(m_Window.get()))
	{
		std::stringstream s = std::stringstream();
		s << "Could not init GUI renderer.\n";
		m_Renderer.release();
		SDL_GL_DeleteContext(m_GLContext);
		m_Window.release();
		SDL_Quit();
		throw std::runtime_error(s.str());
	}
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();
	io.Fonts->AddFontFromFileTTF("DroidSans.ttf", 15.0);
}

bool RecastTool::HandleKeyDown(SDL_Event *e)
{
	// Handle any key presses here.
	if (e->key.keysym.sym == SDLK_ESCAPE)
	{
		done = true;
	}
	else if (e->key.keysym.sym == SDLK_t)
	{
		showLevels = false;
		showSample = false;
		showTestCases = true;
		scanDirectory("Tests", ".txt", files);
	}
	else if (e->key.keysym.sym == SDLK_TAB)
	{
		showMenu = !showMenu;
	}
	else if (e->key.keysym.sym == SDLK_SPACE)
	{
		if (sample)
			sample->handleToggle();
	}
	else if (e->key.keysym.sym == SDLK_1)
	{
		if (sample)
			sample->handleStep();
	}
	else if (e->key.keysym.sym == SDLK_9)
	{
		if (geom)
			geom->save("geomset.txt");
	}
	else if (e->key.keysym.sym == SDLK_0)
	{
		geom = std::unique_ptr<InputGeom>(new InputGeom);
		if (!geom->load(&ctx, "geomset.txt"))
		{
			geom = nullptr;

			showLog = true;
			ctx.dumpLog("Geom load log %s:", meshName);
		}
		if (sample && geom)
		{
			sample->handleMeshChanged(geom.get());
		}
		if (geom || sample)
		{
			this->ResetCameraAndFog(geom, sample, camx, camy, camz, camr, rx, ry);
		}
	}
	else if (e->key.keysym.sym == SDLK_RIGHT)
	{
		slideShow.nextSlide();
	}
	else if (e->key.keysym.sym == SDLK_LEFT)
	{
		slideShow.prevSlide();
	}

	// Handle keyboard movement.
	if (e->key.keysym.sym == SDLK_w)
	{
		forward = 1;
	}
	if (e->key.keysym.sym == SDLK_s)
	{
		reverse = 1;
	}
	if (e->key.keysym.sym == SDLK_a)
	{
		left = 1;
	}
	if (e->key.keysym.sym == SDLK_d)
	{
		right = 1;
	}
	return true;
}

bool RecastTool::HandleKeyUp(SDL_Event *e)
{
	if (e->key.keysym.sym == SDLK_w)
	{
		forward = -1;
	}
	if (e->key.keysym.sym == SDLK_s)
	{
		reverse = -1;
	}
	if (e->key.keysym.sym == SDLK_a)
	{
		left = -1;
	}
	if (e->key.keysym.sym == SDLK_d)
	{
		right = -1;
	}
	return true;
}

bool RecastTool::HandleMouseButtonDown(SDL_Event *e)
{
	if (e->button.button == SDL_BUTTON_RIGHT)
	{
		if (!ImGui::IsMouseHoveringAnyWindow())
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
	return true;
}

bool RecastTool::HandleMouseButtonUp(SDL_Event *e)
{
	// Handle mouse clicks here.
	if (e->button.button == SDL_BUTTON_RIGHT)
	{
		rotate = false;
		if (!ImGui::IsMouseHoveringAnyWindow() && !movedDuringRotate)
		{
			processHitTest = true;
			processHitTestShift = true;
		}
	}
	else if (e->button.button == SDL_BUTTON_LEFT)
	{
		if (!ImGui::IsMouseHoveringAnyWindow())
		{
			processHitTest = true;
			processHitTestShift = (SDL_GetModState() & KMOD_SHIFT) ? true : false;
		}
	}
	return true;
}

bool RecastTool::HandleMouseMotion(SDL_Event *e)
{
	mx = e->motion.x;
	my = m_Height - 1 - e->motion.y;
	if (rotate)
	{
		int dx = mx - origx;
		int dy = my - origy;
		rx = origrx - dy * 0.25f;
		ry = origry + dx * 0.25f;
		if (dx * dx + dy * dy > 3 * 3)
			movedDuringRotate = true;
	}
	return true;
}

void RecastTool::HandleInput()
{
	// Handle input events.
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSdl_ProcessEvent(&event);
		switch (event.type)
		{
		case SDL_KEYDOWN:
			this->HandleKeyDown(&event);
			break;
		case SDL_KEYUP:
			this->HandleKeyUp(&event);
			break;
		case SDL_MOUSEBUTTONDOWN:
			this->HandleMouseButtonDown(&event);
			break;
		case SDL_MOUSEBUTTONUP:
			this->HandleMouseButtonUp(&event);
			break;
		case SDL_MOUSEMOTION:
			this->HandleMouseMotion(&event);
			break;
		case SDL_QUIT:
			done = true;
			break;
		default:
			break;
		}
	}
}

float RecastTool::UpdateTimeStamp()
{
	Uint32	time = SDL_GetTicks();
	float	dt = (time - m_LastTime) / 1000.0f;
	m_LastTime = time;
	m_TotalTime += dt;
	return dt;
}

void RecastTool::ProcessHitTest()
{
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
}

void RecastTool::UpdateSimulation(float dt)
{
	// Update sample simulation.
	const float SIM_RATE = 20;
	const float DELTA_TIME = 1.0f / SIM_RATE;
	m_TimeAcc = rcClamp(m_TimeAcc + dt, -1.0f, 1.0f);
	int simIter = 0;
	while (m_TimeAcc > DELTA_TIME)
	{
		m_TimeAcc -= DELTA_TIME;
		if (simIter < 5 && sample)
			sample->handleUpdate(DELTA_TIME);
		simIter++;
	}
}

void RecastTool::ClampFrameRate(float dt)
{
	// Clamp the framerate so that we do not hog all the CPU.
	const float MIN_FRAME_TIME = 1.0f / 40.0f;
	if (dt < MIN_FRAME_TIME)
	{
		int ms = (int)((MIN_FRAME_TIME - dt) * 1000.0f);
		if (ms > 10) ms = 10;
		if (ms >= 0) SDL_Delay(ms);
	}
}

void RecastTool::RenderSampleSelection()
{
	std::shared_ptr<Sample> newSample = nullptr;

	if (ImGui::BeginPopup("sampleSelection"))
	{
		ImGui::Text("Select Sample");
		ImGui::Separator();

		for (size_t i = 0; i < g_nsamples; ++i)
		{
			if (ImGui::Selectable(g_samples[i].name))
			{
				newSample = std::shared_ptr<Sample>(g_samples[i].create());
				if (newSample)
				{
					strcpy(sampleName, g_samples[i].name);
				}
			}
		}

		ImGui::EndPopup();

		if (newSample)
		{
			sample = newSample;
			sample->setContext(&ctx);
			if (geom && sample)
			{
				sample->handleMeshChanged(geom.get());
			}
			showSample = false;
		}

		if (geom || sample)
		{
			this->ResetCameraAndFog(geom, sample, camx, camy, camz, camr, rx, ry);
		}
	}
}

void RecastTool::RenderLevelSelection()
{
	int levelToLoad = -1;

	if (ImGui::BeginPopup("levelSelection"))
	{
		ImGui::Text("Select Level");
		ImGui::Separator();
		scanDirectory("Meshes", ".obj", files);
		for (size_t i = 0; i < files.size(); ++i)
		{
			if (ImGui::Selectable(files[i].c_str()))
			{
				levelToLoad = i;
				strncpy(meshName, files[i].c_str(), sizeof(meshName));
			}
		}

		ImGui::EndPopup();
	}
	if (levelToLoad != -1)
	{
		meshName[sizeof(meshName) - 1] = '\0';
		showLevels = false;

		geom.release();
		geom = 0;

		char path[256];
		strcpy(path, "Meshes/");
		strcat(path, meshName);

		geom = std::unique_ptr<InputGeom>(new InputGeom);
		if (!geom || !geom->loadMesh(&ctx, path))
		{
			geom.release();
			geom = 0;

			showLog = true;
			ctx.dumpLog("Geom load log %s:", meshName);
		}
		if (sample && geom)
		{
			sample->handleMeshChanged(geom.get());
		}

		if (geom || sample)
		{
			this->ResetCameraAndFog(geom, sample, camx, camy, camz, camr, rx, ry);
		}

	}
}

void RecastTool::RenderPropertiesWindow()
{
	if (showProperties)
	{
		ImGui::SetNextWindowPos(ImVec2(m_Width - 260, 10));
		ImGui::Begin("Properties", &showProperties, ImVec2(250, m_Height - 20), 0.7, ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
		ImGui::PushItemWidth(-100);
		ImGui::Checkbox("Show Log", &showLog);
		ImGui::Checkbox("Show Tools", &showTools);

		ImGui::Separator();

		if (ImGui::Button("Sample"))
		{
			ImGui::OpenPopup("sampleSelection");
		}
		ImGui::SameLine();
		ImGui::Text(sampleName);
		this->RenderSampleSelection();

		ImGui::Separator();
		if (ImGui::Button("Mesh"))
		{
			ImGui::OpenPopup("levelSelection");
		}
		ImGui::SameLine();
		ImGui::Text(meshName);
		this->RenderLevelSelection();

		if (geom)
		{
			char text[64];
			snprintf(text, 64, "Verts: %.1fk  Tris: %.1fk",
				geom->getMesh()->getVertCount() / 1000.0f,
				geom->getMesh()->getTriCount() / 1000.0f);
			ImGui::Text(text);
		}
		if (ImGui::CollapsingHeader("Settings"))
		{
			if (geom && sample)
			{
				if (ImGui::TreeNode("Sample Settings"))
				{
					sample->handleSettings();

					if (ImGui::Button("Build"))
					{
						ctx.resetLog();
						if (!sample->handleBuild())
						{
							showLog = true;
						}
						ctx.dumpLog("Build log %s:", meshName);

						test.release();
						test = nullptr;
					}

					ImGui::TreePop();
				}
			}

			if (sample)
			{
				if (ImGui::TreeNode("Debug Mode"))
				{
					sample->handleDebugMode();
					ImGui::TreePop();
				}

			}
		}
		ImGui::End();
	}
}

void RecastTool::RenderToolsWindow()
{
	// Tools
	if (!showTestCases && showTools)
	{
		ImGui::SetNextWindowPos(ImVec2(10, 10));
		ImGui::Begin("Tools", &showTools, ImVec2(250, m_Height - 20), 0.7, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings);
		ImGui::PushItemWidth(140);
		if (sample)
			sample->handleTools();

		ImGui::End();
	}
}

void RecastTool::RenderLogWindow()
{
	if (showLog)
	{
		ImGui::SetNextWindowPos(ImVec2(270, 10));
		ImGui::Begin("Log", &showLog, ImVec2(m_Width - 300 - 250, 200), 0.5, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings);

		for (int i = 0; i < ctx.getLogCount(); ++i)
			ImGui::Text(ctx.getLogText(i));

		ImGui::End();
	}
}

void RecastTool::RenderTestCaseWindow()
{
	// Test cases
	if (showTestCases)
	{
		ImGui::SetNextWindowPos(ImVec2(10, 10));
		ImGui::Begin("Tools", &showTools, ImVec2(250, m_Height - 20), 0.7, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings);

		int testToLoad = -1;


		std::vector<const char*> cstrings;
		for (size_t i = 0; i < files.size(); ++i)
			cstrings.push_back(files[i].c_str());

		const char** items = &cstrings[0];

		ImGui::ListBox("Choose Test to Run", &testToLoad, items, files.size(), 3);

		if (testToLoad != -1)
		{
			char path[256];
			strcpy(path, "Tests/");
			strcat(path, cstrings[testToLoad]);
			test = std::unique_ptr<TestCase>(new TestCase);
			if (test)
			{
				// Load the test.
				if (!test->load(path))
				{
					test.release();
					test = nullptr;
				}

				// Create sample
				std::shared_ptr<Sample> newSample = nullptr;
				for (int i = 0; i < g_nsamples; ++i)
				{
					if (strcmp(g_samples[i].name, test->getSampleName().c_str()) == 0)
					{
						newSample = std::shared_ptr<Sample>(g_samples[i].create());
						if (newSample) strcpy(sampleName, g_samples[i].name);
					}
				}
				if (newSample)
				{
					sample = newSample;
					sample->setContext(&ctx);
					showSample = false;
				}

				// Load geom.
				strcpy(meshName, test->getGeomFileName().c_str());
				meshName[sizeof(meshName) - 1] = '\0';

				geom.release();
				geom = 0;

				strcpy(path, "Meshes/");
				strcat(path, meshName);

				geom = std::unique_ptr<InputGeom>(new InputGeom);
				if (!geom || !geom->loadMesh(&ctx, path))
				{
					geom.release();
					geom = 0;
					showLog = true;
					ctx.dumpLog("Geom load log %s:", meshName);
				}
				if (sample && geom)
				{
					sample->handleMeshChanged(geom.get());
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
					this->ResetCameraAndFog(geom, sample, camx, camy, camz, camr, rx, ry);
				}

				// Do the tests.
				if (sample)
					test->doTests(sample->getNavMesh(), sample->getNavMeshQuery());
			}
		}

		ImGui::End();
	}
}

void RecastTool::Update(float dt)
{
	// Update and render
	glViewport(0, 0, m_Width, m_Height);
	glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);

	// Render 3d
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(50.0f, (float)m_Width / (float)m_Height, 1.0f, camr);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(rx, 1, 0, 0);
	glRotatef(ry, 0, 1, 0);
	glTranslatef(-camx, -camy, -camz);

	// Get hit ray position and direction.
	glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelviewMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	gluUnProject(mx, my, 0.0f, modelviewMatrix, projectionMatrix, viewport, &x, &y, &z);
	rays[0] = (float)x; rays[1] = (float)y; rays[2] = (float)z;
	gluUnProject(mx, my, 1.0f, modelviewMatrix, projectionMatrix, viewport, &x, &y, &z);
	raye[0] = (float)x; raye[1] = (float)y; raye[2] = (float)z;

	float keybSpeed = 22.0f;
	if (SDL_GetModState() & KMOD_SHIFT)
		keybSpeed *= 4.0f;

	moveW = rcClamp(moveW + dt * 4 * forward, 0.0f, 1.0f);
	moveS = rcClamp(moveS + dt * 4 * reverse, 0.0f, 1.0f);
	moveA = rcClamp(moveA + dt * 4 * left, 0.0f, 1.0f);
	moveD = rcClamp(moveD + dt * 4 * right, 0.0f, 1.0f);

	float movex = (moveD - moveA) * keybSpeed * dt;
	float movey = (moveS - moveW) * keybSpeed * dt + scrollZoom * 2.0f;
	scrollZoom = 0;

	camx += movex * (float)modelviewMatrix[0];
	camy += movex * (float)modelviewMatrix[4];
	camz += movex * (float)modelviewMatrix[8];

	camx += movey * (float)modelviewMatrix[2];
	camy += movey * (float)modelviewMatrix[6];
	camz += movey * (float)modelviewMatrix[10];
}

void RecastTool::Render(float dt)
{
	glEnable(GL_FOG);
	if (sample)
		sample->handleRender();
	if (test)
		test->handleRender();
	glDisable(GL_FOG);

	this->RenderGui(dt);
	SDL_GL_SwapWindow(m_Window.get());
}

void RecastTool::RenderGui(float dt)
{
	// Render GUI
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, m_Width, 0, m_Height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	ImGui_ImplSdl_NewFrame(m_Window.get());

	ImGui::SetNextWindowPos(ImVec2(280, m_Height - 270));
	if (ImGui::Begin("Overlay", nullptr, ImVec2(0, 250), 0.0f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs))
	{
		// Help text.
		if (showMenu)
		{
			ImGui::TextColored(ImVec4(1.0, 1.0, 1.0, 0.75), "W/S/A/D: Move  RMB: Rotate");
		}

		if (sample)
		{
			sample->handleRenderOverlay((double*)projectionMatrix, (double*)modelviewMatrix, (int*)viewport);
		}

		if (test && test->handleRenderOverlay((double*)projectionMatrix, (double*)modelviewMatrix, (int*)viewport))
		{

		}

		if (showMenu)
		{
			this->RenderPropertiesWindow();
			this->RenderToolsWindow();
			this->RenderTestCaseWindow();
			this->RenderLogWindow();
		}
		slideShow.updateAndDraw(dt, (float)m_Width, (float)m_Height);

		// Marker

		if (markerPositionSet && gluProject((GLdouble)markerPosition[0], (GLdouble)markerPosition[1], (GLdouble)markerPosition[2],
			modelviewMatrix, projectionMatrix, viewport, &x, &y, &z))
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
		ImGui::End();
	}

	glEnable(GL_DEPTH_TEST);
	ImGui::Render();
}

void RecastTool::Run()
{
	m_LastTime = SDL_GetTicks();

	glEnable(GL_CULL_FACE);

	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, camr * 0.1f);
	glFogf(GL_FOG_END, camr * 1.25f);
	glFogfv(GL_FOG_COLOR, fogColor);

	glDepthFunc(GL_LEQUAL);

	while (!done)
	{
		processHitTest = false;
		processHitTestShift = false;

		this->HandleInput();
		float dt = this->UpdateTimeStamp();
		this->ProcessHitTest();
		this->UpdateSimulation(dt);
		this->ClampFrameRate(dt);
		this->Update(dt);
		this->Render(dt);
	}
}

void RecastTool::DrawMarker(float markerPosition[3], GLdouble projectionMatrix[16], GLdouble modelviewMatrix[16], GLint viewport[4])
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

void RecastTool::ResetCameraAndFog(const std::unique_ptr<InputGeom>& geom, const std::shared_ptr<Sample>& sample, float & camx, float & camy, float & camz, float & camr, float & rx, float & ry)
{
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
