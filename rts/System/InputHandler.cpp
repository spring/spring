#include "StdAfx.h"
#include "InputHandler.h"

InputHandler input;

InputHandler::InputHandler()
{
}

void InputHandler::PushEvent(const SDL_Event& ev)
{
	sig(ev);
}

boost::signals::connection InputHandler::AddHandler(SignalType::slot_function_type handler)
{
	return sig.connect(handler);
}
