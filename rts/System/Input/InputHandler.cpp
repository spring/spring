/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "InputHandler.h"
#include "System/TimeProfiler.h"

InputHandler input;

InputHandler::InputHandler()
{
}

void InputHandler::PushEvent(const SDL_Event& ev)
{
	sig(ev);
}

void InputHandler::PushEvents()
{
	SCOPED_TIMER("Misc::InputHandler::PushEvents");
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		// SDL_PollEvent may modify FPU flags
		streflop::streflop_init<streflop::Simple>();
		PushEvent(event);
	}
}


boost::signals2::connection InputHandler::AddHandler(SignalType::slot_function_type handler)
{
	return sig.connect(handler);
}
