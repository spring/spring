#ifndef GUIENDGAMEDIALOG_H
#define GUIENDGAMEDIALOG_H

#include "GUIcontroller.h"
#include "GUIgraph.h"

#include <vector>
#include <map>

class GUIslider;
class GUIstateButton;
class GUItable;

class GUIendgameDialog : public GUIdialogController
{
public:
	GUIendgameDialog();
	virtual ~GUIendgameDialog();

	void DialogEvent(const std::string& event);

	void UpdateStatistics();

protected:
	void ButtonPressed(GUIbutton* b);
	void TableSelection(GUItable* table, int i,int button);

	GUIframe* CreateControl(const std::string& type, int x, int y, int w, int h, TdfParser& parser);
	
	typedef vector<GUIgraph::Dataset> DatasetList;

	map<string,  DatasetList> statistics;

	vector<string> tableEntries;

	GUItable *stats;
	GUIgraph *graph;

	bool showDif;
	int lastSelection[2];
public:

	void PrivateDraw(void);

	void ShowPlayerStats(void);

	void ShowGraph(void);

};

#endif	// GUIENDGAMEDIALOG_H
