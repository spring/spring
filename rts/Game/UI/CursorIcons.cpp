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
#include "Rendering/Units/UnitDrawer.h"
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

void CCursorIcons::SetCustomType(int cmdID, const std::string& cursor)
{
	if (cursor.empty()) {
		customTypes.erase(cmdID);
	} else {
		customTypes[cmdID] = cursor;
	}
}


void CCursorIcons::Draw()
{
	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	Sort();
	DrawCursors();
	DrawTexts();
	DrawBuilds();
	Clear();

	glBindTexture(GL_TEXTURE_2D, 0);
	glPopAttrib();
}


void CCursorIcons::Sort()
{
	// sort to minimize the number of texture bindings, and to
	// avoid overdraw from multiple units with the same command

	spring::VectorSortUnique(icons);
	spring::VectorSortUnique(texts);
	spring::VectorSortUnique(buildIcons);
}

void CCursorIcons::DrawCursors() const
{
	if (icons.empty() || !cmdColors.UseQueueIcons())
		return;

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_TC>();
	auto& sh = rb.GetShader();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));

	sh.Enable();
	sh.SetUniform("alphaCtrl", 0.01f, 1.0f, 0.0f, 0.0f); // test > 0.01

	const SColor iconColor = { 1.0f, 1.0f, 1.0f, cmdColors.QueueIconAlpha() };

	const CMouseCursor* lastCursor = nullptr;

	for (const Icon& icon : icons) {
		const CMouseCursor* currentCursor = GetCursor(icon.cmd);
		if (!currentCursor)
			continue;

		if (currentCursor != lastCursor) {
			rb.Submit(GL_TRIANGLES);
			currentCursor->BindTexture();

			lastCursor = currentCursor;
		}

		const float3 winCoors = camera->CalcWindowCoordinates(icon.pos);
		const float2 winScale = { cmdColors.QueueIconScale(), 1.0f };
		const float4& matParams = currentCursor->CalcFrameMatrixParams(winCoors, winScale);

		CMatrix44f cursorMat;
		cursorMat.Translate(matParams.x, matParams.y, 0.0f);
		cursorMat.Scale({ matParams.z, matParams.w, 1.0f });

		rb.AddQuadTriangles(
			{ cursorMat * ICON_VERTS[0], ICON_TXCDS[0].x, ICON_TXCDS[0].y, iconColor }, // tl
			{ cursorMat * ICON_VERTS[3], ICON_TXCDS[3].x, ICON_TXCDS[3].y, iconColor }, // tr
			{ cursorMat * ICON_VERTS[2], ICON_TXCDS[2].x, ICON_TXCDS[2].y, iconColor }, // br
			{ cursorMat * ICON_VERTS[1], ICON_TXCDS[1].x, ICON_TXCDS[1].y, iconColor }  // bl
		);
	}
	rb.Submit(GL_TRIANGLES);

	sh.SetUniform("alphaCtrl", 0.0f, 0.0f, 0.0f, 1.0f); // no test
	sh.Disable();

	glBindTexture(GL_TEXTURE_2D, 0);

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


void CCursorIcons::DrawTexts() const
{
	if (texts.empty())
		return;

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	const float fontScale = 1.0f;
	const float yOffset = 50.0f * globalRendering->pixelY;

	font->Begin();
	font->SetColors(); //default

	for (const auto& text : texts) {
		const float3 winPos = camera->CalcWindowCoordinates(text.pos);
		if (winPos.z <= 1.0f) {
			const float x = (winPos.x * globalRendering->pixelX);
			const float y = (winPos.y * globalRendering->pixelY) + yOffset;

			if (guihandler->GetOutlineFonts()) {
				font->glPrint(x, y, fontScale, FONT_OUTLINE | FONT_CENTER | FONT_TOP | FONT_SCALE | FONT_NORM, text.text);
			} else {
				font->glPrint(x, y, fontScale, FONT_SCALE | FONT_CENTER | FONT_TOP | FONT_NORM, text.text);
			}
		}
	}
	font->End();
}


void CCursorIcons::DrawBuilds() const
{
	unitDrawer->DrawBuildIcons(buildIcons);
}



const CMouseCursor* CCursorIcons::GetCursor(int cmd) const
{
	std::string cursorName;

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
		case CMD_ONOFF:
		case CMD_CLOAK:
		case CMD_STOCKPILE:
		case CMD_REPEAT:
		case CMD_TRAJECTORY:
		case CMD_AUTOREPAIRLEVEL:
*/
		default: {
			const auto it = customTypes.find(cmd);
			if (it == customTypes.cend()) {
				return NULL;
			}
			cursorName = it->second;
		}
	}

	// look for the cursor of this name assigned in MouseHandler
	return mouse->FindCursor(cursorName);
}
