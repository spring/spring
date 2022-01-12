/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring> // memset
#include <cstdlib>
#include <cstdarg> // va_start

#include "CameraHandler.h"

#include "Action.h"
#include "Camera.h"
#include "Camera/CameraController.h"
#include "Camera/DummyController.h"
#include "Camera/FPSController.h"
#include "Camera/OverheadController.h"
#include "Camera/RotOverheadController.h"
#include "Camera/FreeController.h"
#include "Camera/OverviewController.h"
#include "Camera/SpringController.h"
#include "Players/Player.h"
#include "UI/UnitTracker.h"
#include "Rendering/GlobalRendering.h"
#include "System/SpringMath.h"
#include "System/SafeUtil.h"
#include "System/StringHash.h"
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
	.maximumValue(CCameraHandler::CAMERA_MODE_DUMMY - 1);

CONFIG(float, CamTimeFactor)
	.defaultValue(1.0f)
	.minimumValue(0.0f)
	.description("Scales the speed of camera transitions, e.g. zooming or position change.");

CONFIG(float, CamTimeExponent)
	.defaultValue(4.0f)
	.minimumValue(0.0f)
	.description("Camera transitions happen at lerp(old, new, timeNorm ^ CamTimeExponent).");


CCameraHandler* camHandler = nullptr;




// cameras[ACTIVE] is just used to store which of the others is active
static CCamera cameras[CCamera::CAMTYPE_COUNT];

static uint8_t camControllerMem[CCameraHandler::CAMERA_MODE_LAST][sizeof(CFreeController)];
static uint8_t camHandlerMem[sizeof(CCameraHandler)];


void CCameraHandler::SetActiveCamera(unsigned int camType) { cameras[CCamera::CAMTYPE_ACTIVE].SetCamType(camType); }
void CCameraHandler::InitStatic() {
	// initialize all global cameras
	for (unsigned int i = CCamera::CAMTYPE_PLAYER; i < CCamera::CAMTYPE_COUNT; i++) {
		cameras[i].SetCamType(i);
		cameras[i].SetProjType((i == CCamera::CAMTYPE_SHADOW)? CCamera::PROJTYPE_ORTHO: CCamera::PROJTYPE_PERSP);
		cameras[i].SetClipCtrlMatrix(CMatrix44f::ClipControl(globalRendering->supportClipSpaceControl));
	}

	SetActiveCamera(CCamera::CAMTYPE_PLAYER);

	camHandler = new (camHandlerMem) CCameraHandler();
}

void CCameraHandler::KillStatic() {
	spring::SafeDestruct(camHandler);
	std::memset(camHandlerMem, 0, sizeof(camHandlerMem));
}


CCamera* CCameraHandler::GetCamera(unsigned int camType) { return &cameras[camType]; }
CCamera* CCameraHandler::GetActiveCamera() { return (GetCamera(cameras[CCamera::CAMTYPE_ACTIVE].GetCamType())); }



// some controllers access the heightmap, do not construct them yet
CCameraHandler::CCameraHandler() {
	camControllers.fill(nullptr);
	camControllers[CAMERA_MODE_DUMMY] = new (camControllerMem[CAMERA_MODE_DUMMY]) CDummyController();
}

CCameraHandler::~CCameraHandler() {
	// regular controllers should already have been killed
	assert(camControllers[0] == nullptr);
	spring::SafeDestruct(camControllers[CAMERA_MODE_DUMMY]);
	std::memset(camControllerMem[CAMERA_MODE_DUMMY], 0, sizeof(camControllerMem[CAMERA_MODE_DUMMY]));
}


void CCameraHandler::Init()
{
	{
		InitControllers();

		for (unsigned int i = 0; i < CAMERA_MODE_DUMMY; i++) {
			nameModeMap[camControllers[i]->GetName()] = i;
		}
	}
	{
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

		RegisterAction("camtimefactor");
		RegisterAction("camtimeexponent");

		SortRegisteredActions();
	}

	{
		camTransState.startFOV  = 90.0f;
		camTransState.timeStart =  0.0f;
		camTransState.timeEnd   =  0.0f;

		camTransState.timeFactor   = configHandler->GetFloat("CamTimeFactor");
		camTransState.timeExponent = configHandler->GetFloat("CamTimeExponent");
	}

	SetCameraMode(configHandler->GetString("CamModeName"));

	for (CCameraController* cc: camControllers) {
		cc->Update();
	}
}

void CCameraHandler::Kill()
{
	KillControllers();
}


void CCameraHandler::InitControllers()
{
	static_assert(sizeof(        CFPSController) <= sizeof(camControllerMem[CAMERA_MODE_FIRSTPERSON]), "");
	static_assert(sizeof(   COverheadController) <= sizeof(camControllerMem[CAMERA_MODE_OVERHEAD   ]), "");
	static_assert(sizeof(     CSpringController) <= sizeof(camControllerMem[CAMERA_MODE_SPRING     ]), "");
	static_assert(sizeof(CRotOverheadController) <= sizeof(camControllerMem[CAMERA_MODE_ROTOVERHEAD]), "");
	static_assert(sizeof(       CFreeController) <= sizeof(camControllerMem[CAMERA_MODE_FREE       ]), "");
	static_assert(sizeof(   COverviewController) <= sizeof(camControllerMem[CAMERA_MODE_OVERVIEW   ]), "");

	// FPS camera must always be the first one in the list
	camControllers[CAMERA_MODE_FIRSTPERSON] = new (camControllerMem[CAMERA_MODE_FIRSTPERSON])         CFPSController();
	camControllers[CAMERA_MODE_OVERHEAD   ] = new (camControllerMem[CAMERA_MODE_OVERHEAD   ])    COverheadController();
	camControllers[CAMERA_MODE_SPRING     ] = new (camControllerMem[CAMERA_MODE_SPRING     ])      CSpringController();
	camControllers[CAMERA_MODE_ROTOVERHEAD] = new (camControllerMem[CAMERA_MODE_ROTOVERHEAD]) CRotOverheadController();
	camControllers[CAMERA_MODE_FREE       ] = new (camControllerMem[CAMERA_MODE_FREE       ])        CFreeController();
	camControllers[CAMERA_MODE_OVERVIEW   ] = new (camControllerMem[CAMERA_MODE_OVERVIEW   ])    COverviewController();
}

void CCameraHandler::KillControllers()
{
	if (camControllers[0] == nullptr)
		return;

	SetCameraMode(CAMERA_MODE_DUMMY);

	for (unsigned int i = 0; i < CAMERA_MODE_DUMMY; i++) {
		spring::SafeDestruct(camControllers[i]);
		std::memset(camControllerMem[i], 0, sizeof(camControllerMem[i]));
	}

	assert(camControllers[0] == nullptr);
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
	const unsigned int oldMode = currCamCtrlNum;

	if ((newMode >= camControllers.size()) || (newMode == oldMode))
		return;

	// switching {from, to} the dummy only happens on {init, kill}
	// in either case an interpolated transition is not necessary
	if (oldMode == CAMERA_MODE_DUMMY || newMode == CAMERA_MODE_DUMMY) {
		currCamCtrlNum = newMode;
		return;
	}

	CameraTransition(1.0f);

	CCameraController* oldCamCtrl = camControllers[                 oldMode];
	CCameraController* newCamCtrl = camControllers[currCamCtrlNum = newMode];

	newCamCtrl->SetPos(oldCamCtrl->SwitchFrom());
	newCamCtrl->SwitchTo(oldMode);
	newCamCtrl->Update();
}

void CCameraHandler::SetCameraMode(const std::string& modeName)
{
	const int modeNum = (!modeName.empty())? GetModeIndex(modeName): configHandler->GetInt("CamMode");

	// do nothing if the name is not matched
	if (modeNum < 0)
		return;

	SetCameraMode(modeNum);
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

	if (it != sm.cend()) {
		const unsigned int camMode = it->second;
		const unsigned int oldMode = currCamCtrlNum;

		if (camMode >= camControllers.size())
			return false;

		if (camMode != currCamCtrlNum)
			camControllers[currCamCtrlNum = camMode]->SwitchTo(oldMode);
	}

	const bool result = camControllers[currCamCtrlNum]->SetState(sm);
	camControllers[currCamCtrlNum]->Update();
	return result;
}


void CCameraHandler::PushAction(const Action& action)
{
	switch (hashString(action.command.c_str())) {
		case hashString("viewfps"): {
			SetCameraMode(CAMERA_MODE_FIRSTPERSON);
		} break;
		case hashString("viewta"): {
			SetCameraMode(CAMERA_MODE_OVERHEAD);
		} break;
		case hashString("viewspring"): {
			SetCameraMode(CAMERA_MODE_SPRING);
		} break;
		case hashString("viewrot"): {
			SetCameraMode(CAMERA_MODE_ROTOVERHEAD);
		} break;
		case hashString("viewfree"): {
				SetCameraMode(CAMERA_MODE_FREE);
		} break;
		case hashString("viewov"): {
			SetCameraMode(CAMERA_MODE_OVERVIEW);
		} break;

		case hashString("viewtaflip"): {
			COverheadController* ohCamCtrl = dynamic_cast<COverheadController*>(camControllers[CAMERA_MODE_OVERHEAD]);

			if (ohCamCtrl != nullptr) {
				if (!action.extra.empty()) {
					ohCamCtrl->flipped = !!atoi(action.extra.c_str());
				} else {
					ohCamCtrl->flipped = !ohCamCtrl->flipped;
				}

				ohCamCtrl->Update();
			}
		} break;

		case hashString("viewsave"): {
			if (!action.extra.empty()) {
				SaveView(action.extra);
				LOG("[CamHandler::%s] saved view \"%s\"", __func__, action.extra.c_str());
			}
		} break;
		case hashString("viewload"): {
			if (LoadView(action.extra))
				LOG("[CamHandler::%s] loaded view \"%s\"", __func__, action.extra.c_str());

		} break;

		case hashString("toggleoverview"): {
			ToggleOverviewCamera();
		} break;
		case hashString("togglecammode"): {
			ToggleState();
		} break;

		case hashString("camtimefactor"): {
			if (!action.extra.empty())
				camTransState.timeFactor = std::atof(action.extra.c_str());

			LOG("[CamHandler::%s] set transition-time factor to %f", __func__, camTransState.timeFactor);
		} break;
		case hashString("camtimeexponent"): {
			if (!action.extra.empty())
				camTransState.timeExponent = std::atof(action.extra.c_str());

			LOG("[CamHandler::%s] set transition-time exponent to %f", __func__, camTransState.timeExponent);
		} break;
	}
}


bool CCameraHandler::LoadViewData(const ViewData& vd)
{
	if (vd.empty())
		return false;

	const auto it = vd.find("mode");

	if (it != vd.cend()) {
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

