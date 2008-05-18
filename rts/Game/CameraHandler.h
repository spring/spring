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
public:
	CCameraHandler();
	~CCameraHandler();
	
	void UpdateCam();
	void SetCameraMode(unsigned mode);
	void PushMode();
	void PopMode();
	void CameraTransition(float time);
	
	void ToggleState();
	void ToggleOverviewCamera(void);
	
	void SaveView(const std::string& name);
	bool LoadView(const std::string& name);
	
	CCameraController& GetCurrentController() {return *currCamCtrl;};
	int GetCurrentControllerNum() const {return currCamCtrlNum;};
	const std::vector<CCameraController*>& GetAvailableControllers() const {return camControllers;};
	
	virtual void PushAction(const Action&);
	
private:
	std::vector<CCameraController*> camControllers;
	std::stack<unsigned> controllerStack;
	CCameraController* currCamCtrl;
	int currCamCtrlNum;
	
	float cameraTime;
	float cameraTimeLeft;
	float cameraTimeFactor;
	float cameraTimeExponent;
	
	struct ViewData {
		bool operator==(const ViewData& vd) const {
			return (mode == vd.mode) && (state == vd.state);
		}
		int mode;
		std::vector<float> state;
	};
	bool LoadViewData(const ViewData& vd);	
	ViewData tmpView;
	std::map<std::string, ViewData> views;
};

extern CCameraHandler* camHandler;


#endif
