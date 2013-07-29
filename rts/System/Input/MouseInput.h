/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOUSE_INPUT_H
#define MOUSE_INPUT_H

#include <SDL_events.h>
#include <boost/signals2/connection.hpp>

#include "System/Vec2.h"

struct int2;

class IMouseInput
{
public:
	static IMouseInput* GetInstance();
	static void FreeInstance(IMouseInput*);

	IMouseInput ();
	virtual ~IMouseInput() {}

	virtual int2 GetPos() { return mousepos; }
	void SetPos(int2 pos);

	bool HandleSDLMouseEvent(const SDL_Event& event);

	virtual void SetWMMouseCursor(void* wmcursor) {}
	
protected:
	int2 mousepos;
	boost::signals2::connection inputCon;
};

extern IMouseInput* mouseInput;

#endif 
