/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LineDrawer.h"

#include <cmath>

#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Game/Camera.h"
#include "Game/UI/CommandColors.h"

CLineDrawer lineDrawer;


CLineDrawer::CLineDrawer()
	: lineStipple(false)
	, useColorRestarts(false)
	, useRestartColor(false)
	, restartAlpha(0.0f)
	, stippleTimer(0.0f)
	, lastPos(ZeroVector)
	, restartColor(nullptr)
	, lastColor(nullptr)
{
	regularLines.reserve(32);
	stippleLines.reserve(32);
}


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
	LinePair* ptr;
	if (lineStipple) {
		stippleLines.emplace_back();
		ptr = &stippleLines.back();
	} else {
		regularLines.emplace_back();
		ptr = &regularLines.back();
	}
	LinePair& p = *ptr;

	if (!useColorRestarts)	 {
		p.glType = GL_LINE_STRIP;

		p.verts.push_back({lastPos, lastColor});
	} else {
		p.glType = GL_LINES;
	}
}


void CLineDrawer::DrawAll(bool onMiniMap)
{
	// TODO: grab the minimap transform (DrawForReal->EnterNormalizedCoors->DrawWorldStuff)
	if (onMiniMap)
		return;
	if (regularLines.empty() && stippleLines.empty())
		return;

	glAttribStatePtr->PushEnableBit();
	glAttribStatePtr->DisableDepthTest();
	glDisable(GL_LINE_STIPPLE);


	GL::RenderDataBufferC* buffer = GL::GetRenderBufferC();
	Shader::IProgramObject* shader = buffer->GetShader();

	shader->Enable();
	shader->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, camera->GetProjectionMatrix());
	shader->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, camera->GetViewMatrix());

	for (const auto& regularLine : regularLines) {
		const size_t size = regularLine.verts.size();

		// TODO: batch lines of equal type
		if (size > 0) {
			buffer->SafeAppend(regularLine.verts.data(), size);
			buffer->Submit(regularLine.glType);
		}
	}

	if (!stippleLines.empty()) {
		glEnable(GL_LINE_STIPPLE);

		for (const auto& stippleLine : stippleLines) {
			const size_t size = stippleLine.verts.size();

			if (size > 0) {
				buffer->SafeAppend(stippleLine.verts.data(), size);
				buffer->Submit(stippleLine.glType);
			}
		}

		glDisable(GL_LINE_STIPPLE);
	}

	shader->Disable();
	glAttribStatePtr->PopBits();

	regularLines.clear();
	stippleLines.clear();
}

