#include "StdAfx.h"
#include "LineDrawer.h"
#include "Game/UI/CommandColors.h"
#include "GlobalStuff.h"

CLineDrawer lineDrawer;


CLineDrawer::CLineDrawer()
{
	stippleTimer = 0.0f;
}


void CLineDrawer::UpdateLineStipple()
{
	stippleTimer += (gu->lastFrameTime * cmdColors.StippleSpeed());
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
