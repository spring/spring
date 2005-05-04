#pragma once
#include "object.h"
#include <deque>
#include <string>

class CInputReceiver :
	public CObject
{
public:
	CInputReceiver(void);
	virtual ~CInputReceiver(void);

	virtual bool KeyPressed(unsigned char key){return false;};
	virtual bool KeyReleased(unsigned char key){return false;};

	virtual bool MousePress(int x, int y, int button){return false;};
	virtual void MouseMove(int x, int y, int dx,int dy, int button){};
	virtual void MouseRelease(int x, int y, int button){};
	virtual bool IsAbove(int x, int y){return false;};
	virtual void Draw(){};
	virtual std::string GetTooltip(int x,int y){return "No tooltip defined";};

	static CInputReceiver* GetReceiverAt(int x,int y);

	struct ContainerBox {
		ContainerBox();
		ContainerBox operator+(ContainerBox other);

		float x1;
		float y1;
		float x2;
		float y2;
	};
	bool InBox(float x, float y, const ContainerBox& box);
	void DrawBox(ContainerBox b);
};

extern std::deque<CInputReceiver*> inputReceivers;
