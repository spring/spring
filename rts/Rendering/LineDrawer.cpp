/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// TODO: move this out of Sim, this is rendering code!

#include "LineDrawer.h"

#include <cmath>

#include "Rendering/GlobalRendering.h"
#include "Game/UI/CommandColors.h"

CLineDrawer lineDrawer;


CLineDrawer::CLineDrawer()
	: lineStipple(false)
	, useColorRestarts(false)
	, useRestartColor(false)
	, restartAlpha(0.0f)
	, restartColor(NULL)
	, lastPos(ZeroVector)
	, lastColor(NULL)
	, stippleTimer(0.0f)
{
	lines.reserve(32);
	stippled.reserve(32);
}


void CLineDrawer::UpdateLineStipple()
{
	stippleTimer += (globalRendering->lastFrameTime * 0.001f * cmdColors.StippleSpeed());
	stippleTimer = std::fmod(stippleTimer, (16.0f / 20.0f));
}


void CLineDrawer::SetupLineStipple()
{
	const unsigned int stipPat = (0xffff & cmdColors.StipplePattern());
	if ((stipPat != 0x0000) && (stipPat != 0xffff)) {
		lineStipple = true;
	} else {
		lineStipple = false;
		return;
	}
	const unsigned int fullPat = (stipPat << 16) | (stipPat & 0x0000ffff);
	const int shiftBits = 15 - (int(stippleTimer * 20.0f) % 16);
	glLineStipple(cmdColors.StippleFactor(), (fullPat >> shiftBits));
}


void CLineDrawer::DrawAll()
{
	if (lines.empty() && stippled.empty())
		return;
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_STIPPLE);

	for (int i = 0; i<lines.size(); ++i) {
		int size = lines[i].colors.size();
		if(size > 0) {
			glColorPointer(4, GL_FLOAT, 0, &lines[i].colors[0]);
			glVertexPointer(3, GL_FLOAT, 0, &lines[i].verts[0]);
			glDrawArrays(lines[i].type, 0, size/4);
		}
	}

	if (!stippled.empty()) {
		glEnable(GL_LINE_STIPPLE);
		for (int i = 0; i<stippled.size(); ++i) {
			int size = stippled[i].colors.size();
			if(size > 0) {
				glColorPointer(4, GL_FLOAT, 0, &stippled[i].colors[0]);
				glVertexPointer(3, GL_FLOAT, 0, &stippled[i].verts[0]);
				glDrawArrays(stippled[i].type, 0, size/4);
			}
		}
		glDisable(GL_LINE_STIPPLE);
	}

	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glPopAttrib();

	lines.clear();
	stippled.clear();
}
