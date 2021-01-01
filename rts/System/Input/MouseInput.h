/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOUSE_INPUT_H
#define MOUSE_INPUT_H

#include <SDL_events.h>
#include <slimsig/connection.h>
#include "System/Input/InputHandler.h"

#include "System/type2.h"

class IMouseInput
{
public:
	static IMouseInput* GetInstance(bool relModeWarp);
	static void FreeInstance(IMouseInput*);

	IMouseInput() = default;
	IMouseInput(bool relModeWarp);
	virtual ~IMouseInput();

	virtual void InstallWndCallback() {}

	int2 GetPos() const { return mousepos; }

	bool SetPos(int2 pos);
	bool WarpPos(int2 pos);
	bool SetWarpPos(int2 pos) { return (SetPos(pos) && WarpPos(pos)); }

	bool HandleSDLMouseEvent(const SDL_Event& event);

	virtual void SetWMMouseCursor(void* wmcursor) {}

protected:
	int2 mousepos;
	InputHandler::SignalType::connection_type inputCon;
};

extern IMouseInput* mouseInput;

#endif
