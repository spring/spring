/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>


#include "CursorIcons.h"
#include "CommandColors.h"
#include "MouseHandler.h"
#include "GuiHandler.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"


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


void CCursorIcons::SetCustomType(int cmdID, const string& cursor)
{
	if (cursor.empty()) {
		customTypes.erase(cmdID);
	} else {
		customTypes[cmdID] = cursor;
	}
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
	const CMouseCursor* currentCursor = NULL;

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
	glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	glColor4f(1.0f,  1.0f, 1.0f, 1.0f);

	const float fontScale = 1.0f;
	const float yOffset = 50.0f * globalRendering->pixelY;

	font->Begin();
	font->SetColors(); //default

	std::set<IconText>::iterator it;
	for (it = texts.begin(); it != texts.end(); ++it) {
		const float3 winPos = camera->CalcWindowCoordinates(it->pos);
		if (winPos.z <= 1.0f) {
			const float x = (winPos.x * globalRendering->pixelX);
			const float y = (winPos.y * globalRendering->pixelY) + yOffset;

			if (guihandler->GetOutlineFonts()) {
				font->glPrint(x, y, fontScale, FONT_OUTLINE | FONT_CENTER | FONT_TOP | FONT_SCALE | FONT_NORM, it->text);
			} else {
				font->glPrint(x, y, fontScale, FONT_SCALE | FONT_CENTER | FONT_TOP | FONT_NORM, it->text);
			}
		}
	}
	font->End();
}


void CCursorIcons::DrawBuilds()
{
	glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);

	glEnable(GL_DEPTH_TEST);
	glColor4f(1.0f, 1.0f, 1.0f, 0.3f);

	for (auto it = buildIcons.begin() ; it != buildIcons.end(); ++it) {
		CUnitDrawer::DrawBuildingSample(unitDefHandler->GetUnitDefByID(-(it->cmd)), it->team, it->pos, it->facing);
	}

	glDisable(GL_DEPTH_TEST);
}



const CMouseCursor* CCursorIcons::GetCursor(int cmd) const
{
	string cursorName;

	switch (cmd) {
		case CMD_WAIT:            cursorName = "Wait";         break;
		case CMD_TIMEWAIT:        cursorName = "TimeWait";     break;
		case CMD_SQUADWAIT:       cursorName = "SquadWait";    break;
		case CMD_DEATHWAIT:       cursorName = "Wait";         break; // there is a "DeathWait" cursor, but to prevent cheating, we have to use the same like for CMD_WAIT
		case CMD_GATHERWAIT:      cursorName = "GatherWait";   break;
		case CMD_MOVE:            cursorName = "Move";         break;
		case CMD_PATROL:          cursorName = "Patrol";       break;
		case CMD_FIGHT:           cursorName = "Fight";        break;
		case CMD_ATTACK:          cursorName = "Attack";       break;
		case CMD_AREA_ATTACK:     cursorName = "Area attack";  break;
		case CMD_GUARD:           cursorName = "Guard";        break;
		case CMD_REPAIR:          cursorName = "Repair";       break;
		case CMD_LOAD_ONTO:       cursorName = "Load units";   break;
		case CMD_LOAD_UNITS:      cursorName = "Load units";   break;
		case CMD_UNLOAD_UNITS:    cursorName = "Unload units"; break;
		case CMD_UNLOAD_UNIT:     cursorName = "Unload units"; break;
		case CMD_RECLAIM:         cursorName = "Reclaim";      break;
		case CMD_MANUALFIRE:      cursorName = "ManualFire";   break;
		case CMD_RESURRECT:       cursorName = "Resurrect";    break;
		case CMD_CAPTURE:         cursorName = "Capture";      break;
		case CMD_SELFD:           cursorName = "SelfD";        break;
		case CMD_RESTORE:         cursorName = "Restore";      break;
/*
		case CMD_STOP:
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
		default: {
			std::map<int, std::string>::const_iterator it = customTypes.find(cmd);
			if (it == customTypes.end()) {
				return NULL;
			}
			cursorName = it->second;
		}
	}

	// look for the cursor of this name assigned in MouseHandler
	return mouse->FindCursor(cursorName);
}
