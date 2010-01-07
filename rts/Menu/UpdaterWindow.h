#ifndef UPDATERWINDOW_H
#define UPDATERWINDOW_H

#include "lib/liblobby/Connection.h"

#include "aGui/Window.h"

namespace agui
{
	class TextElement;
	class LineEdit;
};

class UpdaterWindow : public agui::Window, public Connection
{
public:
	UpdaterWindow();

	virtual void DoneConnecting(bool success, const std::string& err);
	virtual void ServerGreeting(const std::string& serverVer, const std::string& springVer, int udpport, int mode);
	virtual void Denied(const std::string& reason);
	virtual void LoginEnd();
	virtual void RegisterDenied(const std::string& reason);
	virtual void RegisterAccept();
	
private:
	void Login();
	void Register();
	
	agui::TextElement* serverLabel;
	agui::TextElement* label;
	agui::LineEdit* user;
	agui::LineEdit* passwd;
};


#endif