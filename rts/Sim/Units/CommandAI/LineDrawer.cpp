/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// TODO: move this out of Sim, this is rendering code!

#include "StdAfx.h"

#include "LineDrawer.h"

#include "Rendering/GlobalRendering.h"
#include "Game/UI/CommandColors.h"
#include "System/GlobalUnsynced.h"

CLineDrawer lineDrawer;


CLineDrawer::CLineDrawer()
{
	stippleTimer = 0.0f;
	lines.reserve(32);
	stippled.reserve(32);
}


void CLineDrawer::UpdateLineStipple()
{
	stippleTimer += (globalRendering->lastFrameTime * cmdColors.StippleSpeed());
	stippleTimer = fmod(stippleTimer, (16.0f / 20.0f));
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

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_STIPPLE);

	for (int i = 0; i<lines.size(); ++i) {
		int size = lines[i].verts.size();
		if(size > 0) {
			glColorPointer(4, GL_FLOAT, 0, &lines[i].colors[0]);
			glVertexPointer(3, GL_FLOAT, 0, &lines[i].verts[0]);
			glDrawArrays(lines[i].type, 0, size/3);
		}
	}

	if (!stippled.empty()) {
		glEnable(GL_LINE_STIPPLE);
		for (int i = 0; i<stippled.size(); ++i) {
			int size = stippled[i].verts.size();
			if(size > 0) {
				glColorPointer(4, GL_FLOAT, 0, &stippled[i].colors[0]);
				glVertexPointer(3, GL_FLOAT, 0, &stippled[i].verts[0]);
				glDrawArrays(stippled[i].type, 0, size/3);
			}
		}
		glDisable(GL_LINE_STIPPLE);
	}

	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glEnable(GL_DEPTH_TEST);

	lines.clear();
	stippled.clear();
}
