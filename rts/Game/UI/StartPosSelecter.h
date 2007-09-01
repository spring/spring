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

	bool Ready();
	void ShowReady(bool value) { showReady = value; }

	static CStartPosSelecter* selector;
private:
	bool showReady;
	ContainerBox readyBox;
};


#endif /* STARTPOSSELECTER_H */
