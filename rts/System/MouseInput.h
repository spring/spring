#ifndef MOUSE_INPUT_H
#define MOUSE_INPUT_H

#include <SDL_events.h>

class IMouseInput
{
public:
	static IMouseInput* Get();

	IMouseInput ();
	virtual ~IMouseInput() {}

	virtual int2 GetPos () = 0;
	virtual void SetPos (int2 pos) = 0;

	virtual void Update () {}

	virtual void HandleSDLMouseEvent (SDL_Event& event) = 0;

protected:
	Uint8 scrollWheelSpeed;
};


extern IMouseInput *mouseInput;

#endif 

