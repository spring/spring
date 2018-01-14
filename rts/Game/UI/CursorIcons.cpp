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
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"


CCursorIcons cursorIcons;

static constexpr float3 ICON_VERTS[] = {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
static constexpr float2 ICON_TXCDS[] = {{0.0f, 0.0f      }, {0.0f, 1.0f      }, {1.0f, 1.0f      }, {1.0f, 0.0f      }};


void CCursorIcons::Sort()
{
	// sort to minimize the number of texture bindings, and to
	// avoid overdraw from multiple units with the same command
	std::sort(icons.begin(), icons.end());
	std::sort(texts.begin(), texts.end());
	std::sort(buildIcons.begin(), buildIcons.end());

	icons.erase(std::unique(icons.begin(), icons.end()), icons.end());
	texts.erase(std::unique(texts.begin(), texts.end()), texts.end()); 
	buildIcons.erase(std::unique(buildIcons.begin(), buildIcons.end()), buildIcons.end());  
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
	glAlphaFunc(GL_GREATER, 0.01f);
	glDepthMask(GL_FALSE);

	Sort();
	DrawCursors();
	DrawTexts();
	DrawBuilds();
	Clear();

	glBindTexture(GL_TEXTURE_2D, 0);
	glPopAttrib();
}


void CCursorIcons::DrawCursors()
{
	if (icons.empty() || !cmdColors.UseQueueIcons())
		return;

	GL::RenderDataBufferTC* buffer = GL::GetRenderBufferTC();
	Shader::IProgramObject* shader = buffer->GetShader();

	shader->Enable();
	shader->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, CMatrix44f::Identity());
	shader->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f, globalRendering->supportClipSpaceControl * 1.0f));

	const SColor iconColor = {1.0f, 1.0f, 1.0f, cmdColors.QueueIconAlpha()};


	const CMouseCursor* currentCursor = nullptr;

	// force the first binding
	int currentCommand = icons[0].cmd + 1;

	for (const Icon& icon: icons) {
		if (icon.cmd != currentCommand) {
			if ((currentCursor = GetCursor(currentCommand = icon.cmd)) == nullptr)
				continue;

			buffer->Submit(GL_QUADS);
			currentCursor->BindTexture();
		}

		const float3& winCoors = camera->CalcWindowCoordinates(icon.pos);
		const float4& matParams = currentCursor->CalcFrameMatrixParams(winCoors);

		CMatrix44f cursorMat;
		cursorMat.Translate(matParams.x, matParams.y, 0.0f);
		cursorMat.Scale({matParams.z, matParams.w, 1.0f});

		buffer->SafeAppend({cursorMat * ICON_VERTS[0], ICON_TXCDS[0].x, ICON_TXCDS[0].y, iconColor});
		buffer->SafeAppend({cursorMat * ICON_VERTS[1], ICON_TXCDS[1].x, ICON_TXCDS[1].y, iconColor});
		buffer->SafeAppend({cursorMat * ICON_VERTS[2], ICON_TXCDS[2].x, ICON_TXCDS[2].y, iconColor});
		buffer->SafeAppend({cursorMat * ICON_VERTS[3], ICON_TXCDS[3].x, ICON_TXCDS[3].y, iconColor});
	}

	buffer->Submit(GL_QUADS);
	shader->Disable();
}


void CCursorIcons::DrawTexts()
{
	if (texts.empty())
		return;

	glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);

	const unsigned int fontFlags = (FONT_OUTLINE * guihandler->GetOutlineFonts()) | FONT_SCALE | FONT_CENTER | FONT_TOP | FONT_NORM | FONT_BUFFERED;

	const float fontScale = 1.0f;
	const float yOffset = 50.0f * globalRendering->pixelY;

	font->SetColors(); // default

	for (const IconText& iconText: texts) {
		const float3 winCoors = camera->CalcWindowCoordinates(iconText.pos);

		if (winCoors.z > 1.0f)
			continue;

		const float x = (winCoors.x * globalRendering->pixelX);
		const float y = (winCoors.y * globalRendering->pixelY) + yOffset;

		font->glPrint(x, y, fontScale, fontFlags, iconText.text);
	}

	font->DrawBufferedGL4();
}


void CCursorIcons::DrawBuilds()
{
	glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);

	glEnable(GL_DEPTH_TEST);
	glColor4f(1.0f, 1.0f, 1.0f, 0.3f);

	for (auto it = buildIcons.begin() ; it != buildIcons.end(); ++it) {
		GL::PushMatrix();
		GL::LoadIdentity();
		GL::Translate(it->pos);
		GL::RotateY(it->facing * 90.0f);

		CUnitDrawer::DrawObjectDefAlpha(unitDefHandler->GetUnitDefByID(-(it->cmd)), it->team, false);

		GL::PopMatrix();
	}

	glDisable(GL_DEPTH_TEST);
}



const CMouseCursor* CCursorIcons::GetCursor(int cmd) const
{
	switch (cmd) {
		case CMD_WAIT:            return (mouse->FindCursor("Wait"        )); break;
		case CMD_TIMEWAIT:        return (mouse->FindCursor("TimeWait"    )); break;
		case CMD_SQUADWAIT:       return (mouse->FindCursor("SquadWait"   )); break;
		case CMD_DEATHWAIT:       return (mouse->FindCursor("Wait"        )); break; // there is a "DeathWait" cursor, but use CMD_WAIT's to prevent cheating
		case CMD_GATHERWAIT:      return (mouse->FindCursor("GatherWait"  )); break;
		case CMD_MOVE:            return (mouse->FindCursor("Move"        )); break;
		case CMD_PATROL:          return (mouse->FindCursor("Patrol"      )); break;
		case CMD_FIGHT:           return (mouse->FindCursor("Fight"       )); break;
		case CMD_ATTACK:          return (mouse->FindCursor("Attack"      )); break;
		case CMD_AREA_ATTACK:     return (mouse->FindCursor("Area attack" )); break;
		case CMD_GUARD:           return (mouse->FindCursor("Guard"       )); break;
		case CMD_REPAIR:          return (mouse->FindCursor("Repair"      )); break;
		case CMD_LOAD_ONTO:       return (mouse->FindCursor("Load units"  )); break;
		case CMD_LOAD_UNITS:      return (mouse->FindCursor("Load units"  )); break;
		case CMD_UNLOAD_UNITS:    return (mouse->FindCursor("Unload units")); break;
		case CMD_UNLOAD_UNIT:     return (mouse->FindCursor("Unload units")); break;
		case CMD_RECLAIM:         return (mouse->FindCursor("Reclaim"     )); break;
		case CMD_MANUALFIRE:      return (mouse->FindCursor("ManualFire"  )); break;
		case CMD_RESURRECT:       return (mouse->FindCursor("Resurrect"   )); break;
		case CMD_CAPTURE:         return (mouse->FindCursor("Capture"     )); break;
		case CMD_SELFD:           return (mouse->FindCursor("SelfD"       )); break;
		case CMD_RESTORE:         return (mouse->FindCursor("Restore"     )); break;
		#if 0
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
		#endif
		default: break;
	}

	const auto it = customTypes.find(cmd);

	// look for the cursor of this name assigned in MouseHandler
	if (it != customTypes.end())
		return (mouse->FindCursor(it->second));

	return nullptr;
}

