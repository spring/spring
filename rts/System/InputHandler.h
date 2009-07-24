#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <list>
#include <boost/function.hpp>
#include <SDL_events.h>

/**
 * @brief Simple thing: events go in, events come out
 *
 */
class InputHandler
{
public:
	// return bool means do not send event to other handlers
	typedef boost::function<bool(const SDL_Event&)> HandlerFunc;
	InputHandler();
	
	void AddHandler(HandlerFunc);
	void RemoveHandler(HandlerFunc);

	void PushEvent(const SDL_Event& ev);
	
private:
	std::list<HandlerFunc> handlers;
};

extern InputHandler input;

#endif
