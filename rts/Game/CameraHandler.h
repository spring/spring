/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CAMERA_HANDLER_H
#define _CAMERA_HANDLER_H

#include <unordered_map>
#include <vector>
#include <string>

#include "Camera/CameraController.h"
#include "Console.h"

class CCamera;
class CPlayer;

class CCameraHandler : public CommandReceiver
{
public:
	typedef CCameraController::StateMap ViewData;

public:
	CCameraHandler();
	~CCameraHandler();

	void UpdateController(CPlayer* player, bool fpsMode, bool fsEdgeMove, bool wnEdgeMove);

	void SetCameraMode(unsigned int mode);
	void SetCameraMode(const std::string& mode);
	void PushMode();
	void PopMode();

	void CameraTransition(float nsecs);
	void UpdateTransition();

	void ToggleState();
	void ToggleOverviewCamera();

	void PushAction(const Action&);

	void SaveView(const std::string& name);
	bool LoadView(const std::string& name);

	int GetModeIndex(const std::string& modeName) const;

	/**
	 * @brief write current camera settings in a vector
	 */
	void GetState(CCameraController::StateMap& sm) const;

	/**
	 * @brief restore a camera state
	 * @param sm the state to set
	 * @return false when vector has wrong size or garbage data, true when aplied without errors
	 */
	bool SetState(const CCameraController::StateMap& sm);

	CCameraController& GetCurrentController() { return *(camControllers[currCamCtrlNum]); }
	unsigned int GetCurrentControllerNum() const { return currCamCtrlNum; }

	const std::vector<CCameraController*>& GetControllers() const { return camControllers; }

	enum {
		CAMERA_MODE_FIRSTPERSON = 0,
		CAMERA_MODE_OVERHEAD    = 1,
		CAMERA_MODE_SPRING      = 2,
		CAMERA_MODE_ROTOVERHEAD = 3,
		CAMERA_MODE_FREE        = 4,
		CAMERA_MODE_OVERVIEW    = 5,
		CAMERA_MODE_LAST        = 6,
	};

private:
	void UpdateController(CCameraController& camCon, bool keyMove, bool wheelMove, bool edgeMove);
	bool LoadViewData(const ViewData& vd);

private:
	unsigned int currCamCtrlNum;

	struct CamTransitionState {
		float3 startPos;
		float3 tweenPos;
		float3 startRot;
		float3 tweenRot;

		float startFOV;
		float tweenFOV;

		float timeStart;
		float timeEnd;
		float timeFactor;
		float timeExponent;
	};

	CamTransitionState camTransState;

	std::vector<CCameraController*> camControllers;
	std::vector<unsigned int> controllerStack;

	std::unordered_map<std::string, ViewData> viewDataMap;
	std::unordered_map<std::string, unsigned int> nameModeMap;
};

extern CCameraHandler* camHandler;

#endif // _CAMERA_HANDLER_H
