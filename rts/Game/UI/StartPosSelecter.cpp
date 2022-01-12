/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "StartPosSelecter.h"
#include "MouseHandler.h"
#include "Game/Camera.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/InMapDraw.h"
#include "Game/Players/Player.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/Fonts/glFont.h"
#include "Net/Protocol/NetProtocol.h"
#include "Sim/Misc/TeamHandler.h"


CStartPosSelecter* CStartPosSelecter::selector = nullptr;


CStartPosSelecter::CStartPosSelecter() : CInputReceiver(BACK)
{
	showReadyBox = true;
	startPosSet = false;

	readyBox.x1 = 0.71f;
	readyBox.y1 = 0.72f;
	readyBox.x2 = 0.81f;
	readyBox.y2 = 0.76f;

	selector = this;
}

CStartPosSelecter::~CStartPosSelecter()
{
	selector = nullptr;
}


bool CStartPosSelecter::Ready(bool luaForcedReady)
{
	if (!gs->PreSimFrame()) {
		delete this;
		return true;
	}

	const CTeam* mt = teamHandler.Team(gu->myTeam);
	const float3* sp = nullptr;

	// player did not set a startpos yet, so do not let
	// him ready up if he clicked on the ready-box first
	//
	// do allow in the special case where player already
	// sent a RDYSTATE_UPDATED and then rejoined the game
	if (!mt->HasValidStartPos()) {
		if (!startPosSet && !luaForcedReady)
			return false;

		sp = &setStartPos;
	} else {
		sp = &mt->GetStartPos();
	}

	if (luaForcedReady) {
		clientNet->Send(CBaseNetProtocol::Get().SendStartPos(gu->myPlayerNum, gu->myTeam, CPlayer::PLAYER_RDYSTATE_FORCED, sp->x, sp->y, sp->z));
	} else {
		clientNet->Send(CBaseNetProtocol::Get().SendStartPos(gu->myPlayerNum, gu->myTeam, CPlayer::PLAYER_RDYSTATE_READIED, sp->x, sp->y, sp->z));
	}

	delete this;
	return true;
}


bool CStartPosSelecter::MousePress(int x, int y, int button)
{
	const float mx = MouseX(x);
	const float my = MouseY(y);

	if ((showReadyBox && InBox(mx, my, readyBox)) || !gs->PreSimFrame())
		return (!Ready(false));

	const float dist = CGround::LineGroundCol(camera->GetPos(), camera->GetPos() + mouse->dir * camera->GetFarPlaneDist() * 1.4f, false);

	if (dist < 0.0f)
		return true;

	inMapDrawer->SendErase(setStartPos);
	startPosSet = true;
	setStartPos = camera->GetPos() + mouse->dir * dist;
	clientNet->Send(CBaseNetProtocol::Get().SendStartPos(gu->myPlayerNum, gu->myTeam, CPlayer::PLAYER_RDYSTATE_UPDATED, setStartPos.x, setStartPos.y, setStartPos.z));

	return true;
}


#if 0
void CStartPosSelecter::DrawStartBox(GL::RenderDataBufferC* buffer, Shader::IProgramObject* shader) const
{
	glAttribStatePtr->EnableDepthTest();

	const std::vector<AllyTeam>& allyStartData = CGameSetup::GetAllyStartingData();
	const AllyTeam& myStartData = allyStartData[gu->myAllyTeam];

	const float by = myStartData.startRectTop * mapDims.mapy * SQUARE_SIZE;
	const float bx = myStartData.startRectLeft * mapDims.mapx * SQUARE_SIZE;

	const float dy = (myStartData.startRectBottom - myStartData.startRectTop ) * mapDims.mapy * SQUARE_SIZE / 10;
	const float dx = (myStartData.startRectRight  - myStartData.startRectLeft) * mapDims.mapx * SQUARE_SIZE / 10;

	// draw starting-rectangle restrictions
	for (int a = 0; a < 10; ++a) {
		float3 pos1(bx + (a    ) * dx, 0.0f, by); // tl
		float3 pos2(bx + (a + 1) * dx, 0.0f, by); // tr

		pos1.y = CGround::GetHeightAboveWater(pos1.x, pos1.z, false);
		pos2.y = CGround::GetHeightAboveWater(pos2.x, pos2.z, false);

		buffer->SafeAppend({pos1                    , {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos2                    , {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos2 + UpVector * 100.0f, {0.2f, 0.8f, 0.2f, 0.5f}});

		buffer->SafeAppend({pos2 + UpVector * 100.0f, {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos1 + UpVector * 100.0f, {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos1                    , {0.2f, 0.8f, 0.2f, 0.5f}});


		pos1 = float3(bx + (a    ) * dx, 0.0f, by + dy * 10.0f);
		pos2 = float3(bx + (a + 1) * dx, 0.0f, by + dy * 10.0f);
		pos1.y = CGround::GetHeightAboveWater(pos1.x, pos1.z, false);
		pos2.y = CGround::GetHeightAboveWater(pos2.x, pos2.z, false);

		buffer->SafeAppend({pos1                    , {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos2                    , {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos2 + UpVector * 100.0f, {0.2f, 0.8f, 0.2f, 0.5f}});

		buffer->SafeAppend({pos2 + UpVector * 100.0f, {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos1 + UpVector * 100.0f, {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos1                    , {0.2f, 0.8f, 0.2f, 0.5f}});


		pos1 = float3(bx, 0.0f, by + dy * (a    ));
		pos2 = float3(bx, 0.0f, by + dy * (a + 1));
		pos1.y = CGround::GetHeightAboveWater(pos1.x, pos1.z, false);
		pos2.y = CGround::GetHeightAboveWater(pos2.x, pos2.z, false);

		buffer->SafeAppend({pos1                    , {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos2                    , {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos2 + UpVector * 100.0f, {0.2f, 0.8f, 0.2f, 0.5f}});

		buffer->SafeAppend({pos2 + UpVector * 100.0f, {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos1 + UpVector * 100.0f, {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos1                    , {0.2f, 0.8f, 0.2f, 0.5f}});


		pos1 = float3(bx + dx * 10.0f, 0.0f, by + dy * (a    ));
		pos2 = float3(bx + dx * 10.0f, 0.0f, by + dy * (a + 1));
		pos1.y = CGround::GetHeightAboveWater(pos1.x, pos1.z, false);
		pos2.y = CGround::GetHeightAboveWater(pos2.x, pos2.z, false);

		buffer->SafeAppend({pos1                    , {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos2                    , {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos2 + UpVector * 100.0f, {0.2f, 0.8f, 0.2f, 0.5f}});

		buffer->SafeAppend({pos2 + UpVector * 100.0f, {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos1 + UpVector * 100.0f, {0.2f, 0.8f, 0.2f, 0.5f}});
		buffer->SafeAppend({pos1                    , {0.2f, 0.8f, 0.2f, 0.5f}});
	}

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));
	buffer->Submit(GL_TRIANGLES);
	shader->Disable();

	glAttribStatePtr->DisableDepthTest();
}
#endif


void CStartPosSelecter::Draw()
{
	if (gu->spectating) {
		delete this;
		return;
	}


	GL::RenderDataBufferC* buffer = GL::GetRenderBufferC();
	Shader::IProgramObject* shader = buffer->GetShader();

	// lua-fied!
	// DrawStartBox(buffer, shader);

	if (!showReadyBox)
		return;

	const float mx =                               float(mouse->lastx)  * globalRendering->pixelX;
	const float my = (globalRendering->viewSizeY - float(mouse->lasty)) * globalRendering->pixelY;


	glAttribStatePtr->EnableBlendMask();

	{
		gleDrawQuadC(readyBox, InBox(mx, my, readyBox)? SColor{0.7f, 0.2f, 0.2f, guiAlpha}: SColor{0.7f, 0.7f, 0.2f, guiAlpha}, buffer);

		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE);
		glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	{
		gleDrawQuadC(readyBox, InBox(mx, my, readyBox)? SColor{0.7f, 0.2f, 0.2f, guiAlpha}: SColor{0.7f, 0.7f, 0.2f, guiAlpha}, buffer);

		glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	{
		shader->Enable();
		shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
		shader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));
		buffer->Submit(GL_TRIANGLES);
		shader->Disable();
	}

	{
		// fit text into box
		const float ySize = readyBox.y2 - readyBox.y1;
		const float xSize = readyBox.x2 - readyBox.x1;

		const float yPos = 0.5f * (readyBox.y1 + readyBox.y2);
		const float xPos = 0.5f * (readyBox.x1 + readyBox.x2);

		const float unitWidth  = font->GetSize() * font->GetTextWidth("Ready") * globalRendering->pixelX;
		const float unitHeight = font->GetSize() * font->GetLineHeight(      ) * globalRendering->pixelY;

		const float fontScale = 0.9f * std::min(xSize / unitWidth, ySize / unitHeight);

		font->SetColors(); // default
		font->glPrint(xPos, yPos, fontScale, FONT_OUTLINE | FONT_CENTER | FONT_VCENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Ready");
		font->DrawBufferedGL4();
	}
}

