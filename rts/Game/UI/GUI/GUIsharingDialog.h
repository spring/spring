#ifndef GUISHARINGDIALOG_H
#define GUISHARINGDIALOG_H

#include "GUIcontroller.h"

class GUIslider;
class GUIstateButton;
class GUItable;

class GUIsharingDialog : public GUIdialogController
{
public:
	GUIsharingDialog();
	virtual ~GUIsharingDialog();
	
	void DialogEvent(const std::string& event);

protected:
	void ButtonPressed(GUIbutton* b);
	void TableSelection(GUItable* table, int i,int button);
	void SliderChanged(GUIslider* slider, int i);
	
	void UpdatePlayerList();
	
	GUIframe* CreateControl(const std::string& type, int x, int y, int w, int h, TdfParser& parser);
	
	GUIslider* giveEnergy;
	GUIslider* giveMetal;
	GUIslider* shareEnergy;
	GUIslider* shareMetal;
	
	GUIstateButton* giveUnits;
	GUIstateButton* shareVision;
	
	GUItable *table;
};

#endif	// GUISHARINGDIALOG_H
