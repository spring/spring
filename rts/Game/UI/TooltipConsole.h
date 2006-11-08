#ifndef TOOLTIPCONSOLE_H
#define TOOLTIPCONSOLE_H

#include "InputReceiver.h"

class CTooltipConsole : public CInputReceiver {
	public:
		CTooltipConsole(void);
		~CTooltipConsole(void);
		void Draw(void);
		bool IsAbove(int x,int y);
		bool disabled;
	protected:
		float x, y, w, h;
		bool outFont;
};

extern CTooltipConsole* tooltip;

#endif /* TOOLTIPCONSOLE_H */
