/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/cstdint.hpp>
#include <SDL_keysym.h>

#include "FreeController.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Input/KeyInput.h"

using std::max;
using std::min;

CONFIG(bool, CamFreeEnabled).defaultValue(false);
CONFIG(bool, CamFreeInvertAlt).defaultValue(false);
CONFIG(bool, CamFreeGoForward).defaultValue(false);
CONFIG(float, CamFreeFOV).defaultValue(45.0f);
CONFIG(float, CamFreeScrollSpeed).defaultValue(500.0f);
CONFIG(float, CamFreeGravity).defaultValue(-500.0f);
CONFIG(float, CamFreeSlide).defaultValue(0.5f);
CONFIG(float, CamFreeGroundOffset).defaultValue(16.0f);
CONFIG(float, CamFreeTiltSpeed).defaultValue(150.0f);
CONFIG(float, CamFreeAutoTilt).defaultValue(150.0f);
CONFIG(float, CamFreeVelTime).defaultValue(1.5f);
CONFIG(float, CamFreeAngVelTime).defaultValue(1.0f);

/******************************************************************************/
/******************************************************************************/
//
//  TODO: - separate speed variable for tracking state
//        - smooth transitions between tracking states and units
//        - improve controls
//        - rename it?  ;-)
//

CFreeController::CFreeController()
: vel(0.0f, 0.0f, 0.0f),
  avel(0.0f, 0.0f, 0.0f),
  prevVel(0.0f, 0.0f, 0.0f),
  prevAvel(0.0f, 0.0f, 0.0f),
  tracking(false),
  trackPos(0.0f, 0.0f, 0.0f),
  trackRadius(0.0f),
  gndLock(false)
{
	dir = float3(0.0f, -2.0f, -1.0f);
	dir.ANormalize();
	if (camera) {
		const float hDist = math::sqrt((dir.x * dir.x) + (dir.z * dir.z));
		camera->rot.y = math::atan2(dir.x, dir.z);
		camera->rot.x = math::atan2(dir.y, hDist);
	}
	pos -= (dir * 1000.0f);

	enabled     = configHandler->GetBool("CamFreeEnabled");
	invertAlt   = configHandler->GetBool("CamFreeInvertAlt");
	goForward   = configHandler->GetBool("CamFreeGoForward");
	fov         = configHandler->GetFloat("CamFreeFOV");
	scrollSpeed = configHandler->GetFloat("CamFreeScrollSpeed");
	gravity     = configHandler->GetFloat("CamFreeGravity");
	slide       = configHandler->GetFloat("CamFreeSlide");
	gndOffset   = configHandler->GetFloat("CamFreeGroundOffset");
	tiltSpeed   = configHandler->GetFloat("CamFreeTiltSpeed");
	tiltSpeed   = tiltSpeed * (PI / 180.0);
	autoTilt    = configHandler->GetFloat("CamFreeAutoTilt");
	autoTilt    = autoTilt * (PI / 180.0);
	velTime     = configHandler->GetFloat("CamFreeVelTime");
	velTime     = max(0.1f, velTime);
	avelTime    = configHandler->GetFloat("CamFreeAngVelTime");
	avelTime    = max(0.1f, avelTime);
}


void CFreeController::SetTrackingInfo(const float3& target, float radius)
{
	tracking = true;
	trackPos = target;
	trackRadius = radius;;

	// lock the view direction to the target
	const float3 diff(trackPos - pos);
	const float rads = math::atan2(diff.x, diff.z);
	camera->rot.y = rads;

	const float len2D = diff.Length2D();
	if (math::fabs(len2D) <= 0.001f) {
		camera->rot.x = 0.0f;
	} else {
		camera->rot.x = math::atan2((trackPos.y - pos.y), len2D);
	}

	camera->UpdateForward();
}


void CFreeController::Update()
{
	if (!globalRendering->active) {
		vel  = ZeroVector;
		avel = ZeroVector;
		prevVel  = vel;
		prevAvel = avel;
		return;
	}

	// safeties
	velTime  = max(0.1f,  velTime);
	avelTime = max(0.1f, avelTime);

	// save some old state
	const float ctrlVelY = vel.y;
	const float3 prevPos = pos;

	// setup the time fractions
	const float ft = globalRendering->lastFrameTime;
	const float nt = (ft / velTime); // next time factor
	const float pt = (1.0f - nt);    // prev time factor
	const float ant = (ft / avelTime); // next time factor
	const float apt = (1.0f - ant);    // prev time factor

	// adjustment to match the ground slope
	float autoTiltVel = 0.0f;
	if (gndLock && (autoTilt > 0.0f)) {
		const float gndHeight = ground->GetHeightReal(pos.x, pos.z, false);
		if (pos.y < (gndHeight + gndOffset + 1.0f)) {
			float3 hDir;
			hDir.y = 0.0f;
			hDir.x = (float)math::sin(camera->rot.y);
			hDir.z = (float)math::cos(camera->rot.y);
			const float3 gndNormal = ground->GetSmoothNormal(pos.x, pos.z, false);
			const float dot = gndNormal.dot(hDir);
			const float gndRotX = (float)math::acos(dot) - (PI * 0.5f);
			const float rotXdiff = (gndRotX - camera->rot.x);
			autoTiltVel = (autoTilt * rotXdiff);
		}
	}

	// convert control velocity into position velocity
	if (!tracking) {
		if (goForward) {
			const float3 tmpVel((camera->forward * vel.x) +
			                    (UpVector        * vel.y) +
			                    (camera->right   * vel.z));
			vel = tmpVel;
		}
		else {
			float3 forwardNoY(camera->forward.x, 0.0f, camera->forward.z);
			forwardNoY.ANormalize();
			const float3 tmpVel((forwardNoY    * vel.x) +
			                    (UpVector      * vel.y) +
			                    (camera->right * vel.z));
			vel = tmpVel;
		}
	}

	// smooth the velocities
	vel  =  (vel * nt)  +  (prevVel * pt);
	avel = (avel * ant) + (prevAvel * apt);

	// no smoothing for gravity (still isn't right)
	if (gndLock) {
		const float dGrav = (gravity * ft);
		vel.y += dGrav;
		if (slide > 0.0f) {
			const float gndHeight = ground->GetHeightReal(pos.x, pos.z, false);
			if (pos.y < (gndHeight + gndOffset + 1.0f)) {
				const float3 gndNormal = ground->GetSmoothNormal(pos.x, pos.z, false);
				const float dotVal = gndNormal.y;
				const float scale = (dotVal * slide * -dGrav);
				vel.x += (gndNormal.x * scale);
				vel.z += (gndNormal.z * scale);
			}
		}
	}

	// set the new position/rotation
	if (!tracking) {
		pos           += (vel         * ft);
		camera->rot   += (avel        * ft);
		camera->rot.x += (autoTiltVel * ft); // note that this is not smoothed
	}
	else {
		// speed along the tracking direction varies with distance
		const float3 diff = (pos - trackPos);
		if (goForward) {
			const float dist = max(0.1f, diff.Length());
			const float nomDist = 512.0f;
			float speedScale = (dist / nomDist);
			speedScale = max(0.25f, min(16.0f, speedScale));
			const float delta = -vel.x * (ft * speedScale);
			const float newDist = max(trackRadius, (dist + delta));
			const float scale = (newDist / dist);
			pos = trackPos + (diff * scale);
			pos.y += (vel.y * ft);
		}
		else {
			const float dist = max(0.1f, diff.Length2D());
			const float nomDist = 512.0f;
			float speedScale = (dist / nomDist);
			speedScale = max(0.25f, min(16.0f, speedScale));
			const float delta = -vel.x * (ft * speedScale);
			const float newDist = max(trackRadius, (dist + delta));
			const float scale = (newDist / dist);
			pos.x = trackPos.x + (scale * diff.x);
			pos.z = trackPos.z + (scale * diff.z);
			pos.y += (vel.y * ft);
		}

		// convert the angular velocity into its positional change
		const float3 diff2 = (pos - trackPos);
		const float deltaRad = (avel.y * ft);
		const float cos_val = math::cos(deltaRad);
		const float sin_val = math::sin(deltaRad);
		pos.x = trackPos.x + ((cos_val * diff2.x) + (sin_val * diff2.z));
		pos.z = trackPos.z + ((cos_val * diff2.z) - (sin_val * diff2.x));
	}

	// setup ground lock
	const float gndHeight = ground->GetHeightReal(pos.x, pos.z, false);

	if (keyInput->IsKeyPressed(SDLK_LSHIFT)) {
		if (ctrlVelY > 0.0f) {
			gndLock = false;
		} else if ((gndOffset > 0.0f) && (ctrlVelY < 0.0f) &&
		           (pos.y < (gndHeight + gndOffset))) {
			gndLock = true;
		}
	}

	// positional clamps
	if (gndOffset < 0.0f) {
		pos.y = (gndHeight - gndOffset);
		vel.y = 0.0f;
	}
	else if (gndLock && (gravity >= 0.0f)) {
		pos.y = (gndHeight + gndOffset);
		vel.y = 0.0f;
	}
	else if (gndOffset > 0.0f) {
		const float minHeight = (gndHeight + gndOffset);
		if (pos.y < minHeight) {
			pos.y = minHeight;
			if (gndLock) {
				vel.y = min(math::fabs(scrollSpeed), ((minHeight - prevPos.y) / ft));
			} else {
				vel.y = 0.0f;
			}
		}
	}

	// angular clamps
	const float xRotLimit = (PI * 0.4999f);
	if (camera->rot.x > xRotLimit) {
		camera->rot.x = xRotLimit;
		avel.x = 0.0f;
	}
	else if (camera->rot.x < -xRotLimit) {
		camera->rot.x = -xRotLimit;
		avel.x = 0.0f;
	}
	camera->rot.y = math::fmod(camera->rot.y, PI * 2.0f);

	// setup for the next loop
	prevVel  = vel;
	prevAvel = avel;
	vel  = ZeroVector;
	avel = ZeroVector;

	tracking = false;
}

float3 CFreeController::GetDir() const
{
	float3 dir;
	dir.x = (float)(math::sin(camera->rot.y) * math::cos(camera->rot.x));
	dir.z = (float)(math::cos(camera->rot.y) * math::cos(camera->rot.x));
	dir.y = (float)(math::sin(camera->rot.x));
	dir.ANormalize();
	return dir;
}


void CFreeController::KeyMove(float3 move)
{
	const float qy = (move.y == 0.0f) ? 0.0f : (move.y > 0.0f ? 1.0f : -1.0f);
	const float qx = (move.x == 0.0f) ? 0.0f : (move.x > 0.0f ? 1.0f : -1.0f);

	const float speed  = (keyInput->IsKeyPressed(SDLK_LMETA))? 4.0f * scrollSpeed : scrollSpeed;
	const float aspeed = (keyInput->IsKeyPressed(SDLK_LMETA))? 2.0f * tiltSpeed   : tiltSpeed;

	if (keyInput->IsKeyPressed(SDLK_LCTRL)) {
		avel.x += (aspeed * -qy); // tilt
	}
	else if (keyInput->IsKeyPressed(SDLK_LSHIFT)) {
		vel.y += (speed * -qy); // up/down
	}
	else {
		vel.x += (speed * qy); // forwards/backwards
	}

	if (tracking) {
		avel.y += (aspeed * qx); // turntable rotation
	}
	else if (!keyInput->GetKeyState(SDLK_LALT) == invertAlt) {
		vel.z += (speed * qx); // left/right
	}
	else {
		avel.y += (aspeed * -qx); // spin
	}

	return;
}


void CFreeController::MouseMove(float3 move)
{
	const boost::uint8_t prevAlt   = keyInput->GetKeyState(SDLK_LALT);
	const boost::uint8_t prevCtrl  = keyInput->GetKeyState(SDLK_LCTRL);
	const boost::uint8_t prevShift = keyInput->GetKeyState(SDLK_LSHIFT);

	keyInput->SetKeyState(SDLK_LCTRL, !prevCtrl);
	keyInput->SetKeyState(SDLK_LALT, (invertAlt == !prevAlt));

	KeyMove(move);

	keyInput->SetKeyState(SDLK_LALT, prevAlt);
	keyInput->SetKeyState(SDLK_LCTRL, prevCtrl);
	keyInput->SetKeyState(SDLK_LSHIFT, prevShift);
}


void CFreeController::ScreenEdgeMove(float3 move)
{
	const boost::uint8_t prevAlt   = keyInput->GetKeyState(SDLK_LALT);
	const boost::uint8_t prevCtrl  = keyInput->GetKeyState(SDLK_LCTRL);
	const boost::uint8_t prevShift = keyInput->GetKeyState(SDLK_LSHIFT);

	keyInput->SetKeyState(SDLK_LALT, (invertAlt == !prevAlt));
	KeyMove(move);

	keyInput->SetKeyState(SDLK_LALT, prevAlt);
	keyInput->SetKeyState(SDLK_LCTRL, prevCtrl);
	keyInput->SetKeyState(SDLK_LSHIFT, prevShift);
}


void CFreeController::MouseWheelMove(float move)
{
	const boost::uint8_t prevCtrl  = keyInput->GetKeyState(SDLK_LCTRL);
	const boost::uint8_t prevShift = keyInput->GetKeyState(SDLK_LSHIFT);

	keyInput->SetKeyState(SDLK_LCTRL, 0);
	keyInput->SetKeyState(SDLK_LSHIFT, 1);

	KeyMove(float3(0.0f, move, 0.0f));

	keyInput->SetKeyState(SDLK_LCTRL, prevCtrl);
	keyInput->SetKeyState(SDLK_LSHIFT, prevShift);
}


void CFreeController::SetPos(const float3& newPos)
{
	const float h = ground->GetHeightReal(newPos.x, newPos.z, false);
	const float3 target = float3(newPos.x, h, newPos.z);
//	const float3 target = newPos;
	const float yDiff = pos.y - target.y;
	if ((yDiff * dir.y) >= 0.0f) {
		pos = float3(newPos.x, h, newPos.z);
	} else {
		pos = target - (dir * math::fabs(yDiff / dir.y));
	} // FIXME
/*
	const float oldPosY = pos.y;
	CCameraController::SetPos(newPos);
	pos.y = oldPosY;
	if (gndOffset != 0.0f) {
		const float h = ground->GetHeightReal(pos.x, pos.z, false);
		const float absH = h + math::fabsf(gndOffset);
		if (pos.y < absH) {
			pos.y = absH;
		}
	}
*/
	prevVel  = ZeroVector;
	prevAvel = ZeroVector;
}


float3 CFreeController::SwitchFrom() const
{
	const float x = max(0.1f, min(float3::maxxpos - 0.1f, pos.x));
	const float z = max(0.1f, min(float3::maxzpos - 0.1f, pos.z));
	return float3(x, ground->GetHeightAboveWater(x, z, false) + 5.0f, z);
}


void CFreeController::SwitchTo(bool showText)
{
	if (showText) {
		LOG("Switching to Free style camera");
	}
	prevVel  = ZeroVector;
	prevAvel = ZeroVector;
}


void CFreeController::GetState(StateMap& sm) const
{
	CCameraController::GetState(sm);

	sm["gndOffset"]   = gndOffset;
	sm["gravity"]     = gravity;
	sm["slide"]       = slide;
	sm["scrollSpeed"] = scrollSpeed;
	sm["tiltSpeed"]   = tiltSpeed;
	sm["velTime"]     = velTime;
	sm["avelTime"]    = avelTime;
	sm["autoTilt"]    = autoTilt;

	sm["goForward"]   = goForward ? +1.0f : -1.0f;
	sm["invertAlt"]   = invertAlt ? +1.0f : -1.0f;
	sm["gndLock"]     = gndLock   ? +1.0f : -1.0f;

	sm["rx"] = camera->rot.x;
	sm["ry"] = camera->rot.y;
	sm["rz"] = camera->rot.z;

	sm["vx"] = prevVel.x;
	sm["vy"] = prevVel.y;
	sm["vz"] = prevVel.z;

	sm["avx"] = prevAvel.x;
	sm["avy"] = prevAvel.y;
	sm["avz"] = prevAvel.z;
}


bool CFreeController::SetState(const StateMap& sm)
{
	CCameraController::SetState(sm);

	SetStateFloat(sm, "fov",         fov);
	SetStateFloat(sm, "gndOffset",   gndOffset);
	SetStateFloat(sm, "gravity",     gravity);
	SetStateFloat(sm, "slide",       slide);
	SetStateFloat(sm, "scrollSpeed", scrollSpeed);
	SetStateFloat(sm, "tiltSpeed",   tiltSpeed);
	SetStateFloat(sm, "velTime",     velTime);
	SetStateFloat(sm, "avelTime",    avelTime);
	SetStateFloat(sm, "autoTilt",    autoTilt);

	SetStateBool (sm, "goForward",   goForward);
	SetStateBool (sm, "invertAlt",   invertAlt);
	SetStateBool (sm, "gndLock",     gndLock);

	SetStateFloat(sm, "rx", camera->rot.x);
	SetStateFloat(sm, "ry", camera->rot.y);
	SetStateFloat(sm, "rz", camera->rot.z);

	SetStateFloat(sm, "vx", prevVel.x);
	SetStateFloat(sm, "vy", prevVel.y);
	SetStateFloat(sm, "vz", prevVel.z);

	SetStateFloat(sm, "avx", prevAvel.x);
	SetStateFloat(sm, "avy", prevAvel.y);
	SetStateFloat(sm, "avz", prevAvel.z);

	return true;
}
