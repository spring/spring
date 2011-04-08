/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL_mouse.h>

#include "FPSUnitController.h"
#include "UI/MouseHandler.h"
#include "Game/Camera.h"
#include "Game/TraceRay.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Weapons/Weapon.h"
#include "System/GlobalUnsynced.h"
#include "System/myMath.h"
#include "System/NetProtocol.h"

FPSUnitController::FPSUnitController()
	: viewDir(float3(0.0f, 0.0f, 1.0f))
	, targetPos(float3(0.0f, 0.0f, 1.0f))
	, targetDist(1000.0f)
	, forward(false)
	, back(false)
	, left(false)
	, right(false)
	, mouse1(false)
	, mouse2(false)
{
	oldPitch   = 0;
	oldHeading = 0;
	oldState   = 255;
	oldDCpos   = ZeroVector;

	targetUnit = NULL;
	controllee = NULL;
	controller = NULL;
}

void FPSUnitController::Update() {
	const int piece = controllee->script->AimFromWeapon(0);
	const float3 relPos = controllee->script->GetPiecePos(piece);
	const float3 pos = controllee->pos +
		controllee->frontdir * relPos.z +
		controllee->updir    * relPos.y +
		controllee->rightdir * relPos.x +
		UpVector             * 7.0f;

	oldDCpos = pos;

	float hitDist;
	CUnit* hitUnit;
	CFeature* hitFeature;
	{
		int origAllyTeam = gu->myAllyTeam;
		gu->myAllyTeam = teamHandler->AllyTeam(controllee->team);

		//hitDist = helper->TraceRayTeam(pos, dc->viewDir, controllee->maxRange, hit, 1, controllee, teamHandler->AllyTeam(team));
		hitDist = TraceRay::GuiTraceRay(pos, viewDir, controllee->maxRange, true, controllee, hitUnit, hitFeature);

		gu->myAllyTeam = origAllyTeam;
	}

	if (hitUnit) {
		targetUnit = hitUnit;
		targetDist = hitDist;
		targetPos  = hitUnit->pos;

		if (!mouse2) {
			controllee->AttackUnit(hitUnit, true, true);
		}
	} else {
		hitDist = std::min(hitDist, controllee->maxRange * 0.95f);

		targetUnit = NULL;
		targetDist = hitDist;
		targetPos  = pos + viewDir * targetDist;

		if (!mouse2) {
			controllee->AttackGround(targetPos, true, true);

			for (std::vector<CWeapon*>::iterator wi = controllee->weapons.begin(); wi != controllee->weapons.end(); ++wi) {
				const float d = std::min(targetDist, (*wi)->range * 0.95f);
				const float3 p = pos + viewDir * d;

				(*wi)->AttackGround(p, true);
			}
		}
	}
}



void FPSUnitController::RecvStateUpdate(const unsigned char* buf) {
	forward    = !!(buf[2] & (1 << 0));
	back       = !!(buf[2] & (1 << 1));
	left       = !!(buf[2] & (1 << 2));
	right      = !!(buf[2] & (1 << 3));
	mouse1     = !!(buf[2] & (1 << 4));

	bool newMouse2 = !!(buf[2] & (1 << 5));

	if (!mouse2 && newMouse2 && controllee != NULL) {
		controllee->AttackUnit(NULL, true);
	}

	mouse2 = newMouse2;

	const short int h = *((short int*) &buf[3]);
	const short int p = *((short int*) &buf[5]);

	viewDir = GetVectorFromHAndPExact(h, p);
}

void FPSUnitController::SendStateUpdate(const bool* camMove) {
	if (!gu->fpsMode) { return; }

	unsigned char state = 0;

	if (camMove[0]) { state |= (1 << 0); }
	if (camMove[1]) { state |= (1 << 1); }
	if (camMove[2]) { state |= (1 << 2); }
	if (camMove[3]) { state |= (1 << 3); }
	if (mouse->buttons[SDL_BUTTON_LEFT].pressed)  { state |= (1 << 4); }
	if (mouse->buttons[SDL_BUTTON_RIGHT].pressed) { state |= (1 << 5); }

	shortint2 hp = GetHAndPFromVector(camera->forward);

	if (hp.x != oldHeading || hp.y != oldPitch || state != oldState) {
		oldHeading = hp.x;
		oldPitch   = hp.y;
		oldState   = state;

		net->Send(CBaseNetProtocol::Get().SendDirectControlUpdate(gu->myPlayerNum, state, hp.x, hp.y));
	}
}
