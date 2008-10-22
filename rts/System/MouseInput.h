#ifndef MOUSE_INPUT_H
#define MOUSE_INPUT_H

#include <SDL_events.h>

#include "Vec2.h"

class IMouseInput
{
public:
	static IMouseInput* Get();

	IMouseInput ();
	virtual ~IMouseInput();

	virtual int2 GetPos () = 0;
	virtual void SetPos (int2 pos) = 0;

	virtual void Update () {}

	virtual void HandleSDLMouseEvent (SDL_Event& event) = 0;

	virtual void SetWMMouseCursor (void* wmcursor) {}
};


extern IMouseInput *mouseInput;

#endif 

