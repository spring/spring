#include "SmoothController.h"

#include <SDL_keysym.h>
#include <SDL_types.h>
#include <SDL_timer.h>

#include "Platform/ConfigHandler.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/UI/MouseHandler.h"
#include "LogOutput.h"
#include "Map/Ground.h"



extern Uint8 *keys;

SmoothController::SmoothController(int num)
	: CCameraController(num),
	height(500),zscale(0.5f),
	oldAltHeight(500),
	maxHeight(10000),
	changeAltHeight(true),
	flipped(false),
	speedFactor(1)
{
	scrollSpeed = configHandler.GetInt("SmoothScrollSpeed",10)*0.1f;
	tiltSpeed = configHandler.GetFloat("SmoothTiltSpeed",1.0f);
	enabled = !!configHandler.GetInt("SmoothEnabled",1);
	fov = configHandler.GetFloat("SmoothFOV", 45.0f);
	lastSource = Noone;
}


SmoothController::~SmoothController()
{
}

void SmoothController::KeyMove(float3 move)
{
	if (flipped) {
		move.x = -move.x;
		move.y = -move.y;
	}
	move*=sqrt(move.z)*200;
	float pixelsize= camera->GetTanHalfFov()*2/gu->viewSizeY*height*2;
	const float3 thisMove(move.x*pixelsize*2*scrollSpeed, 0, -move.y*pixelsize*2*scrollSpeed);
	static unsigned lastKeyMove = SDL_GetTicks();
	const unsigned timeDiff = SDL_GetTicks() - lastKeyMove;
	lastKeyMove = SDL_GetTicks();
	if (thisMove.x != 0 || thisMove.z != 0)
	{
		// user want to move with keys
		lastSource = Key;
		Move(thisMove, timeDiff);
	}
	else if (lastSource == Key)
	{
		// last move order was given by keys, so call Move() to break
		Move(thisMove, timeDiff);
	}
}

void SmoothController::MouseMove(float3 move)
{
	if (flipped) {
		move.x = -move.x;
		move.y = -move.y;
	}
	float pixelsize=100*mouseScale* camera->GetTanHalfFov() *2/gu->viewSizeY*height*2;
	pos.x+=move.x*pixelsize*(1+keys[SDLK_LSHIFT]*3)*scrollSpeed;
	pos.z+=move.y*pixelsize*(1+keys[SDLK_LSHIFT]*3)*scrollSpeed;
}

void SmoothController::ScreenEdgeMove(float3 move)
{
	if (flipped) {
		move.x = -move.x;
		move.y = -move.y;
	}
	move*=sqrt(move.z)*200;
	float pixelsize= camera->GetTanHalfFov()*2/gu->viewSizeY*height*2;
	const float3 thisMove(move.x*pixelsize*2*scrollSpeed, 0, -move.y*pixelsize*2*scrollSpeed);
	static unsigned lastScreenMove = SDL_GetTicks();
	const unsigned timeDiff = SDL_GetTicks() - lastScreenMove;
	lastScreenMove = SDL_GetTicks();
	if (thisMove.x != 0 || thisMove.z != 0)
	{
		// user want to move with mouse on screen edge
		lastSource = ScreenEdge;
		Move(thisMove, timeDiff);
	}
	else if (lastSource == ScreenEdge)
	{
		// last move order was given by screen edge, so call Move() to break
		Move(thisMove, timeDiff);
	}
}

void SmoothController::MouseWheelMove(float move)
{
	// tilt the camera if LCTRL is pressed
	if (keys[SDLK_LCTRL]) {
		zscale *= (1.0f + (move * tiltSpeed * mouseScale * (keys[SDLK_LSHIFT] ? 3.0f : 1.0f)));
		if (zscale < 0.05f) zscale = 0.05f;
		if (zscale > 10) zscale = 10;
	} else { // holding down LALT uses 'instant-zoom' from here to the end of the function
		// ZOOM IN to mouse cursor instead of mid screen
		if (move < 0) {
			float3 cpos=pos-dir*height;
			float dif=-height * move * mouseScale*0.7f * (keys[SDLK_LSHIFT] ? 3:1);
			if ((height - dif) <60.0f) {
				dif = height - 60.0f;
			}
			if (keys[SDLK_LALT]) { // instant-zoom: zoom in to standard view
				dif = (height - oldAltHeight) / mouse->dir.y * dir.y;
			}
			float3 wantedPos = cpos + mouse->dir * dif;
			float newHeight = ground->LineGroundCol(wantedPos, wantedPos + dir * 15000);
			if (newHeight < 0) {
				newHeight = height* (1.0f + move * mouseScale * 0.7f * (keys[SDLK_LSHIFT] ? 3:1));
			}
			if ((wantedPos.y + (dir.y * newHeight)) < 0) {
				newHeight = -wantedPos.y / dir.y;
			}
			if (newHeight < maxHeight) {
				height = newHeight;
				pos = wantedPos + dir * height;
			}
		// ZOOM OUT from mid screen
		} else {
			if (keys[SDLK_LALT]) { // instant-zoom: zoom out to the max
				if(height<maxHeight*0.5f && changeAltHeight){
					oldAltHeight=height;
					changeAltHeight=false;
				}
				height=maxHeight;
				pos.x=gs->mapx*4;
				pos.z=gs->mapy*4.8f; // somewhat longer toward bottom
			} else {
				height*=1+move * mouseScale*0.7f * (keys[SDLK_LSHIFT] ? 3:1);
			}
		}
		// instant-zoom: turn on the smooth transition and reset the camera tilt
		if (keys[SDLK_LALT]) {
			zscale = 0.5f;
			camHandler->CameraTransition(1.0f);
		} else {
			changeAltHeight = true;
		}
	}
}

float3 SmoothController::GetPos()
{
	maxHeight=9.5f*max(gs->mapx,gs->mapy);		//map not created when constructor run

	if (pos.x < 0.01f) { pos.x = 0.01f; }
	if (pos.z < 0.01f) { pos.z = 0.01f; }
	if (pos.x > ((gs->mapx * SQUARE_SIZE) - 0.01f)) {
		pos.x = ((gs->mapx * SQUARE_SIZE) - 0.01f);
	}
	if (pos.z > ((gs->mapy * SQUARE_SIZE) - 0.01f)) {
		pos.z = ((gs->mapy * SQUARE_SIZE) - 0.01f);
	}
	if (height < 60.0f) {
		height = 60.0f;
	}
	if (height > maxHeight) {
		height = maxHeight;
	}

	pos.y = ground->GetHeight(pos.x,pos.z);
	dir = float3(0.0f, -1.0f, flipped ? zscale : -zscale).Normalize();

	float3 cpos = pos - dir * height;

	return cpos;
}

float3 SmoothController::GetDir()
{
	return dir;
}

float3 SmoothController::SwitchFrom() const
{
	return pos;
}

void SmoothController::SwitchTo(bool showText)
{
	if(showText)
		logOutput.Print("Switching to smooth camera");
}

void SmoothController::GetState(std::vector<float>& fv) const
{
	fv.push_back(/* 0 */ (float)num);
	fv.push_back(/* 1 */ pos.x);
	fv.push_back(/* 2 */ pos.y);
	fv.push_back(/* 3 */ pos.z);
	fv.push_back(/* 4 */ dir.x);
	fv.push_back(/* 5 */ dir.y);
	fv.push_back(/* 6 */ dir.z);
	fv.push_back(/* 7 */ height);
	fv.push_back(/* 8 */ zscale);
	fv.push_back(/* 9 */ flipped ? +1.0f : -1.0f);
}

bool SmoothController::SetState(const std::vector<float>& fv)
{
	if ((fv.size() != 10) || (fv[0] != (float)num)) {
		return false;
	}
	pos.x   =  fv[1];
	pos.y   =  fv[2];
	pos.z   =  fv[3];
	dir.x   =  fv[4];
	dir.y   =  fv[5];
	dir.z   =  fv[6];
	height  =  fv[7];
	zscale  =  fv[8];
	flipped = (fv[9] > 0.0f);
	return true;
}

void SmoothController::Move(const float3& move, const unsigned timeDiff)
{
	if ((move.x != 0 || move.z != 0) && speedFactor < maxSpeedFactor)
	{
		// accelerate
		speedFactor += timeDiff;
		if (speedFactor > maxSpeedFactor)
			speedFactor = maxSpeedFactor; // don't know why, but std::min produces undefined references here
		lastMove = move;
		pos += move*static_cast<float>(speedFactor)/static_cast<float>(maxSpeedFactor);
	}
	else if ((move.x == 0 || move.z == 0) && speedFactor > 1)
	{
		// break
		if (timeDiff > speedFactor)
			speedFactor = 0;
		else
			speedFactor -= timeDiff;
		pos += lastMove*static_cast<float>(speedFactor)/static_cast<float>(maxSpeedFactor);
	}
	else
	{
		// move or hold position (move = {0,0,0})
		pos += move;
		lastMove = move;
	}
}
