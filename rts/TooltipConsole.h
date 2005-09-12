#ifndef TOOLTIPCONSOLE_H
#define TOOLTIPCONSOLE_H

#include "InputReceiver.h"

class CTooltipConsole :
	public CInputReceiver
{
public:
	CTooltipConsole(void);
	~CTooltipConsole(void);
	void Draw(void);
	bool IsAbove(int x,int y);
};

extern CTooltipConsole* tooltip;

#endif /* TOOLTIPCONSOLE_H */
