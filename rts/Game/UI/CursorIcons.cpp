#include "StdAfx.h"
#include "CursorIcons.h"
#include "CommandColors.h"
#include "MouseHandler.h"
#include "../Camera.h"
#include "../command.h"
#include "Rendering/GL/myGL.h"

#include <algorithm>


CCursorIcons* cursorIcons = NULL;


CCursorIcons::CCursorIcons()
{
}


CCursorIcons::~CCursorIcons()
{
}


void CCursorIcons::Draw()
{
	if (icons.empty() || !cmdColors.UseQueueIcons()) {
		return;
	}

	glDepthMask(GL_FALSE);

	glEnable(GL_TEXTURE_2D);
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0f, 1.0f, 0.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER, 0.01f);
	glColor4f(1.0f, 1.0f, 1.0f, cmdColors.QueueIconAlpha());
	
	int currentCmd = (icons.begin()->command + 1); // force the first binding
	CMouseCursor* currentCursor = NULL;
	
	std::set<Icon>::iterator it;
	for (it = icons.begin(); it != icons.end(); ++it) {
		if (it->command != currentCmd) {
			currentCmd = it->command;
			currentCursor = GetCursor(currentCmd);
			if (currentCursor != NULL) {
				currentCursor->BindTexture();
			}
		}
		if (currentCursor != NULL) {
			float3 winPos = camera->CalcWindowCoordinates(it->pos);
			if (winPos.z <= 1.0f) {
				currentCursor->DrawQuad((int)winPos.x, (int)winPos.y);
			}
		}
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glDepthMask(GL_TRUE);

	glViewport(gu->screenxPos,0,gu->screenx,gu->screeny);

	// clear the list	
	icons.clear();
}


CMouseCursor* CCursorIcons::GetCursor(int cmd)
{
	string cursorName;

	switch (cmd) {
		case CMD_WAIT:            cursorName = "Wait"; break;
		case CMD_MOVE:            cursorName = "Move"; break;
		case CMD_PATROL:          cursorName = "Patrol"; break;
		case CMD_FIGHT:           cursorName = "Fight"; break;
		case CMD_ATTACK:          cursorName = "Attack"; break;
		case CMD_AREA_ATTACK:     cursorName = "Attack"; break;
		case CMD_LOOPBACKATTACK:  cursorName = "Attack"; break;
		case CMD_GUARD:           cursorName = "Guard"; break;
		case CMD_REPAIR:          cursorName = "Repair"; break;
		case CMD_LOAD_UNITS:      cursorName = "Load units"; break;
		case CMD_UNLOAD_UNITS:    cursorName = "Unload units"; break;
		case CMD_UNLOAD_UNIT:     cursorName = "Unload units"; break;
		case CMD_RECLAIM:         cursorName = "Reclaim"; break;
		case CMD_DGUN:            cursorName = "DGun"; break;
		case CMD_RESURRECT:       cursorName = "Resurrect"; break;
		case CMD_CAPTURE:         cursorName = "Capture"; break;
		case CMD_SELFD:           cursorName = "SelfD"; break;
		case CMD_DEATHWATCH:      cursorName = "DeathWatch"; break;
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
		case CMD_RESTORE:
		case CMD_REPEAT:
		case CMD_TRAJECTORY:
		case CMD_AUTOREPAIRLEVEL:
*/
		default: return NULL;
	}

	map<std::string, CMouseCursor *>::const_iterator it;
	it = mouse->cursors.find(cursorName);
	if (it != mouse->cursors.end()) {
		return it->second;
	}
	
	return NULL;
}


