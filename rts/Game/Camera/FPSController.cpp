#include "StdAfx.h"
#include "mmgr.h"

#include "FPSController.h"

#include "ConfigHandler.h"
#include "Game/Camera.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "GlobalUnsynced.h"

using std::min;
using std::max;

CFPSController::CFPSController()
	: oldHeight(300)
{
	scrollSpeed = configHandler->Get("FPSScrollSpeed", 10) * 0.1f;
	enabled = !!configHandler->Get("FPSEnabled", 1);
	fov = configHandler->Get("FPSFOV", 45.0f);
}


void CFPSController::KeyMove(float3 move)
{
	move*=move.z*400;
	pos+=(camera->forward*move.y+camera->right*move.x)*scrollSpeed;
}


void CFPSController::MouseMove(float3 move)
{
	camera->rot.y -= mouseScale*move.x;
	camera->rot.x -= mouseScale*move.y*move.z;

	if(camera->rot.x>PI*0.4999f)
		camera->rot.x=PI*0.4999f;
	if(camera->rot.x<-PI*0.4999f)
		camera->rot.x=-PI*0.4999f;
}


void CFPSController::ScreenEdgeMove(float3 move)
{
	KeyMove(move);
}


void CFPSController::MouseWheelMove(float move)
{
	pos += camera->up * move;
}


float3 CFPSController::GetPos()
{
	if (!gu->directControl)
	{
		const float margin = 0.01f;
		const float xMin = margin;
		const float zMin = margin;
		const float xMax = (float)(gs->mapx * SQUARE_SIZE) - margin;
		const float zMax = (float)(gs->mapy * SQUARE_SIZE) - margin;

		pos.x = max(xMin, min(xMax, pos.x));
		pos.z = max(zMin, min(zMax, pos.z));

		const float gndHeight = ground->GetHeight(pos.x, pos.z);
		const float yMin = gndHeight + 5.0f;
		const float yMax = 9000.0f;
		pos.y = max(yMin, min(yMax, pos.y));
		oldHeight = pos.y - gndHeight;
	}

	return pos;
}


float3 CFPSController::GetDir()
{
	dir.x = (float)(cos(camera->rot.x) * sin(camera->rot.y));
	dir.z = (float)(cos(camera->rot.x) * cos(camera->rot.y));
	dir.y = (float)(sin(camera->rot.x));
	dir.ANormalize();
	return dir;
}


void CFPSController::SetPos(const float3& newPos)
{
	CCameraController::SetPos(newPos);

	if (!gu->directControl)
	{
		pos.y = ground->GetHeight(pos.x, pos.z) + oldHeight;
	}
}


void CFPSController::SetDir(const float3& newDir)
{
	dir = newDir;
}


float3 CFPSController::SwitchFrom() const
{
	return pos;
}


void CFPSController::SwitchTo(bool showText)
{
	if (showText) {
		logOutput.Print("Switching to FPS style camera");
  }
}


void CFPSController::GetState(StateMap& sm) const
{
	sm["px"] = pos.x;
	sm["py"] = pos.y;
	sm["pz"] = pos.z;

	sm["dx"] = dir.x;
	sm["dy"] = dir.y;
	sm["dz"] = dir.z;

	sm["rx"] = camera->rot.x;
	sm["ry"] = camera->rot.y;
	sm["rz"] = camera->rot.z;

	sm["oldHeight"] = oldHeight;
}


bool CFPSController::SetState(const StateMap& sm)
{
	SetStateFloat(sm, "px", pos.x);
	SetStateFloat(sm, "py", pos.y);
	SetStateFloat(sm, "pz", pos.z);

	SetStateFloat(sm, "dx", dir.x);
	SetStateFloat(sm, "dy", dir.y);
	SetStateFloat(sm, "dz", dir.z);

	SetStateFloat(sm, "rx", camera->rot.x);
	SetStateFloat(sm, "ry", camera->rot.y);
	SetStateFloat(sm, "rz", camera->rot.z);

	SetStateFloat(sm, "oldHeight", oldHeight);

	return true;
}

 
