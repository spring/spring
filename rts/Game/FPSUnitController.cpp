/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FPSUnitController.h"

#include "UI/MouseHandler.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Game/TraceRay.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Weapons/Weapon.h"
#include "System/myMath.h"
#include "Net/Protocol/NetProtocol.h"

#include <SDL_mouse.h>


FPSUnitController::FPSUnitController()
	: targetUnit(NULL)
	, controllee(NULL)
	, controller(NULL)
	, viewDir(FwdVector)
	, targetPos(FwdVector)
	, targetDist(1000.0f)
	, forward(false)
	, back(false)
	, left(false)
	, right(false)
	, mouse1(false)
	, mouse2(false)
	, oldHeading(0)
	, oldPitch(0)
	, oldState(255)
	, oldDCpos(ZeroVector)
{
}



void FPSUnitController::Update() {
	const int piece = controllee->script->AimFromWeapon(0);
	const float3 relPos = controllee->script->GetPiecePos(piece);
	const float3 pos = controllee->GetObjectSpacePos(relPos) + (UpVector * 7.0f);

	oldDCpos = pos;

	CUnit* hitUnit;
	CFeature* hitFeature;

	// SYNCED, do NOT use GuiTraceRay which also checks gu->spectatingFullView
	float hitDist = TraceRay::TraceRay(pos, viewDir, controllee->maxRange, Collision::NOCLOAKED, controllee, hitUnit, hitFeature);

	if (hitUnit != NULL) {
		targetUnit = hitUnit;
		targetDist = hitDist;
		targetPos  = hitUnit->pos;

		if (!mouse2) {
			controllee->AttackUnit(hitUnit, true, true, true);
		}
	} else {
		hitDist = std::min(hitDist, controllee->maxRange * 0.95f);

		targetUnit = NULL;
		targetDist = hitDist;
		targetPos  = pos + viewDir * targetDist;

		if (!mouse2) {
			// if a target position is in range, but not on the ground,
			// projectiles can gain extra flighttime and travel further
			//
			// NOTE: CWeapon::AttackGround checks range via TryTarget
			if ((targetPos.y - CGround::GetHeightReal(targetPos.x, targetPos.z)) <= SQUARE_SIZE) {
				controllee->AttackGround(targetPos, true, true, true);
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

	const bool newMouse2 = !!(buf[2] & (1 << 5));

	if (!mouse2 && newMouse2 && controllee != NULL) {
		controllee->AttackUnit(NULL, true, true, true);
	}

	mouse2 = newMouse2;

	const short int h = *((short int*) &buf[3]);
	const short int p = *((short int*) &buf[5]);

	viewDir = GetVectorFromHAndPExact(h, p);
}

void FPSUnitController::SendStateUpdate() {
	if (!gu->fpsMode)
		return;

	const bool* camMoveState = camera->GetMovState();
	const CMouseHandler::ButtonPressEvt* mouseButtons = mouse->buttons;

	unsigned char state = 0;
	state |= ((camMoveState[CCamera::MOVE_STATE_FWD]) * (1 << 0));
	state |= ((camMoveState[CCamera::MOVE_STATE_BCK]) * (1 << 1));
	state |= ((camMoveState[CCamera::MOVE_STATE_LFT]) * (1 << 2));
	state |= ((camMoveState[CCamera::MOVE_STATE_RGT]) * (1 << 3));
	state |= ((mouseButtons[SDL_BUTTON_LEFT ].pressed) * (1 << 4));
	state |= ((mouseButtons[SDL_BUTTON_RIGHT].pressed) * (1 << 5));

	shortint2 hp = GetHAndPFromVector(camera->GetDir());

	if (hp.x != oldHeading || hp.y != oldPitch || state != oldState) {
		oldHeading = hp.x;
		oldPitch   = hp.y;
		oldState   = state;

		clientNet->Send(CBaseNetProtocol::Get().SendDirectControlUpdate(gu->myPlayerNum, state, hp.x, hp.y));
	}
}
