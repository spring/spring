#ifndef CAMERAHANDLER_H
#define CAMERAHANDLER_H

#include <vector>
#include <map>
#include <string>
#include <stack>

#include "Console.h"

class CCameraController;


class CCameraHandler : public CommandReceiver
{
	typedef std::vector<float> ViewData;
public:
	CCameraHandler();
	~CCameraHandler();
	
	void UpdateCam();
	void SetCameraMode(unsigned mode);
	void PushMode();
	void PopMode();
	void CameraTransition(float time);
	
	void ToggleState();
	void ToggleOverviewCamera();
	
	void SaveView(const std::string& name);
	bool LoadView(const std::string& name);
	
	/**
	@brief write current camera settings in a vector
	@param fv vector to hold the data, will be cleared, first float = controller number
	*/
	void GetState(ViewData& fv) const;
	
	/**
	@brief restore a camera state
	@param fv the state to set
	@return false when vector has wrong size or garbage data, true when aplied without errors
	*/
	bool SetState(const ViewData& fv);
	
	CCameraController& GetCurrentController() {return *currCamCtrl;};
	int GetCurrentControllerNum() const {return currCamCtrlNum;};
	const std::string GetCurrentControllerName() const;
	const std::vector<CCameraController*>& GetAvailableControllers() const {return camControllers;};
	
	virtual void PushAction(const Action&);
	
private:
	std::vector<CCameraController*> camControllers;
	std::stack<unsigned> controllerStack;
	CCameraController* currCamCtrl;
	unsigned currCamCtrlNum;
	
	float cameraTime;
	float cameraTimeLeft;
	float cameraTimeFactor;
	float cameraTimeExponent;
	
	bool LoadViewData(const ViewData& vd);
	std::map<std::string, ViewData> views;
};

extern CCameraHandler* camHandler;


#endif
