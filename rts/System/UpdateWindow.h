#ifndef UPDATEWINDOW_H
#define UPDATEWINDOW_H

#include "lib/liblobby/Connection.h"

#include "Game/GameVersion.h"
#include "aGui/Window.h"

namespace agui
{
	class TextElement;
	class Button;
};

class UpdateWindow : public agui::Window, public Connection
{
public:
	UpdateWindow();

	virtual void DoneConnecting(bool success, const std::string& err);
	virtual void DataReceived(const std::string& command, const std::string& msg);
	
private:
	agui::TextElement* label;
	agui::Button* close;
};


#endif