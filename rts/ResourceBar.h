#ifndef RESOURCEBAR_H
#define RESOURCEBAR_H
/*pragma once removed*/
#include "InputReceiver.h"

class CResourceBar :
	public CInputReceiver
{
public:
	CResourceBar(void);
	virtual ~CResourceBar(void);
	void Draw(void);

	bool IsAbove(int x, int y);
	std::string GetTooltip(int x, int y);

	bool MousePress(int x, int y, int button);
	void MouseMove(int x, int y, int dx,int dy, int button);

	ContainerBox box;
	ContainerBox metalBox;
	ContainerBox energyBox;

	bool moveBox;
};

extern CResourceBar* resourceBar;

#endif /* RESOURCEBAR_H */
