/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __CAMERAHANDLER_H__
#define __CAMERAHANDLER_H__

#include <vector>
#include <map>
#include <string>
#include <stack>

#include "Console.h"
#include "Camera/CameraController.h"


class CCameraHandler : public CommandReceiver
{
public:
	typedef CCameraController::StateMap ViewData;

public:
	CCameraHandler();
	~CCameraHandler();

	void UpdateCam();
	void SetCameraMode(unsigned int mode);
	void SetCameraMode(const std::string& mode);
	void PushMode();
	void PopMode();
	void CameraTransition(float time);

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
	 * @param fv the state to set
	 * @return false when vector has wrong size or garbage data, true when aplied without errors
	 */
	bool SetState(const CCameraController::StateMap& sm);

	CCameraController& GetCurrentController() { return *currCamCtrl; }
	int GetCurrentControllerNum() const { return currCamCtrlNum; }
	const std::string GetCurrentControllerName() const;
	const std::vector<CCameraController*>& GetAvailableControllers() const { return camControllers; }

	virtual void PushAction(const Action&);

private:
	enum {
		CAMERA_MODE_FIRSTPERSON = 0,
		CAMERA_MODE_OVERHEAD    = 1,
		CAMERA_MODE_TOTALWAR    = 2,
		CAMERA_MODE_ROTOVERHEAD = 3,
		CAMERA_MODE_FREE        = 4,
		CAMERA_MODE_SMOOTH      = 5,
		CAMERA_MODE_ORBIT       = 6,
		CAMERA_MODE_OVERVIEW    = 7,
		CAMERA_MODE_LAST        = 8,
	};

	std::vector<CCameraController*> camControllers;
	std::stack<unsigned int> controllerStack;
	CCameraController* currCamCtrl;
	unsigned int currCamCtrlNum;

	float cameraTime;
	float cameraTimeLeft;
	float cameraTimeFactor;
	float cameraTimeExponent;

	bool LoadViewData(const ViewData& vd);
	std::map<std::string, ViewData> views;
	std::map<std::string, unsigned int> nameMap;
};

extern CCameraHandler* camHandler;

#endif // __CAMERAHANDLER_H__
