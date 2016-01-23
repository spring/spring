/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CAMERA_HANDLER_H
#define _CAMERA_HANDLER_H

#include <vector>
#include <map>
#include <string>
#include <stack>

#include "Console.h"
#include "Camera/CameraController.h"
#include "Game/Camera.h"

class CPlayer;
class CCameraHandler : public CommandReceiver
{
public:
	typedef CCameraController::StateMap ViewData;

public:
	CCameraHandler();
	~CCameraHandler();

	void UpdateTransition();
	void UpdateController(CPlayer* player, bool fpsMode, bool fsEdgeMove, bool wnEdgeMove);

	void SetCameraMode(unsigned int mode);
	void SetCameraMode(const std::string& mode);
	void PushMode();
	void PopMode();
	void CameraTransition(float nsecs);

	void ToggleState();
	void ToggleOverviewCamera();

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

	CCameraController& GetCurrentController() { return *currCamCtrl; }
	int GetCurrentControllerNum() const { return currCamCtrlNum; }
	const std::string GetCurrentControllerName() const;
	const std::vector<CCameraController*>& GetAvailableControllers() const { return camControllers; }

	virtual void PushAction(const Action&);

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
	std::vector<CCameraController*> camControllers;
	std::stack<unsigned int> controllerStack;
	CCameraController* currCamCtrl;
	unsigned int currCamCtrlNum;

	struct {
		float3 pos;
		float3 rot;
		float  fov;
	} startCam;

	float cameraTimeStart;
	float cameraTimeEnd;
	float cameraTimeFactor;
	float cameraTimeExponent;

	bool LoadViewData(const ViewData& vd);
	std::map<std::string, ViewData> views;
	std::map<std::string, unsigned int> nameMap;
};

extern CCameraHandler* camHandler;

#endif // _CAMERA_HANDLER_H
