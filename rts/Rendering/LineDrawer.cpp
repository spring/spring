/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LineDrawer.h"

#include <cmath>

#include "Game/Camera.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/MiniMap.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "System/ContainerUtil.h"

CLineDrawer lineDrawer;


void CLineDrawer::UpdateLineStipple()
{
	stippleTimer += (globalRendering->lastFrameTime * 0.001f * cmdColors.StippleSpeed());
	stippleTimer = std::fmod(stippleTimer, (16.0f / 20.0f));
}


void CLineDrawer::SetupLineStipple()
{
	const unsigned int stipPat = (0xffff & cmdColors.StipplePattern());

	lineStipple = ((stipPat != 0x0000) && (stipPat != 0xffff));

	if (!lineStipple)
		return;

	const unsigned int fullPat = (stipPat << 16) | (stipPat & 0x0000ffff);
	const int shiftBits = 15 - (int(stippleTimer * 20.0f) % 16);

	glLineStipple(cmdColors.StippleFactor(), (fullPat >> shiftBits));
}


void CLineDrawer::Restart()
{
	LinePair& line = spring::VectorEmplaceBack(lineStipple? stippleLines: regularLines);

	if ((line.glType = useColorRestarts? GL_LINES: GL_LINE_STRIP) == GL_LINES)
		return;

	line.verts.push_back({lastPos, lastColor});
}


void CLineDrawer::DrawAll(bool onMiniMap)
{
	if (regularLines.empty() && stippleLines.empty())
		return;

	// batch lines of equal type
	std::sort(regularLines.begin(), regularLines.end(), [](const LinePair& a, const LinePair& b) { return (a.glType < b.glType); });
	std::sort(stippleLines.begin(), stippleLines.end(), [](const LinePair& a, const LinePair& b) { return (a.glType < b.glType); });

	glAttribStatePtr->PushEnableBit();
	glAttribStatePtr->DisableDepthTest();
	glAttribStatePtr->Disable(GL_LINE_STIPPLE);


	GL::RenderDataBufferC* buffer = GL::GetRenderBufferC();
	Shader::IProgramObject* shader = buffer->GetShader();

	const CMatrix44f& projMat = onMiniMap? minimap->GetProjMat(0): camera->GetProjectionMatrix();
	const CMatrix44f& viewMat = onMiniMap? minimap->GetViewMat(0): camera->GetViewMatrix();

	shader->Enable();
	shader->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, projMat);
	shader->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, viewMat);

	if (!regularLines.empty()) {
		unsigned int glType = regularLines[0].glType;

		for (const auto& regularLine : regularLines) {
			if (regularLine.verts.empty())
				continue;

			buffer->SafeAppend(regularLine.verts.data(), regularLine.verts.size());

			if (regularLine.glType == glType)
				continue;

			buffer->Submit(glType = regularLine.glType);
		}

		buffer->Submit(regularLines[regularLines.size() - 1].glType);
	}

	if (!stippleLines.empty()) {
		unsigned int glType = stippleLines[0].glType;

		for (const auto& stippleLine : stippleLines) {
			if (stippleLine.verts.empty())
				continue;

			buffer->SafeAppend(stippleLine.verts.data(), stippleLine.verts.size());

			if (stippleLine.glType == glType)
				continue;

			buffer->Submit(glType = stippleLine.glType);
		}

		// glAttribStatePtr->Enable(GL_LINE_STIPPLE);
		buffer->Submit(stippleLines[stippleLines.size() - 1].glType);
		// glAttribStatePtr->Disable(GL_LINE_STIPPLE);
	}

	shader->Disable();
	glAttribStatePtr->PopBits();

	regularLines.clear();
	stippleLines.clear();
}

