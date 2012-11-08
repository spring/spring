/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/cstdint.hpp>
#include <SDL_keysym.h>

#include "TWController.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Game/UI/MouseHandler.h"
#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/Input/KeyInput.h"

CONFIG(int, TWScrollSpeed).defaultValue(10);
CONFIG(bool, TWEnabled).defaultValue(true);
CONFIG(float, TWFOV).defaultValue(45.0f);


CTWController::CTWController()
{
	scrollSpeed = configHandler->GetInt("TWScrollSpeed") * 0.1f;
	enabled = configHandler->GetBool("TWEnabled");
	fov = configHandler->GetFloat("TWFOV");
	UpdateVectors();
}


void CTWController::KeyMove(float3 move)
{
	float3 flatForward=camera->forward;
	flatForward.y=0;
	flatForward.ANormalize();

	move *= math::sqrt(move.z) * 200;
	pos  += (camera->right * move.x + flatForward * move.y) * scrollSpeed;
	UpdateVectors();
}


void CTWController::MouseMove(float3 move)
{
	move *= (1 + keyInput->GetKeyState(SDLK_LSHIFT) * 3) * pixelSize;

	float3 flatForward = camera->forward;
	flatForward.y = 0;
	flatForward.ANormalize();

	pos += (camera->right * move.x - flatForward * move.y) * scrollSpeed;
	UpdateVectors();
}


void CTWController::ScreenEdgeMove(float3 move)
{
	if(mouse->lasty<globalRendering->viewSizeY/3){
		camera->rot.y-=move.x*globalRendering->lastFrameTime*0.5f*200;
		move.x=0;
	}
	KeyMove(move);
}


void CTWController::MouseWheelMove(float move)
{
	camera->rot.x-=move*0.001f;
	UpdateVectors();
}


void CTWController::UpdateVectors()
{
	pos.x = Clamp(pos.x, 0.01f, gs->mapx * SQUARE_SIZE - 0.01f);
	pos.z = Clamp(pos.z, 0.01f, gs->mapy * SQUARE_SIZE - 0.01f);
	pos.y = ground->GetHeightAboveWater(pos.x, pos.z, false);

	camera->rot.x = Clamp(camera->rot.x, -PI * 0.4f, -0.1f);

	dir.x = math::sin(camera->rot.y) * math::cos(camera->rot.x);
	dir.y = math::sin(camera->rot.x);
	dir.z = math::cos(camera->rot.y) * math::cos(camera->rot.x);
	dir.ANormalize();
}


void CTWController::Update()
{
	pixelSize = (camera->GetTanHalfFov() * 2.0f) / globalRendering->viewSizeY * (-camera->rot.x * 1500) * 2.0f;
}


float3 CTWController::GetPos() const
{
	float dist = -camera->rot.x * 1500;

	float3 cpos = pos - dir * dist;
	if (cpos.y < ground->GetHeightAboveWater(cpos.x, cpos.z, false) + 5)
		cpos.y = ground->GetHeightAboveWater(cpos.x, cpos.z, false) + 5;

	return cpos;
}


void CTWController::SetPos(const float3& newPos)
{
	pos = newPos;
	UpdateVectors();
}


float3 CTWController::SwitchFrom() const
{
	return pos;
}


void CTWController::SwitchTo(bool showText)
{
	if (showText) {
		LOG("Switching to Total War style camera");
	}
}


void CTWController::GetState(StateMap& sm) const
{
	CCameraController::GetState(sm);
}


bool CTWController::SetState(const StateMap& sm)
{
	CCameraController::SetState(sm);
	return true;
}
