#include "FPSController.h"

#include "Platform/ConfigHandler.h"
#include "Game/Camera.h"
#include "LogOutput.h"
#include "Map/Ground.h"

using namespace std;

CFPSController::CFPSController()
	: oldHeight(300)
{
	scrollSpeed = configHandler.GetInt("FPSScrollSpeed", 10) * 0.1f;
	enabled = !!configHandler.GetInt("FPSEnabled", 1);
	fov = configHandler.GetFloat("FPSFOV", 45.0f);
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
#ifdef DIRECT_CONTROL_ALLOWED
	if (!gu->directControl)
#endif
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
	dir.Normalize();
	return dir;
}

void CFPSController::SetPos(const float3& newPos)
{
	CCameraController::SetPos(newPos);

#ifdef DIRECT_CONTROL_ALLOWED
	if (!gu->directControl)
#endif
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
	if(showText)
		logOutput.Print("Switching to FPS style camera");
}

void CFPSController::GetState(std::vector<float>& fv) const
{
	fv.push_back(/*  1 */ pos.x);
	fv.push_back(/*  2 */ pos.y);
	fv.push_back(/*  3 */ pos.z);
	fv.push_back(/*  4 */ dir.x);
	fv.push_back(/*  5 */ dir.y);
	fv.push_back(/*  6 */ dir.z);
	fv.push_back(/*  7 */ camera->rot.x);
	fv.push_back(/*  8 */ camera->rot.y);
	fv.push_back(/*  9 */ camera->rot.z);
	fv.push_back(/* 10 */ oldHeight);
}

bool CFPSController::SetState(const std::vector<float>& fv)
{
	if (fv.size() != 10) {
		return false;
	}
	pos.x = fv[0];
	pos.y = fv[1];
	pos.z = fv[2];
	dir.x = fv[3];
	dir.y = fv[4];
	dir.z = fv[5];
	camera->rot.x = fv[6];
	camera->rot.y = fv[7];
	camera->rot.z = fv[8];
	oldHeight = fv[9];
	return true;
}

 
