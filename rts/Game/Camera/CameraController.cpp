#include "CameraController.h"

#include "Platform/ConfigHandler.h"


CCameraController::CCameraController() : pos(2000, 70, 1800)
{
	mouseScale = configHandler.GetFloat("FPSMouseScale", 0.01f);
	scrollSpeed = 1;
	fov = 45.0f;
	enabled = true;
}


CCameraController::~CCameraController(void)
{
}
