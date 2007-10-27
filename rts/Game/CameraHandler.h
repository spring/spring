#ifndef CAMERAHANDLER_H
#define CAMERAHANDLER_H

#include <vector>
#include <map>
#include <string>
#include <stack>


class CCameraController;


class CCameraHandler
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
	
	CCameraController* &currCamCtrl;
	std::vector<CCameraController*> camControllers;
	
private:
	std::stack<unsigned> controllerStack;
	int currCamCtrlNum;
	int preControlCamNum;
	CCameraController* overviewController;
	
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
