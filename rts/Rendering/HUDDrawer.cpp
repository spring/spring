/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "HUDDrawer.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/MatrixState.hpp"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDef.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/SpringMath.h"

#include <cmath>

HUDDrawer* HUDDrawer::GetInstance()
{
	static HUDDrawer hud;
	return &hud;
}

void HUDDrawer::DrawModel(const CUnit* unit)
{
	CMatrix44f projMat;
	CMatrix44f viewMat;

	projMat.Translate(-0.8f, -0.4f, 0.0f);
	projMat = projMat * camera->GetProjectionMatrix();

	viewMat.Translate(0.0f, 0.0f, -unit->radius);
	viewMat.Scale({1.0f / unit->radius, 1.0f / unit->radius, 1.0f / unit->radius});

	if (unit->moveType->UseHeading()) {
		viewMat.RotateX(  90.0f * math::DEG_TO_RAD);
		viewMat.RotateZ(-180.0f * math::DEG_TO_RAD);
	} else {
		const float3& cx = camera->GetRight();
		const float3& cy = camera->GetUp();
		const float3& cz = camera->GetDir();

		viewMat = viewMat * CMatrix44f(ZeroVector, {cx.x, cy.x, cz.x}, {cx.y, cy.y, cz.y}, {cx.z, cy.z, cz.z});
	}

	// TODO: DrawIndividualLuaTrans
	unitDrawer->PushIndividualOpaqueState(unit, false);

	IUnitDrawerState* state = unitDrawer->GetDrawerState(DRAWER_STATE_SEL);
	Shader::IProgramObject* shader = state->GetActiveShader();
	LocalModel* model = const_cast<LocalModel*>(&unit->localModel);

	shader->SetUniformMatrix4fv(8, false, viewMat);
	shader->SetUniformMatrix4fv(9, false, projMat);

	model->UpdatePieceMatrices(gs->frameNum);
	// state->SetTeamColor(unit->team, {0.25f, 1.0f});
	state->SetMatrices(CMatrix44f::Identity(), model->GetPieceMatrices());
	model->Draw();

	unitDrawer->PopIndividualOpaqueState(unit, false);
}


void HUDDrawer::DrawUnitDirectionArrow(const CUnit* unit)
{
	CMatrix44f viewMat;

	viewMat.Translate(-0.8f, -0.4f, 0.0f);
	viewMat.Scale({0.33f, 0.33f * globalRendering->aspectRatio, 0.33f});
	viewMat.RotateZ(-((unit->heading / 32768.0f) * 180.0f + 180.0f) * math::DEG_TO_RAD);
	// broken
	// viewMat.Rotate(((unit->heading / 32768.0f) * 180.0f + 180.0f) * math::DEG_TO_RAD, FwdVector);

	const SColor arrowColor = SColor(0.3f, 0.9f, 0.3f, 0.4f);

	GL::RenderDataBufferC* rdbc = GL::GetRenderBufferC();
	Shader::IProgramObject* prog = rdbc->GetShader();

	prog->Enable();
	prog->SetUniformMatrix4x4<float>("u_movi_mat", false, viewMat);
	prog->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::Identity());

	rdbc->SafeAppend({{-0.2f, -0.3f, 0.0f}, arrowColor});
	rdbc->SafeAppend({{-0.2f,  0.3f, 0.0f}, arrowColor});
	rdbc->SafeAppend({{ 0.0f,  0.4f, 0.0f}, arrowColor});
	rdbc->SafeAppend({{ 0.2f,  0.3f, 0.0f}, arrowColor});
	rdbc->SafeAppend({{ 0.2f, -0.3f, 0.0f}, arrowColor});
	rdbc->SafeAppend({{-0.2f, -0.3f, 0.0f}, arrowColor});
	rdbc->Submit(GL_TRIANGLE_FAN);
	// keep enabled for DrawCameraDirectionArrow
	// prog->Disable();
}

void HUDDrawer::DrawCameraDirectionArrow(const CUnit* unit)
{
	const float3 viewDir = camera->GetDir();
	const SColor arrowColor = SColor(0.4f, 0.4f, 1.0f, 0.6f);

	CMatrix44f viewMat;
	viewMat.Translate(-0.8f, -0.4f, 0.0f);
	viewMat.Scale({0.33f, 0.33f * globalRendering->aspectRatio, 0.33f});
	viewMat.RotateZ(-((GetHeadingFromVector(viewDir.x, viewDir.z) / 32768.0f) * 180.0f + 180.0f) * math::DEG_TO_RAD);
	viewMat.Scale({0.4f, 0.4f, 0.3f});

	GL::RenderDataBufferC* rdbc = GL::GetRenderBufferC();
	Shader::IProgramObject* prog = rdbc->GetShader();

	// prog->Enable();
	prog->SetUniformMatrix4x4<float>("u_movi_mat", false, viewMat);
	rdbc->SafeAppend({{-0.2f, -0.3f, 0.0f}, arrowColor});
	rdbc->SafeAppend({{-0.2f,  0.3f, 0.0f}, arrowColor});
	rdbc->SafeAppend({{ 0.0f,  0.5f, 0.0f}, arrowColor});
	rdbc->SafeAppend({{ 0.2f,  0.3f, 0.0f}, arrowColor});
	rdbc->SafeAppend({{ 0.2f, -0.3f, 0.0f}, arrowColor});
	rdbc->SafeAppend({{-0.2f, -0.3f, 0.0f}, arrowColor});
	rdbc->Submit(GL_TRIANGLE_FAN);
	prog->Disable();
}


void HUDDrawer::DrawWeaponStates(const CUnit* unit)
{
	// note: font p.m. is not identity, convert xy-coors from [-1,1] to [0,1]
	font->SetTextColor(0.2f, 0.8f, 0.2f, 0.8f);
	font->glFormat(-0.9f * 0.5f + 0.5f, 0.35f * 0.5f + 0.5f, 1.0f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Health: %.0f / %.0f", (float) unit->health, (float) unit->maxHealth);

	if (playerHandler.Player(gu->myPlayerNum)->fpsController.mouse2)
		font->glPrint(-0.9f * 0.5f + 0.5f, 0.30f * 0.5f + 0.5f, 1.0f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Free-Fire Mode");

	int numWeaponsToPrint = 0;

	for (const CWeapon* w: unit->weapons) {
		numWeaponsToPrint += (!w->weaponDef->isShield);
	}

	if (numWeaponsToPrint == 0) {
		font->DrawBufferedGL4();
		return;
	}

	// we have limited space to draw whole list of weapons
	const float yMax = 0.25f;
	const float yMin = 0.00f;
	const float maxLineHeight = 0.045f;
	const float lineHeight = std::min((yMax - yMin) / numWeaponsToPrint, maxLineHeight);
	const float fontSize = 1.2f * (lineHeight / maxLineHeight);
	float yPos = yMax;

	for (const CWeapon* w: unit->weapons) {
		const WeaponDef* wd = w->weaponDef;

		if (wd->isShield)
			continue;

		yPos -= lineHeight;

		if (wd->stockpile && !w->numStockpiled) {
			if (w->numStockpileQued > 0) {
				font->SetTextColor(0.8f, 0.2f, 0.2f, 0.8f);
				font->glFormat(-0.9f * 0.5f + 0.5f, yPos * 0.5f + 0.5f, fontSize, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "%s: Stockpiling (%i%%)", wd->description.c_str(), std::roundf(100.0f * w->buildPercent));
			} else {
				font->SetTextColor(0.8f, 0.2f, 0.2f, 0.8f);
				font->glFormat(-0.9f * 0.5f + 0.5f, yPos * 0.5f + 0.5f, fontSize, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "%s: No ammo", wd->description.c_str());
			}

			continue;
		}
		if (w->reloadStatus > gs->frameNum) {
			font->SetTextColor(0.8f, 0.2f, 0.2f, 0.8f);
			font->glFormat(-0.9f * 0.5f + 0.5f, yPos * 0.5f + 0.5f, fontSize, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "%s: Reloading (%i%%)", wd->description.c_str(), 100 - std::roundf(100.0f * (w->reloadStatus - gs->frameNum) / int(w->reloadTime / unit->reloadSpeed)));
			continue;
		}
		if (!w->angleGood) {
			font->SetTextColor(0.6f, 0.6f, 0.2f, 0.8f);
			font->glFormat(-0.9f * 0.5f + 0.5f, yPos * 0.5f + 0.5f, fontSize, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "%s: Aiming", wd->description.c_str());
			continue;
		}

		font->SetTextColor(0.2f, 0.8f, 0.2f, 0.8f);
		font->glFormat(-0.9f * 0.5f + 0.5f, yPos * 0.5f + 0.5f, fontSize, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "%s: Ready", wd->description.c_str());
	}

	font->DrawBufferedGL4();
}

void HUDDrawer::DrawTargetReticle(const CUnit* unit)
{
	glAttribStatePtr->DisableDepthTest();

	// draw the reticle in world coordinates
	GL::RenderDataBufferC* rdbc = GL::GetRenderBufferC();
	Shader::IProgramObject* prog = rdbc->GetShader();

	prog->Enable();
	prog->SetUniformMatrix4x4<float>("u_movi_mat", false, camera->GetViewMatrix());
	prog->SetUniformMatrix4x4<float>("u_proj_mat", false, camera->GetProjectionMatrix());



	const SColor colors[] = {SColor(0.0f, 1.0f, 0.0f, 0.7f), SColor(1.0f, 0.0f, 0.0f, 0.7f), SColor(0.0f, 0.0f, 1.0f, 0.7f)};

	for (unsigned int i = 0; i < unit->weapons.size(); ++i) {
		const CWeapon* w = unit->weapons[i];
		const SColor& c = colors[std::min(i, 2u)];

		if (w == nullptr)
			continue;

		if (w->HaveTarget()) {
			float3 pos = w->GetCurrentTargetPos();
			float3 zdir = (pos - camera->GetPos()).ANormalize();
			float3 xdir = (zdir.cross(UpVector)).ANormalize();
			float3 ydir = (xdir.cross(zdir)).Normalize();
			float radius = 10.0f;

			if (w->GetCurrentTarget().type == Target_Unit)
				radius = w->GetCurrentTarget().unit->radius;

			// draw the reticle
			for (int b = 0; b <= 80; ++b) {
				const float  a = b * math::TWOPI / 80.0f;
				const float sa = fastmath::sin(a);
				const float ca = fastmath::cos(a);

				rdbc->SafeAppend({pos + (xdir * sa + ydir * ca) * radius, c});
			}

			rdbc->Submit(GL_LINE_STRIP);

			if (!w->onlyForward) {
				const CPlayer* player = w->owner->fpsControlPlayer;
				const FPSUnitController& controller = player->fpsController;
				const float dist = std::min(controller.targetDist, w->range * 0.9f);

				pos = w->aimFromPos + w->wantedDir * dist;
				zdir = (pos - camera->GetPos()).ANormalize();
				xdir = (zdir.cross(UpVector)).ANormalize();
				ydir = (xdir.cross(zdir)).ANormalize();
				radius = dist / 100.0f;

				for (int b = 0; b <= 80; ++b) {
					const float  a = b * math::TWOPI / 80.0f;
					const float sa = fastmath::sin(a);
					const float ca = fastmath::cos(a);

					rdbc->SafeAppend({pos + (xdir * sa + ydir * ca) * radius, c});
				}

				rdbc->Submit(GL_LINE_STRIP);
			}


			if (!w->onlyForward) {
				rdbc->SafeAppend({pos, c});
				rdbc->SafeAppend({w->GetCurrentTargetPos(), c});

				rdbc->SafeAppend({pos + (xdir * fastmath::sin(math::PI * 0.25f) + ydir * fastmath::cos(math::PI * 0.25f)) * radius, c});
				rdbc->SafeAppend({pos + (xdir * fastmath::sin(math::PI * 1.25f) + ydir * fastmath::cos(math::PI * 1.25f)) * radius, c});

				rdbc->SafeAppend({pos + (xdir * fastmath::sin(math::PI * -0.25f) + ydir * fastmath::cos(math::PI * -0.25f)) * radius, c});
				rdbc->SafeAppend({pos + (xdir * fastmath::sin(math::PI * -1.25f) + ydir * fastmath::cos(math::PI * -1.25f)) * radius, c});
			}

			if ((w->GetCurrentTargetPos() - camera->GetPos()).ANormalize().dot(camera->GetDir()) < 0.7f) {
				rdbc->SafeAppend({w->GetCurrentTargetPos(), c});
				rdbc->SafeAppend({camera->GetPos() + camera->GetDir() * 100.0f, c});
			}

			rdbc->Submit(GL_LINES);
		}
	}

	prog->Disable();
}

void HUDDrawer::Draw(const CUnit* unit)
{
	if (unit == nullptr || !draw)
		return;

	glAttribStatePtr->PushBits(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT);
	glAttribStatePtr->DisableDepthTest();
	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (unit->moveType->UseHeading()) {
		DrawUnitDirectionArrow(unit);
		DrawCameraDirectionArrow(unit);
	}

	glAttribStatePtr->EnableDepthTest();

	DrawModel(unit);
	DrawWeaponStates(unit);
	DrawTargetReticle(unit);

	glAttribStatePtr->PopBits();
}
