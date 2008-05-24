#include "CameraHandler.h"

#include "Game/Camera/CameraController.h"
#include "Game/Camera/FPSController.h"
#include "Game/Camera/OverheadController.h"
#include "Game/Camera/SmoothController.h"
#include "Game/Camera/RotOverheadController.h"
#include "Game/Camera/FreeController.h"
#include "Game/Camera/OverviewController.h"
#include "Game/Camera/TWController.h"
#include "Game/Camera.h"
#include "Game/Action.h"
#include "Platform/ConfigHandler.h"
#include "LogOutput.h"
#include "GlobalStuff.h"


CCameraHandler* camHandler = NULL;


CCameraHandler::CCameraHandler()
{
	cameraTime=0.0f;
	cameraTimeLeft=0.0f;
	
	//fps camera must always be the first one in the list
	std::vector<CCameraController*>& camCtrls = camControllers;
	camCtrls.push_back(new CFPSController()); // 0  (first)
	camCtrls.push_back(new COverheadController()); // 1
	camCtrls.push_back(new CTWController()); // 2
	camCtrls.push_back(new CRotOverheadController()); // 3
	camCtrls.push_back(new CFreeController()); // 4
	camCtrls.push_back(new SmoothController()); // 5
	camCtrls.push_back(new COverviewController()); // 6

	const unsigned mode = std::min(static_cast<unsigned>(configHandler.GetInt("CamMode", 1)), camControllers.size()-1);
	currCamCtrlNum = mode;
	currCamCtrl = camControllers[currCamCtrlNum];

	const double z = 0.0; // casting problems...
	cameraTimeFactor = std::max(z, atof(configHandler.GetString("CamTimeFactor", "1.0").c_str()));
	cameraTimeExponent = std::max(z, atof(configHandler.GetString("CamTimeExponent", "4.0").c_str()));

	RegisterAction("viewfps");		RegisterAction("viewta");
	RegisterAction("viewtw");		RegisterAction("viewrot");
	RegisterAction("viewfree");		RegisterAction("viewov");
	RegisterAction("viewlua");		RegisterAction("viewtaflip");
	RegisterAction("viewsave");		RegisterAction("viewload");
	RegisterAction("toggleoverview");
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

	const float  wantedCamFOV = currCamCtrl->GetFOV();
	const float3 wantedCamPos = currCamCtrl->GetPos();
	const float3 wantedCamDir = currCamCtrl->GetDir();

	if (cameraTimeLeft <= 0.0f) {
		camera->pos = wantedCamPos;
		camera->forward = wantedCamDir;
		if (wantedCamFOV != camera->GetFov())
			camera->SetFov(wantedCamFOV);
	}
	else {
		const float currTime = cameraTimeLeft;
		cameraTimeLeft = std::max(0.0f, (cameraTimeLeft - gu->lastFrameTime));
		const float nextTime = cameraTimeLeft;
		const float exp = cameraTimeExponent;
		const float ratio = 1.0f - (float)pow((nextTime / currTime), exp);

		const float  deltaFOV = wantedCamFOV - camera->GetFov();
		const float3 deltaPos = wantedCamPos - camera->pos;
		const float3 deltaDir = wantedCamDir - camera->forward;
		camera->SetFov(camera->GetFov() + (deltaFOV * ratio));
		camera->pos     += deltaPos * ratio;
		camera->forward += deltaDir * ratio;
		camera->forward.Normalize();
	}
}


void CCameraHandler::SetCameraMode(unsigned mode)
{
	if ((mode >= camControllers.size()) || (mode == static_cast<unsigned>(currCamCtrlNum))) {
		return;
	}

	CameraTransition(1.0f);

	CCameraController* oldCamCtrl = currCamCtrl;

	currCamCtrlNum = mode;
	currCamCtrl = camControllers[mode];
	currCamCtrl->SetPos(oldCamCtrl->SwitchFrom());
	currCamCtrl->SwitchTo();
}


void CCameraHandler::PushMode()
{
	controllerStack.push(GetCurrentControllerNum());
}


void CCameraHandler::PopMode()
{
	if (controllerStack.size() > 0) {
		SetCameraMode(controllerStack.top());
		controllerStack.pop();
	}
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

	CCameraController* oldCamCtrl = currCamCtrl;
	currCamCtrlNum++;
	if (currCamCtrlNum >= camControllers.size()) {
		currCamCtrlNum = 0;
	}

	int a = 0;
	const int maxTries = camControllers.size() - 1;
	while ((a < maxTries) && !camControllers[currCamCtrlNum]->enabled) {
		currCamCtrlNum++;
		if (currCamCtrlNum >= camControllers.size()) {
			currCamCtrlNum = 0;
		}
		a++;
	}

	currCamCtrl = camControllers[currCamCtrlNum];
	currCamCtrl->SetPos(oldCamCtrl->SwitchFrom());
	currCamCtrl->SwitchTo();
}


void CCameraHandler::ToggleOverviewCamera()
{
	const unsigned ovCamNum = camControllers.size() - 1;
	if (controllerStack.empty()) {
		PushMode();
		SetCameraMode(ovCamNum);
	}
	else {
		PopMode();
	}
	CameraTransition(1.0f);
}


void CCameraHandler::SaveView(const std::string& name)
{
	if (name.empty())
		return;
	ViewData vd;
	vd.push_back(currCamCtrlNum);
	currCamCtrl->GetState(vd);
	views[name] = vd;
	return;
}


bool CCameraHandler::LoadView(const std::string& name)
{
	if (name.empty())
		return false;

	std::map<std::string, ViewData>::const_iterator it = views.find(name);
	if (it == views.end()) {
		return false;
	}
	const ViewData& saved = it->second;

	ViewData current;
	GetState(current);
	
	if (saved == current) // load a view twice to return to old settings
	{
		 if (name != "__old_view") // safety: should not happen, but who knows?
			 return LoadView("__old_view");
		 else
			 return false;
	}
	else
	{
		if (name != "__old_view")
			SaveView("__old_view");
		return LoadViewData(saved);
	}
}

void CCameraHandler::GetState(std::vector<float>& fv) const
{
	fv.clear();
	fv.push_back(static_cast<float>(currCamCtrlNum));
	currCamCtrl->GetState(fv);
}

bool CCameraHandler::SetState(const std::vector<float>& fv)
{
	if (fv.empty())
		return false;
	
	const unsigned savedController = static_cast<unsigned>(fv[0]);
	if (savedController >= camControllers.size())
		return false;
	if (savedController != currCamCtrlNum)
	{
		currCamCtrlNum = savedController;
		currCamCtrl = camControllers[savedController];
		currCamCtrl->SwitchTo();
	}
	return currCamCtrl->SetState(fv, 1);
}

const std::string CCameraHandler::GetCurrentControllerName() const
{
	return currCamCtrl->GetName();
}

void CCameraHandler::PushAction(const Action& action)
{
	const std::string cmd = action.command;
	if (cmd == "viewfps") {
		SetCameraMode(0);
	}
	else if (cmd == "viewta") {
		SetCameraMode(1);
	}
	else if (cmd == "viewtw") {
		SetCameraMode(2);
	}
	else if (cmd == "viewrot") {
		SetCameraMode(3);
	}
	else if (cmd == "viewfree") {
		SetCameraMode(4);
	}
	else if (cmd == "viewov") {
		SetCameraMode(5);
	}
	else if (cmd == "viewlua") {
		SetCameraMode(6);
	}
	else if (cmd == "viewtaflip") {
		COverheadController* taCam =
				dynamic_cast<COverheadController*>(camControllers[1]);
		SmoothController* smCam =
				dynamic_cast<SmoothController*>(camControllers[5]);
		if (taCam) {
			if (!action.extra.empty()) {
				taCam->flipped = !!atoi(action.extra.c_str());
			} else {
				taCam->flipped = !taCam->flipped;
			}
		}
		if (smCam) {
			if (!action.extra.empty()) {
				smCam->flipped = !!atoi(action.extra.c_str());
			} else {
				smCam->flipped = !smCam->flipped;
			}
		}
	}
	else if (cmd == "viewsave") {
		if (!action.extra.empty())
		{
			SaveView(action.extra);
			logOutput.Print("Saved view: " + action.extra);
		}
	}
	else if (cmd == "viewload") {
		if (!LoadView(action.extra))
			logOutput.Print("Loading view failed!");
	}
	else if (cmd == "toggleoverview") {
		ToggleOverviewCamera();
	}
}

bool CCameraHandler::LoadViewData(const ViewData& vd)
{
	if (vd.empty())
		return false;

	const unsigned currentMode = currCamCtrlNum;
	const unsigned wantedMode = static_cast<unsigned>(vd[0]);

	if ((wantedMode >= 0) && (wantedMode < camControllers.size())) {
		if (wantedMode != currentMode)
		{
			currCamCtrl->SwitchFrom();
			currCamCtrlNum = wantedMode;
			currCamCtrl = camControllers[currCamCtrlNum];
			const bool showMode = (currentMode != wantedMode);
			currCamCtrl->SwitchTo(showMode);
			CameraTransition(1.0f);
		}
		return currCamCtrl->SetState(vd, 1);
	}
	else
		return false;
}

