/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ResourceBar.h"
#include "GuiHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/Fonts/glFont.h"
#include "Sim/Misc/TeamHandler.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/SpringMath.h"

CResourceBar* resourceBar = nullptr;


CResourceBar::CResourceBar()
	: moveBox(false)
	, enabled(true)
{
	box.x1 = 0.26f;
	box.y1 = 0.97f;
	box.x2 = 1;
	box.y2 = 1;

	metalBox.x1 = 0.09f;
	metalBox.y1 = 0.01f;
	metalBox.x2 = (box.x2 - box.x1) / 2.0f - 0.03f;
	metalBox.y2 = metalBox.y1 + (box.y2 - box.y1);   // extend to the very top of the screen

	energyBox.x1 = 0.48f;
	energyBox.y1 = 0.01f;
	energyBox.x2 = box.x2 - 0.03f - box.x1;
	energyBox.y2 = energyBox.y1 + (box.y2 - box.y1); // extend to the very top of the screen
}


static std::string FloatToSmallString(float num, float mul = 1) {

	char c[50];

	if (num == 0)
		sprintf(c, "0");
	if (       math::fabs(num) < (10       * mul)) {
		sprintf(c, "%.1f",  num);
	} else if (math::fabs(num) < (10000    * mul)) {
		sprintf(c, "%.0f",  num);
	} else if (math::fabs(num) < (10000000 * mul)) {
		sprintf(c, "%.0fk", num / 1000);
	} else {
		sprintf(c, "%.0fM", num / 1000000);
	}

	return c;
}


void CResourceBar::Draw()
{
	if (!enabled)
		return;

	const CTeam* myTeam = teamHandler.Team(gu->myTeam);

	const SResourcePack& rpp  = myTeam->resPrevPull;
	const SResourcePack& rpi  = myTeam->resPrevIncome;
	const SResourcePack& rsnt = myTeam->resSent;
	const SResourcePack& rshr = myTeam->resShare;
	const SResourcePack& rcs  = myTeam->resStorage;
	const SResourcePack& rcr  = myTeam->res;

	GL::RenderDataBufferC* buffer = GL::GetRenderBufferC();
	Shader::IProgramObject* shader = buffer->GetShader();


	glAttribStatePtr->EnableBlendMask();


	const float metalx = box.x1 + 0.01f;
	const float metaly = box.y1 + 0.004f;

	const float metalbarx1 = metalx + 0.08f;
	const float metalbarx2 = box.x1 + (box.x2 - box.x1) / 2.0f - 0.03f;
	const float metalbarlen = metalbarx2 - metalbarx1;

	const float energyx = box.x1 + 0.4f;
	const float energyy = box.y1 + 0.004f;

	const float energybarx1 = energyx + 0.08f;
	const float energybarx2 = box.x2 - 0.03f;
	const float energybarlen = energybarx2 - energybarx1;

	{
		gleDrawQuadC(box, SColor{0.2f, 0.2f, 0.2f, guiAlpha}, buffer);
	}
	{
		// layout metal in box
		const float x1 = metalbarx1;
		const float x2 = metalbarx2;
		const float y1 = metaly + 0.014f;
		const float y2 = metaly + 0.020f;
		const float sx[] = {(rcs.metal != 0.0f)? ((rcr.metal / rcs.metal) * metalbarlen): 0.0f, rshr.metal * metalbarlen};

		gleDrawQuadC(TRectangle<float>{x1                 , y1         , x2                 , y2         }, SColor{0.8f, 0.8f, 0.8f, 0.2f}, buffer);
		gleDrawQuadC(TRectangle<float>{x1                 , y1         , x1 + sx[0]         , y2         }, SColor{1.0f, 1.0f, 1.0f, 1.0f}, buffer);
		gleDrawQuadC(TRectangle<float>{x1 + sx[1] - 0.003f, y1 - 0.003f, x1 + sx[1] + 0.003f, y2 + 0.003f}, SColor{0.9f, 0.2f, 0.2f, 0.7f}, buffer);
	}
	{
		// layout energy in box
		const float x1 = energybarx1;
		const float x2 = energybarx2;
		const float y1 = energyy + 0.014f;
		const float y2 = energyy + 0.020f;
		const float sx[] = {(rcs.energy != 0.0f)? ((rcr.energy / rcs.energy) * energybarlen): 0.0f, rshr.energy * energybarlen};

		gleDrawQuadC(TRectangle<float>{x1                 , y1         , x2                 , y2         }, SColor{0.8f, 0.8f, 0.2f, 0.2f}, buffer);
		gleDrawQuadC(TRectangle<float>{x1                 , y1         , x1 + sx[0]         , y2         }, SColor{1.0f, 1.0f, 0.2f, 1.0f}, buffer);
		gleDrawQuadC(TRectangle<float>{x1 + sx[1] - 0.003f, y1 - 0.003f, x1 + sx[1] + 0.003f, y2 + 0.003f}, SColor{0.9f, 0.2f, 0.2f, 0.7f}, buffer);
	}
	{
		shader->Enable();
		shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
		shader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));
		buffer->Submit(GL_TRIANGLES);
		shader->Disable();
	}

	const float headerFontSize = (box.y2 - box.y1) * 0.7f * globalRendering->viewSizeY;
	const float labelsFontSize = (box.y2 - box.y1) * 0.5f * globalRendering->viewSizeY;

	const unsigned int fontOptions = (guihandler != nullptr && guihandler->GetOutlineFonts()) ? (FONT_OUTLINE | FONT_NORM) : FONT_NORM;


	smallFont->SetTextColor(0.8f, 0.8f, 1.0f, 0.8f);
	smallFont->glPrint(metalx - 0.004f,  (box.y1 + box.y2) * 0.5f, headerFontSize, FONT_VCENTER | fontOptions | FONT_BUFFERED, "Metal");

	smallFont->SetTextColor(1.0f, 1.0f, 0.4f, 0.8f);
	smallFont->glPrint(energyx - 0.018f, (box.y1 + box.y2) * 0.5f, headerFontSize, FONT_VCENTER | fontOptions | FONT_BUFFERED, "Energy");

	smallFont->SetTextColor(1.0f, 0.3f, 0.3f, 1.0f); // Expenses
	smallFont->glFormat(metalx  + 0.044f, box.y1, labelsFontSize, FONT_DESCENDER | fontOptions | FONT_BUFFERED, "-%s(-%s)",
		FloatToSmallString(math::fabs( rpp.metal)).c_str(),
		FloatToSmallString(math::fabs(rsnt.metal)).c_str());
	smallFont->glFormat(energyx + 0.044f, box.y1, labelsFontSize, FONT_DESCENDER | fontOptions | FONT_BUFFERED, "-%s(-%s)",
		FloatToSmallString(math::fabs( rpp.energy)).c_str(),
		FloatToSmallString(math::fabs(rsnt.energy)).c_str());

	smallFont->SetTextColor(0.4f, 1.0f, 0.4f, 0.95f); // Income (sans portion received from allies)
	smallFont->glFormat(metalx  + 0.044f, box.y2 - 2.0f * globalRendering->pixelY, labelsFontSize, FONT_ASCENDER | fontOptions | FONT_BUFFERED, "+%s", FloatToSmallString(rpi.metal ).c_str());
	smallFont->glFormat(energyx + 0.044f, box.y2 - 2.0f * globalRendering->pixelY, labelsFontSize, FONT_ASCENDER | fontOptions | FONT_BUFFERED, "+%s", FloatToSmallString(rpi.energy).c_str());

	smallFont->SetTextColor(1.0f, 1.0f, 1.0f, 0.8f);
	smallFont->glPrint(energybarx2 - 0.01f              , energyy - 0.005f, labelsFontSize, fontOptions | FONT_BUFFERED, FloatToSmallString(rcs.energy));
	smallFont->glPrint(energybarx1 + energybarlen / 2.0f, energyy - 0.005f, labelsFontSize, fontOptions | FONT_BUFFERED, FloatToSmallString(rcr.energy));

	smallFont->glPrint(metalbarx2 - 0.01f             , metaly - 0.005f, labelsFontSize, fontOptions | FONT_BUFFERED, FloatToSmallString(rcs.metal));
	smallFont->glPrint(metalbarx1 + metalbarlen / 2.0f, metaly - 0.005f, labelsFontSize, fontOptions | FONT_BUFFERED, FloatToSmallString(rcr.metal));
	smallFont->SetTextColor(1.0f, 1.0f, 1.0f, 1.0f);

	smallFont->DrawBufferedGL4();
}


bool CResourceBar::IsAbove(int x, int y)
{
	if (!enabled)
		return false;

	const float mx = MouseX(x);
	const float my = MouseY(y);
	return InBox(mx, my, box);
}


std::string CResourceBar::GetTooltip(int x, int y)
{
	const float mx = MouseX(x);

	std::string resourceName;
	if (mx < (box.x1 + 0.36f)) {
		resourceName = "metal";
	} else {
		resourceName = "energy";
	}

	return "Shows your stored " + resourceName + " as well as\n"
	       "income(\xff\x40\xff\x40green\xff\xff\xff\xff) and expenditures (\xff\xff\x20\x20red\xff\xff\xff\xff).\n"
	       "Click in the bar to select your\n"
	       "AutoShareLevel.";
}


bool CResourceBar::MousePress(int x, int y, int button)
{
	if (!enabled)
		return false;

	const float mx = MouseX(x);
	const float my = MouseY(y);

	if (!InBox(mx, my, box))
		return false;

	moveBox = true;

	if (!gu->spectating) {
		if (InBox(mx, my, box + metalBox)) {
			moveBox = false;
			const float metalShare = Clamp((mx - (box.x1 + metalBox.x1)) / (metalBox.x2 - metalBox.x1), 0.f, 1.f);
			clientNet->Send(CBaseNetProtocol::Get().SendSetShare(gu->myPlayerNum, gu->myTeam, metalShare, teamHandler.Team(gu->myTeam)->resShare.energy));
		}
		if (InBox(mx, my, box + energyBox)) {
			moveBox = false;
			const float energyShare = Clamp((mx - (box.x1 + energyBox.x1)) / (energyBox.x2 - energyBox.x1), 0.f, 1.f);
			clientNet->Send(CBaseNetProtocol::Get().SendSetShare(gu->myPlayerNum, gu->myTeam, teamHandler.Team(gu->myTeam)->resShare.metal, energyShare));
		}
	}

	return true;
}


void CResourceBar::MouseMove(int x, int y, int dx, int dy, int button)
{
	if (!enabled)
		return;

	if (moveBox) {
		box.x1 += float(dx) * globalRendering->pixelX;
		box.x2 += float(dx) * globalRendering->pixelX;
		box.y1 -= float(dy) * globalRendering->pixelY;
		box.y2 -= float(dy) * globalRendering->pixelY;
		if (box.x1 < 0.0f) {
			box.x1 = 0.0f;
			box.x2 = 0.74f;
		}
		if (box.x2 > 1.0f) {
			box.x1 = 0.26f;
			box.x2 = 1.00f;
		}
		if (box.y1 < 0.0f) {
			box.y1 = 0.00f;
			box.y2 = 0.03f;
		}
		if (box.y2 > 1.0f) {
			box.y1 = 0.97f;
			box.y2 = 1.00f;
		}
	}
}
