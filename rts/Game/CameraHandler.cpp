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
	))
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

	camTransState.startFOV  = 90.0f;
	camTransState.timeStart =  0.0f;
	camTransState.timeEnd   =  0.0f;

	// FPS camera must always be the first one in the list
	camControllers.resize(CAMERA_MODE_LAST, nullptr);
	camControllers[CAMERA_MODE_FIRSTPERSON] = new CFPSController();
	camControllers[CAMERA_MODE_OVERHEAD   ] = new COverheadController();
	camControllers[CAMERA_MODE_SPRING     ] = new CSpringController();
	camControllers[CAMERA_MODE_ROTOVERHEAD] = new CRotOverheadController();
	camControllers[CAMERA_MODE_FREE       ] = new CFreeController();
	camControllers[CAMERA_MODE_OVERVIEW   ] = new COverviewController();

	for (unsigned int i = 0; i < camControllers.size(); i++) {
		nameModeMap[camControllers[i]->GetName()] = i;
	}

	const std::string& modeName = configHandler->GetString("CamModeName");

	if (!modeName.empty()) {
		currCamCtrlNum = GetModeIndex(modeName);
	} else {
		currCamCtrlNum = configHandler->GetInt("CamMode");
	}

	camTransState.timeFactor   = configHandler->GetFloat("CamTimeFactor");
	camTransState.timeExponent = configHandler->GetFloat("CamTimeExponent");

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

	SetCameraMode(currCamCtrlNum);
}


CCameraHandler::~CCameraHandler()
{
	while (!camControllers.empty()) {
		delete camControllers.back();
		camControllers.pop_back();
	}

	RemoveGlobalCams();
}


void CCameraHandler::UpdateController(CPlayer* player, bool fpsMode, bool fsEdgeMove, bool wnEdgeMove)
{
	CCameraController& camCon = GetCurrentController();
	FPSUnitController& fpsCon = player->fpsController;

	const bool fusEdgeMove = ( globalRendering->fullScreen && fsEdgeMove);
	const bool winEdgeMove = (!globalRendering->fullScreen && wnEdgeMove);

	// We have to update transition both before and after updating the controller:
	// before: if the controller makes a new begincam, it needs to take into account previous transitions
	// after: apply changes made by the controller with 0 time transition.
	UpdateTransition();

	if (fpsCon.oldDCpos != ZeroVector) {
		camCon.SetPos(fpsCon.oldDCpos);
		fpsCon.oldDCpos = ZeroVector;
	}

	if (!fpsMode)
		UpdateController(camCon, true, true, fusEdgeMove || winEdgeMove);

	camCon.Update();

	UpdateTransition();
}

void CCameraHandler::UpdateController(CCameraController& camCon, bool keyMove, bool wheelMove, bool edgeMove)
{
	if (keyMove) {
		// NOTE: z-component contains speed scaling factor, xy is movement
		const float3 camMoveVector = camera->GetMoveVectorFromState(true);

		// key scrolling
		if ((camMoveVector * XYVector).SqLength() > 0.0f) {
			if (camCon.DisableTrackingByKey())
				unitTracker.Disable();

			camCon.KeyMove(camMoveVector);
		}
	}

	if (edgeMove) {
		const float3 camMoveVector = camera->GetMoveVectorFromState(false);

		// screen edge scrolling
		if ((camMoveVector * XYVector).SqLength() > 0.0f) {
			unitTracker.Disable();
			camCon.ScreenEdgeMove(camMoveVector);
		}
	}

	if (wheelMove) {
		// mouse wheel zoom
		float mouseWheelDist = 0.0f;
		mouseWheelDist +=  float(camera->GetMovState()[CCamera::MOVE_STATE_UP]);
		mouseWheelDist += -float(camera->GetMovState()[CCamera::MOVE_STATE_DWN]);
		mouseWheelDist *= 0.2f * globalRendering->lastFrameTime;

		if (mouseWheelDist == 0.0f)
			return;

		camCon.MouseWheelMove(mouseWheelDist);
	}
}


void CCameraHandler::CameraTransition(float nsecs)
{
	nsecs = std::max(nsecs, 0.0f) * camTransState.timeFactor;

	// calculate when transition should end based on duration in seconds
	camTransState.timeStart = globalRendering->lastFrameStart.toMilliSecsf();
	camTransState.timeEnd   = camTransState.timeStart + nsecs * 1000.0f;

	camTransState.startPos = camera->GetPos();
	camTransState.startRot = camera->GetRot();
	camTransState.startFOV = camera->GetVFOV();
}

void CCameraHandler::UpdateTransition()
{
	camTransState.tweenPos = camControllers[currCamCtrlNum]->GetPos();
	camTransState.tweenRot = camControllers[currCamCtrlNum]->GetRot();
	camTransState.tweenFOV = camControllers[currCamCtrlNum]->GetFOV();

	const float transTime = globalRendering->lastFrameStart.toMilliSecsf();

	if (transTime >= camTransState.timeEnd) {
		camera->SetPos(camTransState.tweenPos);
		camera->SetRot(camTransState.tweenRot);
		camera->SetVFOV(camTransState.tweenFOV);
		camera->Update();
		return;
	}

	if (camTransState.timeEnd <= camTransState.timeStart)
		return;

	const float timeRatio = (camTransState.timeEnd - transTime) / (camTransState.timeEnd - camTransState.timeStart);
	const float tweenFact = 1.0f - math::pow(timeRatio, camTransState.timeExponent);

	camera->SetPos(mix(camTransState.startPos, camTransState.tweenPos, tweenFact));
	camera->SetRot(mix(camTransState.startRot, camTransState.tweenRot, tweenFact));
	camera->SetVFOV(mix(camTransState.startFOV, camTransState.tweenFOV, tweenFact));
	camera->Update();
}


void CCameraHandler::SetCameraMode(unsigned int newMode)
{
	if ((newMode >= camControllers.size()) || (newMode == currCamCtrlNum))
		return;

	CameraTransition(1.0f);

	const unsigned int oldMode = currCamCtrlNum;

	CCameraController* oldCamCtrl = camControllers[currCamCtrlNum          ];
	CCameraController* newCamCtrl = camControllers[currCamCtrlNum = newMode];

	newCamCtrl->SetPos(oldCamCtrl->SwitchFrom());
	newCamCtrl->SwitchTo(oldMode);
	newCamCtrl->Update();
}

void CCameraHandler::SetCameraMode(const std::string& modeName)
{
	const int modeNum = GetModeIndex(modeName);

	// do nothing if the name is not matched
	if (modeNum >= 0) {
		SetCameraMode(modeNum);
	}
}


int CCameraHandler::GetModeIndex(const std::string& name) const
{
	const auto it = nameModeMap.find(name);

	if (it != nameModeMap.end())
		return it->second;

	return -1;
}


void CCameraHandler::PushMode()
{
	controllerStack.push_back(GetCurrentControllerNum());
}

void CCameraHandler::PopMode()
{
	if (controllerStack.empty())
		return;

	SetCameraMode(controllerStack.back());
	controllerStack.pop_back();
}


void CCameraHandler::ToggleState()
{
	unsigned int newMode = (currCamCtrlNum + 1) % camControllers.size();
	unsigned int numTries = 0;

	const unsigned int maxTries = camControllers.size() - 1;

	while ((numTries++ < maxTries) && !camControllers[newMode]->enabled) {
		newMode = (newMode + 1) % camControllers.size();
	}

	SetCameraMode(newMode);
}


void CCameraHandler::ToggleOverviewCamera()
{
	CameraTransition(1.0f);

	if (controllerStack.empty()) {
		PushMode();
		SetCameraMode(CAMERA_MODE_OVERVIEW);
	} else {
		PopMode();
	}
}


void CCameraHandler::SaveView(const std::string& name)
{
	if (name.empty())
		return;

	ViewData& vd = viewDataMap[name];
	vd["mode"] = currCamCtrlNum;

	camControllers[currCamCtrlNum]->GetState(vd);
	return;
}

bool CCameraHandler::LoadView(const std::string& name)
{
	if (name.empty())
		return false;

	const auto it = viewDataMap.find(name);

	if (it == viewDataMap.end())
		return false;

	const ViewData& saved = it->second;

	ViewData current;
	GetState(current);

	if (saved == current) {
		// load a view twice to return to old settings
		// safety: should not happen, but who knows?
		if (name != "__old_view")
			return LoadView("__old_view");

		return false;
	}

	if (name != "__old_view")
		SaveView("__old_view");

	return LoadViewData(saved);
}


void CCameraHandler::GetState(CCameraController::StateMap& sm) const
{
	sm.clear();
	sm["mode"] = currCamCtrlNum;

	camControllers[currCamCtrlNum]->GetState(sm);
}


bool CCameraHandler::SetState(const CCameraController::StateMap& sm)
{
	const auto it = sm.find("mode");

	if (it != sm.end()) {
		const unsigned int camMode = it->second;
		const unsigned int oldMode = currCamCtrlNum;

		if (camMode >= camControllers.size())
			return false;

		if (camMode != currCamCtrlNum) {
			camControllers[currCamCtrlNum = camMode]->SwitchTo(oldMode);
		}
	}

	const bool result = camControllers[currCamCtrlNum]->SetState(sm);
	camControllers[currCamCtrlNum]->Update();
	return result;
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
	if (vd.empty())
		return false;

	const auto it = vd.find("mode");

	if (it != vd.end()) {
		const unsigned int camMode = it->second;
		const unsigned int curMode = currCamCtrlNum;

		if (camMode >= camControllers.size())
			return false;

		if (camMode != currCamCtrlNum) {
			CameraTransition(1.0f);
			camControllers[currCamCtrlNum = camMode]->SwitchTo(curMode, camMode != curMode);
		}
	}

	return camControllers[currCamCtrlNum]->SetState(vd);
}

