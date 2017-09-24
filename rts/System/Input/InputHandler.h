/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <slimsig/slimsig.h>
#include <SDL_events.h>

/**
 * @brief Simple thing: events go in, events come out
 *
 */
class InputHandler
{
public:
	typedef slimsig::signal<void (const SDL_Event&)> SignalType;

	InputHandler();

	void PushEvent(const SDL_Event& ev);
	void PushEvents();

	SignalType::connection_type AddHandler(SignalType::callback);

private:
	SignalType sig;
};

extern InputHandler input;

#endif // INPUT_HANDLER_H
