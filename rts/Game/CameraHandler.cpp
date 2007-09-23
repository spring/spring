#include "CameraHandler.h"

#include "Game/Camera/CameraController.h"
#include "Game/Camera/FPSController.h"
#include "Game/Camera/OverheadController.h"
#include "Game/Camera/RotOverheadController.h"
#include "Game/Camera/FreeController.h"
#include "Game/Camera/OverviewController.h"
#include "Game/Camera/TWController.h"
#include "Game/Camera.h"
#include "Platform/ConfigHandler.h"
#include "LogOutput.h"

CCameraHandler* cam = 0;

CCameraHandler::CCameraHandler()
{
	cameraTime=0.0f;
	cameraTimeLeft=0.0f;
	preControlCamNum=0;
	
	//fps camera must always be the first one in the list
	std::vector<CCameraController*>& camCtrls = camControllers;
	camCtrls.push_back(new CFPSController         (camCtrls.size())); // 0  (first)
	camCtrls.push_back(new COverheadController    (camCtrls.size())); // 1
	camCtrls.push_back(new CTWController          (camCtrls.size())); // 2
	camCtrls.push_back(new CRotOverheadController (camCtrls.size())); // 3
	camCtrls.push_back(new CFreeController        (camCtrls.size())); // 4
	camCtrls.push_back(new COverviewController    (camCtrls.size())); // 5  (last)

	int mode = configHandler.GetInt("CamMode", 1);
	mode = std::max(0, std::min(mode, (int)camControllers.size() - 1));
	currentCamControllerNum = mode;
	currentCamController = camControllers[currentCamControllerNum];

	const double z = 0.0; // casting problems...
	cameraTimeFactor =
			std::max(z, atof(configHandler.GetString("CamTimeFactor", "1.0").c_str()));
	cameraTimeExponent =
			std::max(z, atof(configHandler.GetString("CamTimeExponent", "4.0").c_str()));
}

CCameraHandler::~CCameraHandler()
{
	while(!camControllers.empty()){
		delete camControllers.back();
		camControllers.pop_back();
	}
}

void CCameraHandler::UpdateCam()
{
	camera->up.x = 0.0f;
	camera->up.y = 1.0f;
	camera->up.z = 0.0f;

	const float  wantedCamFOV = currentCamController->GetFOV();
	const float3 wantedCamPos = currentCamController->GetPos();
	const float3 wantedCamDir = currentCamController->GetDir();

	if (cameraTimeLeft <= 0.0f) {
		camera->pos = wantedCamPos;
		camera->forward = wantedCamDir;
		camera->fov = wantedCamFOV;
	}
	else {
		const float currTime = cameraTimeLeft;
		cameraTimeLeft = std::max(0.0f, (cameraTimeLeft - gu->lastFrameTime));
		const float nextTime = cameraTimeLeft;
		const float exp = cameraTimeExponent;
		const float ratio = 1.0f - (float)pow((nextTime / currTime), exp);

		const float  deltaFOV = wantedCamFOV - camera->fov;
		const float3 deltaPos = wantedCamPos - camera->pos;
		const float3 deltaDir = wantedCamDir - camera->forward;
		camera->fov     += deltaFOV * ratio;
		camera->pos     += deltaPos * ratio;
		camera->forward += deltaDir * ratio;
		camera->forward.Normalize();
	}
}

void CCameraHandler::SetCameraMode(unsigned mode)
{
	if ((mode >= camControllers.size()) || (mode == currentCamControllerNum)) {
		return;
	}

	preControlCamNum = currentCamControllerNum;
	CameraTransition(1.0f);

	CCameraController* oldCamCtrl = currentCamController;

	currentCamControllerNum = mode;
	currentCamController = camControllers[mode];
	currentCamController->SetPos(oldCamCtrl->SwitchFrom());
	currentCamController->SwitchTo();
}

void CCameraHandler::PushMode()
{
	controllerStack.push(currentCamController->num);
}

void CCameraHandler::PopMode()
{
	SetCameraMode(controllerStack.top());
	controllerStack.pop();
}
	
void CCameraHandler::CameraTransition(float time)
{
	time = std::max(time, 0.0f) * cameraTimeFactor;
	cameraTime = time;
	cameraTimeLeft = time;
}

void CCameraHandler::ToggleState()
{
	CameraTransition(1.0f);

	CCameraController* oldCamCtrl = currentCamController;
	currentCamControllerNum++;
	if (currentCamControllerNum >= camControllers.size()) {
		currentCamControllerNum = 0;
	}

	int a = 0;
	const int maxTries = camControllers.size() - 1;
	while ((a < maxTries) && !camControllers[currentCamControllerNum]->enabled) {
		currentCamControllerNum++;
		if (currentCamControllerNum >= camControllers.size()) {
			currentCamControllerNum = 0;
		}
		a++;
	}

	currentCamController = camControllers[currentCamControllerNum];
	currentCamController->SetPos(oldCamCtrl->SwitchFrom());
	currentCamController->SwitchTo();
}

void CCameraHandler::ToggleOverviewCamera(void)
{
	const int ovCamNum = (int)camControllers.size() - 1;
	if (controllerStack.empty())
	{
		PushMode();
		SetCameraMode(ovCamNum);
	}
	else
	{
		PopMode();
	}
	CameraTransition(1.0f);
}

void CCameraHandler::SaveView(const std::string& name)
{
	if (name.empty()) {
		return;
	}
	ViewData vd;
	vd.mode = currentCamControllerNum;
	currentCamController->GetState(vd.state);
	views[name] = vd;
	logOutput.Print("Saved view: " + name);
	return;
}


bool CCameraHandler::LoadView(const std::string& name)
{
	if (name.empty()) {
		return false;
	}

	std::map<std::string, ViewData>::const_iterator it = views.find(name);
	if (it == views.end()) {
		return false;
	}
	const ViewData& saved = it->second;

	ViewData current;
	current.mode = currentCamControllerNum;
	currentCamController->GetState(current.state);

	for (it = views.begin(); it != views.end(); ++it) {
		if (it->second == current) {
			break;
		}
	}
	if (it == views.end()) {
		tmpView = current;
	}

	ViewData effective;
	if (saved == current) {
		effective = tmpView;
	} else {
		effective = saved;
	}

	return LoadViewData(effective);
}


bool CCameraHandler::LoadViewData(const ViewData& vd)
{
	if (vd.state.size() <= 0) {
		return false;
	}

	int currentMode = currentCamControllerNum;

	if ((vd.mode == -1) ||
			((vd.mode >= 0) && (vd.mode < camControllers.size()))) {
		const float3 dummy = currentCamController->SwitchFrom();
		currentCamControllerNum = vd.mode;
		currentCamController = camControllers[currentCamControllerNum];
		const bool showMode = (currentMode != vd.mode);
		currentCamController->SwitchTo(showMode);
		CameraTransition(1.0f);
	}

	return currentCamController->SetState(vd.state);
}

