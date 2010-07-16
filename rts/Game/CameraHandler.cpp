/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <cstdlib>

#include "mmgr.h"

#include "CameraHandler.h"

#include "Action.h"
#include "Camera.h"
#include "Camera/CameraController.h"
#include "Camera/FPSController.h"
#include "Camera/OverheadController.h"
#include "Camera/SmoothController.h"
#include "Camera/RotOverheadController.h"
#include "Camera/FreeController.h"
#include "Camera/OverviewController.h"
#include "Camera/TWController.h"
#include "Camera/OrbitController.h"
#include "ConfigHandler.h"
#include "LogOutput.h"
#include "GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"


CCameraHandler* camHandler = NULL;


CCameraHandler::CCameraHandler()
{
	cameraTime = 0.0f;
	cameraTimeLeft = 0.0f;

	// FPS camera must always be the first one in the list
	std::vector<CCameraController*>& camCtrls = camControllers;
	camCtrls.push_back(new CFPSController());         // 0
	camCtrls.push_back(new COverheadController());    // 1
	camCtrls.push_back(new CTWController());          // 2
	camCtrls.push_back(new CRotOverheadController()); // 3
	camCtrls.push_back(new CFreeController());        // 4
	camCtrls.push_back(new SmoothController());       // 5
	camCtrls.push_back(new COrbitController());       // 6
	camCtrls.push_back(new COverviewController());    // 7, needs to be last (ToggleOverviewCamera())

	for (unsigned int i = 0; i < camCtrls.size(); i++) {
		nameMap[camCtrls[i]->GetName()] = i;
	}

	int modeIndex;
	const std::string modeName = configHandler->GetString("CamModeName", "");
	if (!modeName.empty()) {
		modeIndex = GetModeIndex(modeName);
	} else {
		modeIndex = configHandler->Get("CamMode", 5);
	}
	const unsigned int mode =
		(unsigned int)std::max(0, std::min(modeIndex, (int)camCtrls.size() - 1));
	currCamCtrlNum = mode;
	currCamCtrl = camControllers[currCamCtrlNum];

	const double z = 0.0; // casting problems...
	cameraTimeFactor   = std::max(z, atof(configHandler->GetString("CamTimeFactor",   "1.0").c_str()));
	cameraTimeExponent = std::max(z, atof(configHandler->GetString("CamTimeExponent", "4.0").c_str()));

	RegisterAction("viewfps");
	RegisterAction("viewta");
	RegisterAction("viewtw");
	RegisterAction("viewrot");
	RegisterAction("viewfree");
	RegisterAction("viewov");
	RegisterAction("viewlua");
	RegisterAction("vieworbit");

	RegisterAction("viewtaflip");

	RegisterAction("toggleoverview");
	RegisterAction("togglecammode");

	RegisterAction("viewsave");
	RegisterAction("viewload");
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
		cameraTimeLeft = std::max(0.0f, (cameraTimeLeft - globalRendering->lastFrameTime));
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


void CCameraHandler::SetCameraMode(unsigned int mode)
{
	if ((mode >= camControllers.size()) || (mode == static_cast<unsigned int>(currCamCtrlNum))) {
		return;
	}

	CameraTransition(1.0f);

	CCameraController* oldCamCtrl = currCamCtrl;

	currCamCtrlNum = mode;
	currCamCtrl = camControllers[mode];
	currCamCtrl->SetPos(oldCamCtrl->SwitchFrom());
	currCamCtrl->SwitchTo();
}


void CCameraHandler::SetCameraMode(const std::string& modeName)
{
	const int modeNum = GetModeIndex(modeName);
	if (modeNum >= 0) {
		SetCameraMode(modeNum);
	}
	// do nothing if the name is not matched
}


int CCameraHandler::GetModeIndex(const std::string& name) const
{
	std::map<std::string, unsigned int>::const_iterator it = nameMap.find(name);
	if (it != nameMap.end()) {
		return it->second;
	}
	return -1;
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
	const unsigned int ovCamNum = camControllers.size() - 1;
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
	vd["mode"] = currCamCtrlNum;
	currCamCtrl->GetState(vd);
	views[name] = vd;
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
	GetState(current);

	if (saved == current) { // load a view twice to return to old settings
		 if (name != "__old_view") { // safety: should not happen, but who knows?
			 return LoadView("__old_view");
		 } else {
			 return false;
			}
	}
	else {
		if (name != "__old_view") {
			SaveView("__old_view");
		}
		return LoadViewData(saved);
	}
}


void CCameraHandler::GetState(CCameraController::StateMap& sm) const
{
	sm.clear();
	sm["mode"] = (float)currCamCtrlNum;
	currCamCtrl->GetState(sm);
}


bool CCameraHandler::SetState(const CCameraController::StateMap& sm)
{
	CCameraController::StateMap::const_iterator it = sm.find("mode");
	if (it != sm.end()) {
		const unsigned int camMode = (unsigned int)it->second;
		if (camMode >= camControllers.size()) {
			return false;
		}
		if (camMode != currCamCtrlNum) {
			currCamCtrlNum = camMode;
			currCamCtrl = camControllers[camMode];
			currCamCtrl->SwitchTo();
		}
	}
	return currCamCtrl->SetState(sm);
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
	else if (cmd == "vieworbit") {
		SetCameraMode(7);
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
		if (!action.extra.empty()) {
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
	else if (cmd == "togglecammode") {
		ToggleState();
	}
}


bool CCameraHandler::LoadViewData(const ViewData& vd)
{
	if (vd.empty()) {
		return false;
	}

	ViewData::const_iterator it = vd.find("mode");
	if (it != vd.end()) {
		const unsigned int camMode = (unsigned int)it->second;
		if (camMode >= camControllers.size()) {
			return false;
		}
		const unsigned int currentMode = currCamCtrlNum;
		if (camMode != currCamCtrlNum) {
			currCamCtrlNum = camMode;
			currCamCtrl = camControllers[camMode];
			const bool showMode = (camMode != currentMode);
			currCamCtrl->SwitchTo(showMode);
			CameraTransition(1.0f);
		}
	}
	return currCamCtrl->SetState(vd);
}

