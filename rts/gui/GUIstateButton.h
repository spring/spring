#ifndef GUISTATEBUTTON_H
#define GUISTATEBUTTON_H
// GUIstateButton.h: interface for the GUIstateButton class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GUISTATEBUTTON_H__8E295471_26AB_4630_90D9_560AFC43C36C__INCLUDED_)
#define AFX_GUISTATEBUTTON_H__8E295471_26AB_4630_90D9_560AFC43C36C__INCLUDED_

#if _MSC_VER >= 1000
/*pragma once removed*/
#endif // _MSC_VER >= 1000

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

#endif // !defined(AFX_GUISTATEBUTTON_H__8E295471_26AB_4630_90D9_560AFC43C36C__INCLUDED_)

#endif /* GUISTATEBUTTON_H */
