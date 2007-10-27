#include "CameraController.h"

#include "Platform/ConfigHandler.h"

#ifdef _WIN32
#define DEFAULT_MOUSE_SCALE 0.01f
#else
#define DEFAULT_MOUSE_SCALE 0.003f
#endif


CCameraController* camCtrl = NULL;


CCameraController::CCameraController(int _num) : num(_num), pos(2000, 70, 1800)
{
	mouseScale = configHandler.GetFloat("FPSMouseScale", DEFAULT_MOUSE_SCALE);
	scrollSpeed = 1;
	fov = 45.0f;
	enabled = true;
}


CCameraController::~CCameraController(void)
{
}
