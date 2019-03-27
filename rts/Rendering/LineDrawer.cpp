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

	if (!(lineStipple = ((stipPat != 0x0000) && (stipPat != 0xffff))))
		return;

	const unsigned int fullPat = (stipPat << 16) | (stipPat & 0x0000ffff);
	const int shiftBits = 15 - (int(stippleTimer * 20.0f) % 16);

	glLineStipple(cmdColors.StippleFactor(), (fullPat >> shiftBits));
}


void CLineDrawer::Restart()
{
	Line& line = spring::VectorEmplaceBack(lineStipple? stippleLines[useColorRestarts]: regularLines[useColorRestarts]);

	if (useColorRestarts)
		return;

	line.push_back({lastPos, lastColor});
}


void CLineDrawer::DrawAll(bool onMiniMap)
{
	if (!HaveRegularLines() && !HaveStippleLines())
		return;

	glAttribStatePtr->PushEnableBit();
	glAttribStatePtr->DisableDepthTest();


	GL::RenderDataBufferC* buffer = GL::GetRenderBufferC();
	Shader::IProgramObject* shader = buffer->GetShader();

	const CMatrix44f& projMat = onMiniMap? minimap->GetProjMat(0): camera->GetProjectionMatrix();
	const CMatrix44f& viewMat = onMiniMap? minimap->GetViewMat(0): camera->GetViewMatrix();

	const auto DrawLines = [&](const std::vector<Line>& lines, unsigned int glType) {
		for (const auto& line: lines) {
			if (line.empty())
				continue;

			buffer->SafeAppend(line.data(), line.size());
		}

		buffer->Submit(glType);
	};

	shader->Enable();
	shader->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, projMat);
	shader->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, viewMat);

	{
		DrawLines(regularLines[0], GL_LINE_LOOP);
		DrawLines(regularLines[1], GL_LINES    );
	}
	{
		// [deprecated]
		// glAttribStatePtr->Enable(GL_LINE_STIPPLE);
		DrawLines(stippleLines[0], GL_LINE_LOOP);
		DrawLines(stippleLines[1], GL_LINES    );
		// glAttribStatePtr->Disable(GL_LINE_STIPPLE);
	}

	shader->Disable();
	glAttribStatePtr->PopBits();

	for (int i = 0; i < 2; i++) {
		regularLines[i].clear();
		stippleLines[i].clear();
	}
}

