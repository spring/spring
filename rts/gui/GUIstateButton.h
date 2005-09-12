// GUIstateButton.h: interface for the GUIstateButton class.
//
//////////////////////////////////////////////////////////////////////

#ifndef GUISTATEBUTTON_H
#define GUISTATEBUTTON_H

#include "GUIbutton.h"
#include <vector>

using namespace std;

class GUIstateButton : public GUIbutton
{
public:
	GUIstateButton(int x, int y, int w, std::vector<string>& states, Functor2<GUIstateButton*, int> callback);
	virtual ~GUIstateButton();
	
	void SetState(int state);
	void SetStates(const vector<string>& states);
	
	int State() { return state; }
	

protected:
	void PrivateDraw();
	void Action();
	void BuildList();

	Functor2<GUIstateButton*, int> click;
	vector<string> states;
	int state;
	float color[4];
};

#endif // GUISTATEBUTTON_H
