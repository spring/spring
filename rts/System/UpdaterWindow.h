#ifndef UPDATERWINDOW_H
#define UPDATERWINDOW_H

#include "lib/liblobby/Connection.h"

#include "Game/GameVersion.h"
#include "aGui/Window.h"

namespace agui
{
	class TextElement;
	class Button;
};

class UpdaterWindow : public agui::Window, public Connection
{
public:
	UpdaterWindow();

	virtual void DoneConnecting(bool success, const std::string& err);
	virtual void DataReceived(const std::string& command, const std::string& msg);
	
private:
	agui::TextElement* label;
	agui::Button* close;
};


#endif