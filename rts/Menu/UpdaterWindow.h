/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UPDATERWINDOW_H
#define UPDATERWINDOW_H

#include "aGui/Window.h"

namespace agui
{
	class TextElement;
	class LineEdit;
};
class Connection;

class UpdaterWindow : public agui::Window
{
public:
	UpdaterWindow(Connection* con);
	~UpdaterWindow();

	void Login();
	void Register();

	void ShowAggreement(const std::string& text);
	void AcceptAgreement();
	void RejectAgreement();

	void ServerLabel(const std::string& text);
	void Label(const std::string& text);

private:
	agui::TextElement* serverLabel;
	agui::TextElement* label;
	agui::LineEdit* user;
	agui::LineEdit* passwd;
	agui::Window* agreement;
	Connection* con;
};

#endif
