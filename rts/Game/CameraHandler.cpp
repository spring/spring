/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdlib>
#include <stdarg.h>

#include "CameraHandler.h"

#include "Action.h"
#include "Camera.h"
#include "Camera/CameraController.h"
#include "Camera/FPSController.h"
#include "Camera/OverheadController.h"
#include "Camera/RotOverheadController.h"
#include "Camera/FreeController.h"
#include "Camera/OverviewController.h"
#include "Camera/SpringController.h"
#include "Players/Player.h"
#include "UI/UnitTracker.h"
#include "Rendering/GlobalRendering.h"
#include "System/myMath.h"
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
	.defaultValue(CCameraHandler::CAMERA_MODE_SPRING)
	.description(strformat("Defines the used camera. Options are:\n%i = FPS\n%i = Overhead\n%i = Spring\n%i = RotOverhead\n%i = Free\n%i = Overview",
		(int)CCameraHandler::CAMERA_MODE_FIRSTPERSON,
		(int)CCameraHandler::CAMERA_MODE_OVERHEAD,
		(int)CCameraHandler::CAMERA_MODE_SPRING,
		(int)CCameraHandler::CAMERA_MODE_ROTOVERHEAD,
		(int)CCameraHandler::CAMERA_MODE_FREE,
		(int)CCameraHandler::CAMERA_MODE_OVERVIEW
	).c_str())
	.minimumValue(0)
	.maximumValue(CCameraHandler::CAMERA_MODE_LAST - 1);

CONFIG(float, CamTimeFactor)
	.defaultValue(1.0f)
	.minimumValue(0.0f)
	.description("Scales the speed of camera transitions, e.g. zooming or position change.");

CONFIG(float, CamTimeExponent)
	.defaultValue(4.0f)
	.minimumValue(0.0f)
	.description("Camera transitions happen at lerp(old, new, timeNorm ^ CamTimeExponent).");


CCameraHandler* camHandler = nullptr;


static void CreateGlobalCams() {
	// create all global cameras
	for (unsigned int i = CCamera::CAMTYPE_PLAYER; i < CCamera::CAMTYPE_ACTIVE; i++) {
		CCamera::SetCamera(i, new CCamera(i));
	}

	CCamera::SetCamera(CCamera::CAMTYPE_ACTIVE, CCamera::GetCamera(CCamera::CAMTYPE_PLAYER));
}

static void RemoveGlobalCams() {
	// remove all global cameras
	for (unsigned int i = CCamera::CAMTYPE_PLAYER; i < CCamera::CAMTYPE_ACTIVE; i++) {
		delete CCamera::GetCamera(i); CCamera::SetCamera(i, nullptr);
	}

	CCamera::SetCamera(CCamera::CAMTYPE_ACTIVE, nullptr);
}


CCameraHandler::CCameraHandler()
{
	CreateGlobalCams();

	cameraTimeStart = 0.0f;
	cameraTimeEnd   = 0.0f;
	startCam.fov    = 90.0f;

	// FPS camera must always be the first one in the list
	camControllers.resize(CAMERA_MODE_LAST, nullptr);
	camControllers[CAMERA_MODE_FIRSTPERSON] = new CFPSController();
	camControllers[CAMERA_MODE_OVERHEAD   ] = new COverheadController();
	camControllers[CAMERA_MODE_SPRING     ] = new CSpringController();
	camControllers[CAMERA_MODE_ROTOVERHEAD] = new CRotOverheadController();
	camControllers[CAMERA_MODE_FREE       ] = new CFreeController();
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
	RegisterAction("viewspring");
	RegisterAction("viewrot");
	RegisterAction("viewfree");
	RegisterAction("viewov");

	RegisterAction("viewtaflip");

	RegisterAction("toggleoverview");
	RegisterAction("togglecammode");

	RegisterAction("viewsave");
	RegisterAction("viewload");

	SetCameraMode(modeIndex);
}


CCameraHandler::~CCameraHandler()
{
	while (!camControllers.empty()){
		delete camControllers.back();
		camControllers.pop_back();
	}

	RemoveGlobalCams();
}


void CCameraHandler::UpdateController(CPlayer* player, bool fpsMode, bool fsEdgeMove, bool wnEdgeMove)
{
	CCameraController& cc = GetCurrentController();
	FPSUnitController& fpsCon = player->fpsController;

	if (fpsCon.oldDCpos != ZeroVector) {
		cc.SetPos(fpsCon.oldDCpos);
		fpsCon.oldDCpos = ZeroVector;
	}

	if (!fpsMode) {
		{
			// NOTE: z-component contains speed scaling factor, xy is movement
			const float3 camMoveVector = camera->GetMoveVectorFromState(true);

			// key scrolling
			if ((camMoveVector * XYVector).SqLength() > 0.0f) {
				if (cc.DisableTrackingByKey())
					unitTracker.Disable();

				cc.KeyMove(camMoveVector);
			}
		}

		if ((globalRendering->fullScreen && fsEdgeMove) || (!globalRendering->fullScreen && wnEdgeMove)) {
			const float3 camMoveVector = camera->GetMoveVectorFromState(false);

			// screen edge scrolling
			if ((camMoveVector * XYVector).SqLength() > 0.0f) {
				unitTracker.Disable();
				cc.ScreenEdgeMove(camMoveVector);
			}
		}

		// mouse wheel zoom
		const float mouseWheelDistUp   = camera->GetMoveDistance(nullptr, nullptr, CCamera::MOVE_STATE_UP);
		const float mouseWheelDistDown = camera->GetMoveDistance(nullptr, nullptr, CCamera::MOVE_STATE_DWN);

		if (math::fabsf(mouseWheelDistUp + mouseWheelDistDown) > 0.0f)
			cc.MouseWheelMove(mouseWheelDistUp + mouseWheelDistDown);
	}

	cc.Update();
}

void CCameraHandler::UpdateTransition()
{
	const float  wantedCamFOV = currCamCtrl->GetFOV();
	const float3 wantedCamPos = currCamCtrl->GetPos();
	const float3 wantedCamRot = currCamCtrl->GetRot();

	const float curTime = spring_now().toMilliSecsf();

	if (curTime >= cameraTimeEnd) {
		camera->SetPos(wantedCamPos);
		camera->SetRot(wantedCamRot);
		camera->SetVFOV(wantedCamFOV);
	} else {
		if (cameraTimeEnd > cameraTimeStart) {
			const float timeRatio = (cameraTimeEnd - curTime) / (cameraTimeEnd - cameraTimeStart);
			const float tweenFact = 1.0f - math::pow(timeRatio, cameraTimeExponent);

			camera->SetPos(mix(startCam.pos, wantedCamPos, tweenFact));
			camera->SetRot(mix(startCam.rot, wantedCamRot, tweenFact));
			camera->SetVFOV(mix(startCam.fov, wantedCamFOV, tweenFact));
		}
	}

	camera->Update();
}


void CCameraHandler::CameraTransition(float nsecs)
{
	UpdateTransition(); // prevents camera stutter when multithreading

	nsecs = std::max(nsecs, 0.0f) * cameraTimeFactor;

	// calculate when transition should end based on duration in seconds
	cameraTimeStart = spring_now().toMilliSecsf();
	cameraTimeEnd   = cameraTimeStart + nsecs * 1000.0f;

	startCam.pos = camera->GetPos();
	startCam.rot = camera->GetRot();
	startCam.fov = camera->GetVFOV();
}


void CCameraHandler::SetCameraMode(unsigned int mode)
{

	if ((mode >= camControllers.size()) || (mode == static_cast<unsigned int>(currCamCtrlNum))) {
		return;
	}

	CameraTransition(1.0f);

	CCameraController* oldCamCtrl = currCamCtrl;

	const int oldMode = currCamCtrlNum;
	currCamCtrlNum = mode;
	currCamCtrl = camControllers[mode];
	currCamCtrl->SetPos(oldCamCtrl->SwitchFrom());
	currCamCtrl->SwitchTo(oldMode);
	currCamCtrl->Update();
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


void CCameraHandler::ToggleState()
{
	auto newMode = currCamCtrlNum + 1;
	newMode %= camControllers.size();

	int a = 0;
	const int maxTries = camControllers.size() - 1;
	while ((a < maxTries) && !camControllers[newMode]->enabled) {
		newMode++;
		newMode %= camControllers.size();
		a++;
	}

	SetCameraMode(newMode);
}


void CCameraHandler::ToggleOverviewCamera()
{

	CameraTransition(1.0f);
	if (controllerStack.empty()) {
		PushMode();
		SetCameraMode(CAMERA_MODE_OVERVIEW);
	}
	else {
		PopMode();
	}
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
			CameraTransition(1.0f);
			const int oldMode = currCamCtrlNum;
			currCamCtrlNum = camMode;
			currCamCtrl = camControllers[camMode];
			currCamCtrl->SwitchTo(oldMode);
		}
	}
	bool result = currCamCtrl->SetState(sm);
	currCamCtrl->Update();
	return result;
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
	else if (cmd == "viewspring") {
		SetCameraMode(CAMERA_MODE_SPRING);
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

	else if (cmd == "viewtaflip") {
		COverheadController* taCam = dynamic_cast<COverheadController*>(camControllers[CAMERA_MODE_OVERHEAD]);

		if (taCam) {
			if (!action.extra.empty()) {
				taCam->flipped = !!atoi(action.extra.c_str());
			} else {
				taCam->flipped = !taCam->flipped;
			}
			taCam->Update();
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
			CameraTransition(1.0f);
			const int oldMode = currCamCtrlNum;
			currCamCtrlNum = camMode;
			currCamCtrl = camControllers[camMode];
			const bool showMode = (camMode != currentMode);
			currCamCtrl->SwitchTo(oldMode, showMode);
		}
	}
	return currCamCtrl->SetState(vd);
}

