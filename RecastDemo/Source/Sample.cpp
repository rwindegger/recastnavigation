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

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include "Sample.h"
#include "InputGeom.h"
#include "Recast.h"
#include "RecastDebugDraw.h"
#include "DetourDebugDraw.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourCrowd.h"
#include "imgui.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include <stdarg.h>

#ifdef WIN32
#	define snprintf _snprintf
#endif

void Sample::ImGuiDrawOverlay(const ImVec2 position, const char* id, const ImVec4 &color, const char* fmt, ...)
{
	ImGui::SetNextWindowPos(position);
	if (ImGui::Begin(id, nullptr, ImVec2(0, 0), 0.0f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs))
	{
		va_list args;
		va_start(args, fmt);
		ImGui::TextColoredV(color, fmt, args);
		va_end(args);
		ImGui::End();
	}
}

Sample::Sample() :
	m_geom(0),
	m_navMesh(0),
	m_navQuery(0),
	m_crowd(0),
	m_navMeshDrawFlags(DU_DRAWNAVMESH_OFFMESHCONS | DU_DRAWNAVMESH_CLOSEDLIST),
	m_tool(0),
	m_ctx(0)
{
	resetCommonSettings();
	m_navQuery = dtAllocNavMeshQuery();
	m_crowd = dtAllocCrowd();

	for (int i = 0; i < MAX_TOOLS; i++)
		m_toolStates[i] = 0;
}

Sample::~Sample()
{
	dtFreeNavMeshQuery(m_navQuery);
	dtFreeNavMesh(m_navMesh);
	dtFreeCrowd(m_crowd);
	delete m_tool;
	for (int i = 0; i < MAX_TOOLS; i++)
		delete m_toolStates[i];
}

void Sample::setTool(SampleTool* tool)
{
	delete m_tool;
	m_tool = tool;
	if (tool)
		m_tool->init(this);
}

void Sample::handleSettings()
{
}

void Sample::handleTools()
{
}

void Sample::handleDebugMode()
{
}

void Sample::handleRender()
{
	if (!m_geom)
		return;

	DebugDrawGL dd;

	// Draw mesh
	duDebugDrawTriMesh(&dd, m_geom->getMesh()->getVerts(), m_geom->getMesh()->getVertCount(),
		m_geom->getMesh()->getTris(), m_geom->getMesh()->getNormals(), m_geom->getMesh()->getTriCount(), 0, 1.0f);
	// Draw bounds
	const float* bmin = m_geom->getMeshBoundsMin();
	const float* bmax = m_geom->getMeshBoundsMax();
	duDebugDrawBoxWire(&dd, bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2], duRGBA(255, 255, 255, 128), 1.0f);
}

void Sample::handleRenderOverlay(double* /*proj*/, double* /*model*/, int* /*view*/)
{
}

void Sample::handleMeshChanged(InputGeom* geom)
{
	m_geom = geom;
}

const float* Sample::getBoundsMin()
{
	if (!m_geom) return 0;
	return m_geom->getMeshBoundsMin();
}

const float* Sample::getBoundsMax()
{
	if (!m_geom) return 0;
	return m_geom->getMeshBoundsMax();
}

void Sample::resetCommonSettings()
{
	m_cellSize = 0.3f;
	m_cellHeight = 0.2f;
	m_agentHeight = 2.0f;
	m_agentRadius = 0.6f;
	m_agentMaxClimb = 0.9f;
	m_agentMaxSlope = 45.0f;
	m_regionMinSize = 8;
	m_regionMergeSize = 20;
	m_edgeMaxLen = 12.0f;
	m_edgeMaxError = 1.3f;
	m_vertsPerPoly = 6.0f;
	m_detailSampleDist = 6.0f;
	m_detailSampleMaxError = 1.0f;
	m_partitionType = SAMPLE_PARTITION_WATERSHED;
}

void Sample::handleCommonSettings()
{
	if (ImGui::TreeNode("Rasterization"))
	{
		ImGui::SliderFloat("Cell Size", &m_cellSize, 0.1f, 1.0f, "%.3f");
		ImGui::SliderFloat("Cell Height", &m_cellHeight, 0.1f, 1.0f, "%.3f");
		if (m_geom)
		{
			const float* bmin = m_geom->getMeshBoundsMin();
			const float* bmax = m_geom->getMeshBoundsMax();
			int gw = 0, gh = 0;
			rcCalcGridSize(bmin, bmax, m_cellSize, &gw, &gh);
			ImGui::Text("Voxels %d x %d", gw, gh);
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Agent"))
	{
		ImGui::SliderFloat("Height", &m_agentHeight, 0.1f, 5.0f, "%.3f");
		ImGui::SliderFloat("Radius", &m_agentRadius, 0.0f, 5.0f, "%.3f");
		ImGui::SliderFloat("Max Climb", &m_agentMaxClimb, 0.1f, 5.0f, "%.3f");
		ImGui::SliderFloat("Max Slope", &m_agentMaxSlope, 0.0f, 90.0f, "%.2f");
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Region"))
	{
		ImGui::SliderFloat("Min Region Size", &m_regionMinSize, 0.0f, 150.0f, "%.2f");
		ImGui::SliderFloat("Merged Region Size", &m_regionMergeSize, 0.0f, 150.0f, "%.2f");
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Partitioning"))
	{
		if (ImGui::RadioButton("Watershed", m_partitionType == SAMPLE_PARTITION_WATERSHED))
			m_partitionType = SAMPLE_PARTITION_WATERSHED;
		if (ImGui::RadioButton("Monotone", m_partitionType == SAMPLE_PARTITION_MONOTONE))
			m_partitionType = SAMPLE_PARTITION_MONOTONE;
		if (ImGui::RadioButton("Layers", m_partitionType == SAMPLE_PARTITION_LAYERS))
			m_partitionType = SAMPLE_PARTITION_LAYERS;
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Polygonization"))
	{
		ImGui::SliderFloat("Max Edge Length", &m_edgeMaxLen, 0.0f, 50.0f, "%.2f");
		ImGui::SliderFloat("Max Edge Error", &m_edgeMaxError, 0.1f, 3.0f, "%.3f");
		ImGui::SliderFloat("Verts Per Poly", &m_vertsPerPoly, 3.0f, 12.0f, "%.3f");
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Detail Mesh"))
	{
		ImGui::SliderFloat("Sample Distance", &m_detailSampleDist, 0.0f, 16.0f, "%.3f");
		ImGui::SliderFloat("Max Sample Error", &m_detailSampleMaxError, 0.0f, 16.0f, "%.3f");
		ImGui::TreePop();
	}
}

void Sample::handleClick(const float* s, const float* p, bool shift)
{
	if (m_tool)
		m_tool->handleClick(s, p, shift);
}

void Sample::handleToggle()
{
	if (m_tool)
		m_tool->handleToggle();
}

void Sample::handleStep()
{
	if (m_tool)
		m_tool->handleStep();
}

bool Sample::handleBuild()
{
	return true;
}

void Sample::handleUpdate(const float dt)
{
	if (m_tool)
		m_tool->handleUpdate(dt);
	updateToolStates(dt);
}

void Sample::updateToolStates(const float dt)
{
	for (int i = 0; i < MAX_TOOLS; i++)
	{
		if (m_toolStates[i])
			m_toolStates[i]->handleUpdate(dt);
	}
}

void Sample::initToolStates(Sample* sample)
{
	for (int i = 0; i < MAX_TOOLS; i++)
	{
		if (m_toolStates[i])
			m_toolStates[i]->init(sample);
	}
}

void Sample::resetToolStates()
{
	for (int i = 0; i < MAX_TOOLS; i++)
	{
		if (m_toolStates[i])
			m_toolStates[i]->reset();
	}
}

void Sample::renderToolStates()
{
	for (int i = 0; i < MAX_TOOLS; i++)
	{
		if (m_toolStates[i])
			m_toolStates[i]->handleRender();
	}
}

void Sample::renderOverlayToolStates(double* proj, double* model, int* view)
{
	for (int i = 0; i < MAX_TOOLS; i++)
	{
		if (m_toolStates[i])
			m_toolStates[i]->handleRenderOverlay(proj, model, view);
	}
}
