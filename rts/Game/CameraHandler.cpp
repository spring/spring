/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdlib>

#include "System/mmgr.h"

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
#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

static std::string strformat(const char* fmt, ...)
{
	char buf[256];
	va_list args;
	va_start(args,fmt);
	VSNPRINTF(buf, sizeof(buf), fmt, args);
	va_end(args);
	return std::string(buf);
}

CONFIG(std::string, CamModeName).defaultValue("");

CONFIG(int, CamMode)
	.defaultValue(CCameraHandler::CAMERA_MODE_SMOOTH)
	.description(strformat("Defines the used camera. Options are:\n%i = FPS\n%i = Overhead\n%i = TotalWar\n%i = RotOverhead\n%i = Free\n%i = SmoothOverhead\n%i = Orbit\n%i = Overview",
		(int)CCameraHandler::CAMERA_MODE_FIRSTPERSON,
		(int)CCameraHandler::CAMERA_MODE_OVERHEAD,
		(int)CCameraHandler::CAMERA_MODE_TOTALWAR,
		(int)CCameraHandler::CAMERA_MODE_ROTOVERHEAD,
		(int)CCameraHandler::CAMERA_MODE_FREE,
		(int)CCameraHandler::CAMERA_MODE_SMOOTH,
		(int)CCameraHandler::CAMERA_MODE_ORBIT,
		(int)CCameraHandler::CAMERA_MODE_OVERVIEW
	).c_str())
	.minimumValue(0)
	.maximumValue(CCameraHandler::CAMERA_MODE_LAST - 1);

CONFIG(float, CamTimeFactor)
	.defaultValue(1.0f)
	.minimumValue(0.0f);

CONFIG(float, CamTimeExponent)
	.defaultValue(4.0f)
	.minimumValue(0.0f);


CCameraHandler* camHandler = NULL;


CCameraHandler::CCameraHandler()
{
	cameraTime = 0.0f;
	cameraTimeLeft = 0.0f;
	cameraTimeStart = 0.0f;
	cameraTimeEnd = 0.0f;

	// FPS camera must always be the first one in the list
	camControllers.resize(CAMERA_MODE_LAST);
	camControllers[CAMERA_MODE_FIRSTPERSON] = new CFPSController();
	camControllers[CAMERA_MODE_OVERHEAD   ] = new COverheadController();
	camControllers[CAMERA_MODE_TOTALWAR   ] = new CTWController();
	camControllers[CAMERA_MODE_ROTOVERHEAD] = new CRotOverheadController();
	camControllers[CAMERA_MODE_FREE       ] = new CFreeController();
	camControllers[CAMERA_MODE_SMOOTH     ] = new SmoothController();
	camControllers[CAMERA_MODE_ORBIT      ] = new COrbitController();
	camControllers[CAMERA_MODE_OVERVIEW   ] = new COverviewController();

	for (unsigned int i = 0; i < camControllers.size(); i++) {
		nameMap[camControllers[i]->GetName()] = i;
	}

	int modeIndex;
	const std::string modeName = configHandler->GetString("CamModeName");
	if (!modeName.empty()) {
		modeIndex = GetModeIndex(modeName);
	} else {
		modeIndex = configHandler->GetInt("CamMode");
	}

	currCamCtrlNum = modeIndex;
	currCamCtrl = camControllers[currCamCtrlNum];

	cameraTimeFactor   = configHandler->GetFloat("CamTimeFactor");
	cameraTimeExponent = configHandler->GetFloat("CamTimeExponent");

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
	while (!camControllers.empty()){
		delete camControllers.back();
		camControllers.pop_back();
	}
}


void CCameraHandler::UpdateCam()
{
	const float  wantedCamFOV = currCamCtrl->GetFOV();
	const float3 wantedCamPos = currCamCtrl->GetPos();
	const float3 wantedCamDir = currCamCtrl->GetDir();

#if 1
	const float curTime = spring_gettime()/1000.f;
	if (curTime >= cameraTimeEnd) {
		camera->pos = wantedCamPos;
		camera->forward = wantedCamDir;
		if (wantedCamFOV != camera->GetFov())
			camera->SetFov(wantedCamFOV);
	}
	else {
		const float exp = cameraTimeExponent;
		const float ratio = 1.0f - (float)math::pow((cameraTimeEnd - curTime)/(cameraTimeEnd - cameraTimeStart), exp);

		const float  deltaFOV = wantedCamFOV - cameraStart.GetFov();
		const float3 deltaPos = wantedCamPos - cameraStart.pos;
		const float3 deltaDir = wantedCamDir - cameraStart.forward;
		camera->SetFov(cameraStart.GetFov() + (deltaFOV * ratio));
		camera->pos     = cameraStart.pos + deltaPos * ratio;
		camera->forward = cameraStart.forward + deltaDir * ratio;
		camera->forward.Normalize();
	}
#else
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
		const float ratio = 1.0f - (float)math::pow((nextTime / currTime), exp);

		const float  deltaFOV = wantedCamFOV - camera->GetFov();
		const float3 deltaPos = wantedCamPos - camera->pos;
		const float3 deltaDir = wantedCamDir - camera->forward;
		camera->SetFov(camera->GetFov() + (deltaFOV * ratio));
		camera->pos     += deltaPos * ratio;
		camera->forward += deltaDir * ratio;
		camera->forward.Normalize();
	}
#endif
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
	if (!controllerStack.empty()) {
		SetCameraMode(controllerStack.top());
		controllerStack.pop();
	}
}


void CCameraHandler::CameraTransition(float time)
{
	time = std::max(time, 0.0f) * cameraTimeFactor;

	cameraTime = time;
	cameraTimeLeft = time;

	cameraTimeStart = spring_gettime()/1000.f;
	cameraTimeEnd = cameraTimeStart + time;
	cameraStart.CopyState(camera);
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
	if (controllerStack.empty()) {
		PushMode();
		SetCameraMode(CAMERA_MODE_OVERVIEW);
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
	} else {
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
		SetCameraMode(CAMERA_MODE_FIRSTPERSON);
	}
	else if (cmd == "viewta") {
		SetCameraMode(CAMERA_MODE_OVERHEAD);
	}
	else if (cmd == "viewtw") {
		SetCameraMode(CAMERA_MODE_TOTALWAR);
	}
	else if (cmd == "viewrot") {
		SetCameraMode(CAMERA_MODE_ROTOVERHEAD);
	}
	else if (cmd == "viewfree") {
		SetCameraMode(CAMERA_MODE_FREE);
	}
	else if (cmd == "viewov") {
		SetCameraMode(CAMERA_MODE_OVERVIEW);
	}
	else if (cmd == "viewlua") {
		SetCameraMode(CAMERA_MODE_SMOOTH); // ?
	}
	else if (cmd == "vieworbit") {
		SetCameraMode(CAMERA_MODE_ORBIT);
	}

	else if (cmd == "viewtaflip") {
		COverheadController* taCam = dynamic_cast<COverheadController*>(camControllers[CAMERA_MODE_OVERHEAD]);
		SmoothController* smCam = dynamic_cast<SmoothController*>(camControllers[CAMERA_MODE_SMOOTH]);

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
			LOG("Saved view: %s", action.extra.c_str());
		}
	}
	else if (cmd == "viewload") {
		if (!LoadView(action.extra)) {
			LOG_L(L_WARNING, "Loading view failed!");
		}
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

