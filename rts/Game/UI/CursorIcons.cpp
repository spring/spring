/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>

#include "CursorIcons.h"
#include "CommandColors.h"
#include "MouseHandler.h"
#include "GuiHandler.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
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
	glAttribStatePtr->PushBits(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAttribStatePtr->DisableDepthMask();

	Sort();
	DrawCursors();
	DrawTexts();
	DrawBuilds();
	Clear();

	glBindTexture(GL_TEXTURE_2D, 0);
	glAttribStatePtr->PopBits();
}


void CCursorIcons::DrawCursors()
{
	if (icons.empty() || !cmdColors.UseQueueIcons())
		return;

	GL::RenderDataBufferTC* buffer = GL::GetRenderBufferTC();
	Shader::IProgramObject* shader = buffer->GetShader();

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));
	shader->SetUniform("u_alpha_test_ctrl", 0.01f, 1.0f, 0.0f, 0.0f); // test > 0.01

	const SColor iconColor = {1.0f, 1.0f, 1.0f, cmdColors.QueueIconAlpha()};


	const CMouseCursor* currentCursor = nullptr;

	// force the first binding
	int currentCommand = icons[0].cmd + 1;

	for (const Icon& icon: icons) {
		if (icon.cmd != currentCommand) {
			if ((currentCursor = GetCursor(currentCommand = icon.cmd)) == nullptr)
				continue;

			buffer->Submit(GL_TRIANGLES);
			currentCursor->BindTexture();
		}

		const float3 winCoors = camera->CalcWindowCoordinates(icon.pos);
		const float2 winScale = {cmdColors.QueueIconScale(), 1.0f};
		const float4& matParams = currentCursor->CalcFrameMatrixParams(winCoors, winScale);

		CMatrix44f cursorMat;
		cursorMat.Translate(matParams.x, matParams.y, 0.0f);
		cursorMat.Scale({matParams.z, matParams.w, 1.0f});

		buffer->SafeAppend({cursorMat * ICON_VERTS[0], ICON_TXCDS[0].x, ICON_TXCDS[0].y, iconColor}); // tl
		buffer->SafeAppend({cursorMat * ICON_VERTS[1], ICON_TXCDS[1].x, ICON_TXCDS[1].y, iconColor}); // bl
		buffer->SafeAppend({cursorMat * ICON_VERTS[2], ICON_TXCDS[2].x, ICON_TXCDS[2].y, iconColor}); // br

		buffer->SafeAppend({cursorMat * ICON_VERTS[2], ICON_TXCDS[2].x, ICON_TXCDS[2].y, iconColor}); // br
		buffer->SafeAppend({cursorMat * ICON_VERTS[3], ICON_TXCDS[3].x, ICON_TXCDS[3].y, iconColor}); // tr
		buffer->SafeAppend({cursorMat * ICON_VERTS[0], ICON_TXCDS[0].x, ICON_TXCDS[0].y, iconColor}); // tl
	}

	buffer->Submit(GL_TRIANGLES);
	shader->SetUniform("u_alpha_test_ctrl", 0.0f, 0.0f, 0.0f, 1.0f); // no test
	shader->Disable();
}


void CCursorIcons::DrawTexts()
{
	if (texts.empty())
		return;

	glAttribStatePtr->ViewPort(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);

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
	glAttribStatePtr->ViewPort(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	glAttribStatePtr->EnableDepthTest();

	if (!buildIcons.empty()) {
		const auto setupStateFunc = [&](const BuildIcon& bi, const S3DModel* mdl) {
			unitDrawer->SetupAlphaDrawing(false, true);
			unitDrawer->PushModelRenderState(mdl);
			unitDrawer->SetTeamColour(bi.team, float2(0.25f, 1.0f));
			return (unitDrawer->GetDrawerState(DRAWER_STATE_SEL));
		};
		const auto resetStateFunc = [&](const BuildIcon&, const S3DModel* mdl) {
			unitDrawer->PopModelRenderState(mdl);
			unitDrawer->ResetAlphaDrawing(false);
		};
		const auto nextModelFunc = [&](const BuildIcon& bi, const S3DModel* mdl) -> const S3DModel* {
			const UnitDef* def = unitDefHandler->GetUnitDefByID(-bi.cmd);
			const S3DModel* nxt = def->LoadModel();

			if (mdl == nullptr || mdl == nxt)
				return nxt;

			// icons are already sorted by type and team
			unitDrawer->PopModelRenderState(mdl);
			unitDrawer->PushModelRenderState(nxt);
			unitDrawer->SetTeamColour(bi.team, float2(0.25f, 1.0f));
			return nxt;
		};
		const auto drawModelFunc = [&](const BuildIcon& bi, const S3DModel* mdl, const IUnitDrawerState* uds) {
			unitDrawer->DrawStaticModelRaw(mdl, uds, bi.pos, bi.facing);
		};

		unitDrawer->DrawStaticModelBatch<BuildIcon>(buildIcons, setupStateFunc, resetStateFunc, nextModelFunc, drawModelFunc);
	}

	glAttribStatePtr->DisableDepthTest();
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

