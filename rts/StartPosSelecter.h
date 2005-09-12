#ifndef STARTPOSSELECTER_H
#define STARTPOSSELECTER_H

#include "InputReceiver.h"

class CStartPosSelecter :
	public CInputReceiver
{
public:
	CStartPosSelecter(void);
	~CStartPosSelecter(void);

	virtual bool MousePress(int x, int y, int button);
	virtual void Draw();

	ContainerBox readyBox;
};


#endif /* STARTPOSSELECTER_H */
