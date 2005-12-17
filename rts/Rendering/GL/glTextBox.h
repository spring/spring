#ifndef GLTEXTBOX_H
#define GLTEXTBOX_H

#include "Game/UI/InputReceiver.h"
#include <string>
#include <vector>

class CglTextBox :
	public CInputReceiver
{
public:
	CglTextBox(std::string heading,std::string text,int autoBreakAt=0);
	virtual ~CglTextBox(void);
	void Draw(void);
	bool IsAbove(int x, int y);
	bool MousePress(int x, int y, int button);
	void MouseRelease(int x, int y,int button);

	std::string heading;
	std::vector<std::string> text;

	ContainerBox box,okButton;
};


#endif /* GLTEXTBOX_H */
