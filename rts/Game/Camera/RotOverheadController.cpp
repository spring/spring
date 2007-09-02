#include "RotOverheadController.h"

#include "Platform/ConfigHandler.h"
#include "Game/Camera.h"
#include "LogOutput.h"
#include "Map/Ground.h"


#ifdef _WIN32
#define DEFAULT_MOUSE_SCALE 0.01f
#else
#define DEFAULT_MOUSE_SCALE 0.003f
#endif


CRotOverheadController::CRotOverheadController(int num)
	: CCameraController(num),
	oldHeight(500)
{
	mouseScale = configHandler.GetFloat("RotOverheadMouseScale", DEFAULT_MOUSE_SCALE);
	scrollSpeed = configHandler.GetInt("RotOverheadScrollSpeed",10)*0.1f;
	enabled=!!configHandler.GetInt("RotOverheadEnabled",1);
	fov = configHandler.GetFloat("RotOverheadFOV", 45.0f);
}

void CRotOverheadController::KeyMove(float3 move)
{
	move*=sqrt(move.z)*400;

	float3 flatForward=camera->forward;
	if(camera->forward.y<-0.9f)
		flatForward+=camera->up;
	flatForward.y=0;
	flatForward.Normalize();

	pos+=(flatForward*move.y+camera->right*move.x)*scrollSpeed;
}

void CRotOverheadController::MouseMove(float3 move)
{
	camera->rot.y -= mouseScale*move.x;
	camera->rot.x -= mouseScale*move.y*move.z;

	if(camera->rot.x>PI*0.4999f)
		camera->rot.x=PI*0.4999f;
	if(camera->rot.x<-PI*0.4999f)
		camera->rot.x=-PI*0.4999f;
}

void CRotOverheadController::ScreenEdgeMove(float3 move)
{
	KeyMove(move);
}

void CRotOverheadController::MouseWheelMove(float move)
{
	const float gheight = ground->GetHeight(pos.x,pos.z);
	float height = pos.y - gheight;
	height *= 1.0f + (move * mouseScale);
	pos.y = height + gheight;
}

float3 CRotOverheadController::GetPos()
{
	if(pos.x<0.01f)
		pos.x=0.01f;
	if(pos.z<0.01f)
		pos.z=0.01f;
	if(pos.x>(gs->mapx)*SQUARE_SIZE-0.01f)
		pos.x=(gs->mapx)*SQUARE_SIZE-0.01f;
	if(pos.z>(gs->mapy)*SQUARE_SIZE-0.01f)
		pos.z=(gs->mapy)*SQUARE_SIZE-0.01f;
	if(pos.y<ground->GetHeight(pos.x,pos.z)+5)
		pos.y=ground->GetHeight(pos.x,pos.z)+5;
	if(pos.y>9000)
		pos.y=9000;

	oldHeight = pos.y - ground->GetHeight(pos.x,pos.z);

	return pos;
}

float3 CRotOverheadController::GetDir()
{
	dir.x=(float)(sin(camera->rot.y)*cos(camera->rot.x));
	dir.y=(float)(sin(camera->rot.x));
	dir.z=(float)(cos(camera->rot.y)*cos(camera->rot.x));
	dir.Normalize();
	return dir;
}

void CRotOverheadController::SetPos(const float3& newPos)
{
	CCameraController::SetPos(newPos);
	pos.y = ground->GetHeight(pos.x, pos.z) + oldHeight;
}

float3 CRotOverheadController::SwitchFrom() const
{
	return pos;
}

void CRotOverheadController::SwitchTo(bool showText)
{
	if(showText)
		logOutput.Print("Switching to Rotatable overhead camera");
}

void CRotOverheadController::GetState(std::vector<float>& fv) const
{
	fv.push_back(/*  0 */ (float)num);
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

bool CRotOverheadController::SetState(const std::vector<float>& fv)
{
	if ((fv.size() != 11) || (fv[0] != (float)num)) {
		return false;
	}
	pos.x = fv[1];
	pos.y = fv[2];
	pos.z = fv[3];
	dir.x = fv[4];
	dir.y = fv[5];
	dir.z = fv[6];
	camera->rot.x = fv[7];
	camera->rot.y = fv[8];
	camera->rot.z = fv[9];
	oldHeight = fv[10];
	return true;
}

