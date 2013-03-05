/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/cstdint.hpp>

#include <SDL_mouse.h>
#include <SDL_keysym.h>

#include "OrbitController.h"
#include "Game/Camera.h"
#include "Game/UI/MouseHandler.h"
#include "Map/Ground.h"
#include "System/Log/ILog.h"
#include "System/Config/ConfigHandler.h"
#include "System/Input/KeyInput.h"

CONFIG(bool, OrbitControllerEnabled).defaultValue(true);
CONFIG(float, OrbitControllerOrbitSpeed).defaultValue(0.25f).minimumValue(0.1f).maximumValue(10.0f);
CONFIG(float, OrbitControllerPanSpeed).defaultValue(2.00f).minimumValue(0.1f).maximumValue(10.0f);
CONFIG(float, OrbitControllerZoomSpeed).defaultValue(5.00f).minimumValue(0.1f).maximumValue(10.0f);

#define DEG2RAD(a) ((a) * (3.141592653f / 180.0f))
#define RAD2DEG(a) ((a) * (180.0f / 3.141592653f))
static const float3 YVEC(0.0f, 1.0f, 0.0f);

COrbitController::COrbitController():
	lastMouseMoveX(0), lastMouseMoveY(0),
	lastMousePressX(0), lastMousePressY(0),
	lastMouseButton(-1),
	currentState(Orbiting),
	distance(512.0f), cDistance(512.0f),
	rotation(0.0f), cRotation(0.0f),
	elevation(0.0f), cElevation(0.0f)
{
	enabled = configHandler->GetBool("OrbitControllerEnabled");

	orbitSpeedFact = configHandler->GetFloat("OrbitControllerOrbitSpeed");
	panSpeedFact   = configHandler->GetFloat("OrbitControllerPanSpeed");
	zoomSpeedFact  = configHandler->GetFloat("OrbitControllerZoomSpeed");
}

void COrbitController::Init(const float3& p, const float3& tar)
{
	CCamera* cam = camera;

	const float l = (tar == ZeroVector)?
		std::max(ground->LineGroundCol(p, p + cam->forward * 1024.0f, false), 512.0f):
		p.distance(tar);

	const float3 t = (tar == ZeroVector)? (p + cam->forward * l): tar;
	const float3 v = (t - p);
	const float3 w = (v / v.Length()); // do not normalize v in-place

	const float d = v.Length();
	const float e = RAD2DEG(math::acos(v.Length2D() / d));
	const float r = RAD2DEG(math::acos(w.x));

	distance  = cDistance = d;
	elevation = cElevation = e;
	rotation  = cRotation = (v.z > 0.0f)? 180.0f + r: 180.0f - r;
	cen       = t;
}



void COrbitController::Update()
{
	if (!keyInput->IsKeyPressed(SDLK_LMETA)) {
		return;
	}

	const int x = mouse->lastx;
	const int y = mouse->lasty;

	const int pdx = lastMousePressX - x;
	const int pdy = lastMousePressY - y;
	const int rdx = lastMouseMoveX - x;
	const int rdy = lastMouseMoveY - y;

	lastMouseMoveX = x;
	lastMouseMoveY = y;

	MyMouseMove(pdx, pdy, rdx, rdy, lastMouseButton);
}



void COrbitController::MousePress(int x, int y, int button)
{
	lastMousePressX = x;
	lastMousePressY = y;
	lastMouseButton = button;
	cDistance = distance;
	cRotation = rotation;
	cElevation = elevation;

	switch (button) {
		case SDL_BUTTON_LEFT: { currentState = Orbiting; } break;
		case SDL_BUTTON_MIDDLE: { currentState = Panning; } break;
		case SDL_BUTTON_RIGHT: { currentState = Zooming; } break;
	}
}

void COrbitController::MouseRelease(int x, int y, int button)
{
	lastMousePressX = x;
	lastMousePressY = y;
	lastMouseButton = -1;
	currentState = None;
}


void COrbitController::MouseMove(float3 move)
{
	// only triggers on SDL_BUTTON_MIDDLE (see CMouseHandler::MouseMove())
}

void COrbitController::MyMouseMove(int dx, int dy, int rdx, int rdy, int button)
{
	switch (button) {
		case SDL_BUTTON_LEFT: {
			rotation = cRotation - (dx * orbitSpeedFact);
			elevation = cElevation - (dy * orbitSpeedFact);
		} break;

		case SDL_BUTTON_RIGHT: {
			distance = cDistance - (dy * zoomSpeedFact);
		} break;
	}

	if (elevation >  89.0f) elevation =  89.0f;
	if (elevation < -89.0f) elevation = -89.0f;
	if (distance  <   1.0f) distance  =   1.0f;

	switch (button) {
		case SDL_BUTTON_LEFT: {
			Orbit();
		} break;

		case SDL_BUTTON_MIDDLE: {
			Pan(rdx, rdy);
		} break;

		case SDL_BUTTON_RIGHT: {
			Zoom();
		} break;
	}
}

float3 COrbitController::GetPos() const
{
	return camera->pos;
}

float3 COrbitController::GetDir() const
{
	float3 dir = cen - camera->pos;
	dir.ANormalize();
	return dir;
}

void COrbitController::Orbit()
{
	CCamera* cam = camera;

	cam->pos = cen + GetOrbitPos();
	cam->pos.y = std::max(cam->pos.y, ground->GetHeightReal(cam->pos.x, cam->pos.z, false));
	cam->forward = (cen - cam->pos).ANormalize();
	cam->up = YVEC;
}

void COrbitController::Pan(int rdx, int rdy)
{
	CCamera* cam = camera;

	// horizontal pan
	cam->pos += (cam->right * -rdx * panSpeedFact);
	cen += (cam->right * -rdx * panSpeedFact);

	// vertical pan
	cam->pos += (cam->up * rdy * panSpeedFact);
	cen += (cam->up * rdy * panSpeedFact);


	// don't allow orbit center or ourselves to drop below the terrain
	const float camGH = ground->GetHeightReal(cam->pos.x, cam->pos.z, false);
	const float cenGH = ground->GetHeightReal(cen.x, cen.z, false);

	if (cam->pos.y < camGH) {
		cam->pos.y = camGH;
	}

	if (cen.y < cenGH) {
		cen.y = cenGH;
		cam->forward = (cen - cam->pos).ANormalize();

		Init(cam->pos, cen);
	}
}

void COrbitController::Zoom()
{
	camera->pos = cen - (camera->forward * distance);
}



void COrbitController::KeyMove(float3 move)
{
	// higher framerate means we take smaller steps per frame
	// (x and y are lastFrameTime in secs., 200FPS ==> 0.005s)
	Pan(int(move.x * -1000), int(move.y * 1000));
}

void COrbitController::ScreenEdgeMove(float3 move)
{
	Pan(int(move.x * -1000), int(move.y * 1000));
}

void COrbitController::MouseWheelMove(float move)
{
}

void COrbitController::SetPos(const float3& newPos)
{
	if (keyInput->IsKeyPressed(SDLK_LMETA)) {
		return;
	}

	// support minimap position hopping
	const float dx = newPos.x - camera->pos.x;
	const float dz = newPos.z - camera->pos.z;

	cen.x += dx;
	cen.z += dz;
	cen.y = ground->GetHeightReal(cen.x, cen.z, false);

	camera->pos.x += dx;
	camera->pos.z += dz;

	Init(camera->pos, cen);
}

float3 COrbitController::GetOrbitPos() const
{
	const float beta = DEG2RAD(elevation);
	const float gamma = DEG2RAD(rotation);

	float cx = distance;
	float cy = 0.0f;
	float cz = 0.0f;
	float tx;

	tx = cx;
	cx = cx * math::cos(beta) + cy * math::sin(beta);
	cy = tx * math::sin(beta) + cy * math::cos(beta);

	tx = cx;
	cx = cx * math::cos(gamma) - cz * math::sin(gamma);
	cz = tx * math::sin(gamma) + cz * math::cos(gamma);

	return float3(cx, cy, cz);
}



float3 COrbitController::SwitchFrom() const
{
	return camera->pos;
}

void COrbitController::SwitchTo(bool showText)
{
	if (showText) {
		LOG("Switching to Orbit style camera");
	}

	Init(camera->pos, ZeroVector);
}



void COrbitController::GetState(StateMap& sm) const
{
	CCameraController::GetState(sm);

	sm["tx"] = cen.x;
	sm["ty"] = cen.y;
	sm["tz"] = cen.z;
}

bool COrbitController::SetState(const StateMap& sm)
{
	CCameraController::SetState(sm);

	SetStateFloat(sm, "tx", cen.x);
	SetStateFloat(sm, "ty", cen.y);
	SetStateFloat(sm, "tz", cen.z);
	return true;
}
