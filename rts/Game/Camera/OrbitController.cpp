#include "StdAfx.h"

#include <SDL_mouse.h>
#include <SDL_keysym.h>

#include "OrbitController.h"
#include "Game/Camera.h"
#include "Game/UI/MouseHandler.h"
#include "Map/Ground.h"
#include "System/LogOutput.h"
#include "Platform/ConfigHandler.h"

extern Uint8* keys;

#define DEG2RAD(a) ((a) * (3.141592653f / 180.0f))
#define RAD2DEG(a) ((a) * (180.0f / 3.141592653f))
static const float3 YVEC(0.0f, 1.0f, 0.0f);

COrbitController::COrbitController():
	lastMousePressX(0), lastMousePressY(0),
	lastMouseMoveX(0), lastMouseMoveY(0),
	lastMouseButton(-1),
	currentState(Orbiting),
	distance(512.0f), cDistance(512.0f),
	rotation(0.0f), cRotation(0.0f),
	elevation(0.0f), cElevation(0.0f)
{
	enabled = !!configHandler.GetInt("OrbitControllerEnabled", 1);
}

void COrbitController::Init(const float3& p, const float3& tar)
{
	CCamera* cam = camera;

	const float l = (tar == ZeroVector)?
		std::max(ground->LineGroundCol(p, p + cam->forward * 1024.0f), 512.0f):
		(p - tar).Length();

	const float3 t = (tar == ZeroVector)? (p + cam->forward * l): tar;
	const float3 v = (t - p);
	const float3 w = (v / v.Length()); // do not normalize v in-place

	const float d = v.Length();
	const float e = RAD2DEG(acosf(v.Length2D() / d));
	const float r = RAD2DEG(acosf(w.x));

	distance  = cDistance = d;
	elevation = cElevation = e;
	rotation  = cRotation = (v.z > 0.0f)? 180.0f + r: 180.0f - r;
	cen       = t;
}



void COrbitController::Update()
{
	if (!keys[SDLK_LMETA]) {
		return;
	}

	// can't use mouse->last{x, y}, since they
	// have already been updated to the current
	// pos at this point
	int x = 0;
	int y = 0;
	int s = SDL_GetMouseState(&x, &y);

	const int pdx = lastMousePressX - x;
	const int pdy = lastMousePressY - y;
	const int rdx = lastMouseMoveX - x;
	const int rdy = lastMouseMoveY - y;

	lastMouseMoveX = x;
	lastMouseMoveY = y;

	MyMouseMove(pdx, pdy, rdx, rdy, lastMouseButton);
}

void COrbitController::KeyMove(float3 move)
{
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
	// only triggers on SLD_BUTTON_MIDDLE (see CMouseHandler::MouseMove())
}

void COrbitController::MyMouseMove(int dx, int dy, int rdx, int rdy, int button)
{
	CCamera* cam = camera;

	switch (button) {
		case SDL_BUTTON_LEFT: {
			rotation = cRotation - (dx * 0.25f);
			elevation = cElevation - (dy * 0.25f);
		} break;

		case SDL_BUTTON_RIGHT: {
			distance = cDistance - (dy * 0.5f * 10.0f);
		} break;
	}

	if (elevation >  89.0f) elevation =  89.0f;
	if (elevation < -89.0f) elevation = -89.0f;
	if (distance  <   1.0f) distance  =   1.0f;

	switch (button) {
		case SDL_BUTTON_LEFT: {
			cam->pos = cen + GetOrbitPos();
			cam->pos.y = std::max(cam->pos.y, ground->GetHeight2(cam->pos.x, cam->pos.z));
			cam->forward = (cen - cam->pos).Normalize();
			cam->up = YVEC;
		} break;

		case SDL_BUTTON_RIGHT: {
			cam->pos = cen - (cam->forward * distance);
		} break;

		case SDL_BUTTON_MIDDLE: {
			// horizontal pan
			cam->pos += (cam->right * -rdx * 2);
			cen += (cam->right * -rdx * 2);

			// vertical pan
			cam->pos += (cam->up * rdy * 2);
			cen += (cam->up * rdy * 2);


			// don't allow orbit center or ourselves to drop below the terrain
			const float camGH = ground->GetHeight2(cam->pos.x, cam->pos.z);
			const float cenGH = ground->GetHeight2(cen.x, cen.z);

			if (cam->pos.y < camGH) {
				cam->pos.y = camGH;
			}

			if (cen.y < cenGH) {
				cen.y = cenGH;
				cam->forward = (cen - cam->pos).Normalize();

				Init(cam->pos, cen);
			}
		} break;
	}
}

void COrbitController::ScreenEdgeMove(float3 move)
{
}

void COrbitController::MouseWheelMove(float move)
{
}



float3 COrbitController::GetPos()
{
	return camera->pos;
}

void COrbitController::SetPos(const float3& newPos)
{
	if (keys[SDLK_LMETA]) {
		return;
	}

	CCamera* cam = camera;

	// support minimap position hopping
	cen = newPos;
	cen.y = ground->GetHeight2(cen.x, cen.z);
	cam->pos = cen - (cam->forward * (cam->pos - cen).Length());

	Init(cam->pos, cen);
}

float3 COrbitController::GetDir()
{
	return (cen - camera->pos).Normalize();
}

float3 COrbitController::GetOrbitPos() const
{
	const float beta = DEG2RAD(elevation);
	const float gamma = DEG2RAD(rotation);

	float cx = distance;
	float cy = 0.0f;
	float cz = 0.0f;
	float tx = cx;

	tx = cx;
	cx = cx * cosf(beta) + cy * sinf(beta);
	cy = tx * sinf(beta) + cy * cosf(beta);

	tx = cx;
	cx = cx * cosf(gamma) - cz * sinf(gamma);
	cz = tx * sinf(gamma) + cz * cosf(gamma);

	return float3(cx, cy, cz);
}



float3 COrbitController::SwitchFrom() const
{
	return camera->pos;
}

void COrbitController::SwitchTo(bool showText)
{
	if (showText) {
		logOutput.Print("Switching to Orbit style camera");
	}

	Init(camera->pos, ZeroVector);
}



void COrbitController::GetState(StateMap& sm) const
{
	sm["px"] = camera->pos.x;
	sm["py"] = camera->pos.y;
	sm["pz"] = camera->pos.z;

	sm["tx"] = cen.x;
	sm["ty"] = cen.y;
	sm["tz"] = cen.z;
}

bool COrbitController::SetState(const StateMap& sm)
{
	SetStateFloat(sm, "px", camera->pos.x);
	SetStateFloat(sm, "py", camera->pos.y);
	SetStateFloat(sm, "pz", camera->pos.z);

	SetStateFloat(sm, "tx", cen.x);
	SetStateFloat(sm, "ty", cen.y);
	SetStateFloat(sm, "tz", cen.z);
	return true;
}
