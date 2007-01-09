#include "StdAfx.h"
#include "CursorIcons.h"
#include "CommandColors.h"
#include "MouseHandler.h"
#include "Game/command.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/UI/OutlineFont.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"

#include <algorithm>


CCursorIcons cursorIcons;


CCursorIcons::CCursorIcons()
{
	enabled = true;
}


CCursorIcons::~CCursorIcons()
{
}


void CCursorIcons::Enable(bool value)
{
	enabled = value;
}


void CCursorIcons::Clear()
{
	icons.clear();
	texts.clear();
	buildIcons.clear();
}


void CCursorIcons::Draw()
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER, 0.01f);
	glDepthMask(GL_FALSE);

	DrawCursors();

	DrawBuilds();

	glDepthMask(GL_TRUE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
}


void CCursorIcons::DrawCursors()
{
	if (icons.empty() || !cmdColors.UseQueueIcons()) {
		return;
	}
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0f, 1.0f, 0.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
 
	glColor4f(1.0f, 1.0f, 1.0f, cmdColors.QueueIconAlpha());
	
	int currentCmd = (icons.begin()->cmd + 1); // force the first binding
	CMouseCursor* currentCursor = NULL;
	
	std::set<Icon>::iterator it;
	for (it = icons.begin(); it != icons.end(); ++it) {
		const int command = it->cmd;
		if (command != currentCmd) {
			currentCmd = command;
			currentCursor = GetCursor(currentCmd);
			if (currentCursor != NULL) {
				currentCursor->BindTexture();
			}
		}
		if (currentCursor != NULL) {
			const float3 winPos = camera->CalcWindowCoordinates(it->pos);
			if (winPos.z <= 1.0f) {
				currentCursor->DrawQuad((int)winPos.x, (int)winPos.y);
			}
		}
	}
	
	DrawTexts(); // use the same transformation

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


void CCursorIcons::DrawTexts()
{
	glViewport(gu->viewPosX, 0, gu->viewSizeX, gu->viewSizeY);
	glColor4f(1.0f,  1.0f, 1.0f, 1.0f);

	const float yScale = 20.0f / (float)gu->viewSizeX;
	const float xScale = yScale / gu->aspectRatio;
	const float xPixel  = 1.0f / (xScale * (float)gu->viewSizeX);
	const float yPixel  = 1.0f / (yScale * (float)gu->viewSizeY);
	const float yOffset = -50.0f / (float)gu->viewSizeX;
	
	std::set<IconText>::iterator it;
	for (it = texts.begin(); it != texts.end(); ++it) {
		const float3 winPos = camera->CalcWindowCoordinates(it->pos);
		if (winPos.z <= 1.0f) {
			const char* text = it->text.c_str();
			const float tWidth  = xScale * font->CalcTextWidth(text);
			const float tHeight = yScale * font->CalcTextHeight(text);
			const float x = (winPos.x / (float)gu->viewSizeX) - (0.5f * tWidth);
			const float y = (winPos.y / (float)gu->viewSizeY) + tHeight + yOffset;

			glPushMatrix();
			glTranslatef(x, y, 0.0f);
			glScalef(xScale, yScale, 1.0f);
			if (outlineFont.IsEnabled()) {
				const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
				const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
				font->glPrintOutlined(text, xPixel, yPixel, white, black);
			} else {
				font->glPrintRaw(text);
			}
			glPopMatrix();
		}
	}
}


void CCursorIcons::DrawBuilds()
{
	glViewport(gu->viewPosX, 0, gu->viewSizeX, gu->viewSizeY);

	glEnable(GL_DEPTH_TEST);
	glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
	
	std::set<BuildIcon>::iterator it;
	for (it = buildIcons.begin() ; it != buildIcons.end(); ++it) {
		const UnitDef* unitDef = unitDefHandler->GetUnitByID(-(it->cmd));
		unitDrawer->DrawBuildingSample(unitDef, it->team, it->pos, it->facing);
	}

	glDisable(GL_DEPTH_TEST);
}



CMouseCursor* CCursorIcons::GetCursor(int cmd)
{
	string cursorName;

	switch (cmd) {
		case CMD_WAIT:            cursorName = "Wait";         break;
		case CMD_TIMEWAIT:        cursorName = "TimeWait";     break;
		case CMD_SQUADWAIT:       cursorName = "SquadWait";    break;
		case CMD_DEATHWAIT:       cursorName = "DeathWait";    break;
		case CMD_GATHERWAIT:      cursorName = "GatherWait";   break;
		case CMD_MOVE:            cursorName = "Move";         break;
		case CMD_PATROL:          cursorName = "Patrol";       break;
		case CMD_FIGHT:           cursorName = "Fight";        break;
		case CMD_ATTACK:          cursorName = "Attack";       break;
		case CMD_AREA_ATTACK:     cursorName = "Area attack";  break;
		case CMD_LOOPBACKATTACK:  cursorName = "Attack";       break;
		case CMD_GUARD:           cursorName = "Guard";        break;
		case CMD_REPAIR:          cursorName = "Repair";       break;
		case CMD_LOAD_UNITS:      cursorName = "Load units";   break;
		case CMD_UNLOAD_UNITS:    cursorName = "Unload units"; break;
		case CMD_UNLOAD_UNIT:     cursorName = "Unload units"; break;
		case CMD_RECLAIM:         cursorName = "Reclaim";      break;
		case CMD_DGUN:            cursorName = "DGun";         break;
		case CMD_RESURRECT:       cursorName = "Resurrect";    break;
		case CMD_CAPTURE:         cursorName = "Capture";      break;
		case CMD_SELFD:           cursorName = "SelfD";        break;
		case CMD_RESTORE:         cursorName = "Restore";      break;
/*
		case CMD_STOP:
		case CMD_AISELECT:
		case CMD_GROUPSELECT:
		case CMD_GROUPADD:
		case CMD_GROUPCLEAR:
		case CMD_FIRE_STATE:
		case CMD_MOVE_STATE:
		case CMD_SETBASE:
		case CMD_INTERNAL:
		case CMD_SET_WANTED_MAX_SPEED:
		case CMD_ONOFF:
		case CMD_CLOAK:
		case CMD_STOCKPILE:
		case CMD_REPEAT:
		case CMD_TRAJECTORY:
		case CMD_AUTOREPAIRLEVEL:
*/
		default: return NULL;
	}

	map<std::string, CMouseCursor *>::const_iterator it;
	it = mouse->cursorCommandMap.find(cursorName);
	if (it != mouse->cursorCommandMap.end()) {
		return it->second;
	}
	
	return NULL;
}


