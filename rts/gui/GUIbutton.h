// GUIbutton.h: interface for the GUIbutton class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GUIBUTTON_H__4BE3DE92_730E_4CCF_B1C6_4E879920FE8F__INCLUDED_)
#define AFX_GUIBUTTON_H__4BE3DE92_730E_4CCF_B1C6_4E879920FE8F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GUIframe.h"
#include "Functor.h"

#include <string>

class GUIbutton;

class GUIbutton : public GUIframe, public GUIcaption
{
public:
	GUIbutton(int x, int y, const string& title, Functor1<GUIbutton*> callback);
	virtual ~GUIbutton();
	
	void SetCaption(const string& caption);
	
	void SetSize(int w, int h);
	
	void SetHighlight(bool hi) { highlight=hi; BuildList(); }
	bool Highlight() { return highlight; }
protected:
	void PrivateDraw();
	bool MouseUpAction(int x, int y, int button);
	bool MouseDownAction(int x, int y, int button);
	bool MouseMoveAction(int x, int y, int xrel, int yrel, int button);
	std::string caption;
	
	GLuint displayList;
	bool isPressed;
	bool isInside;
	bool autosizing;
	bool centerText;
	bool highlight;

	virtual void Action()
	{
		click(this);
	}
	
	virtual void BuildList();

private:
	Functor1<GUIbutton*> click;
};

#endif // !defined(AFX_GUIBUTTON_H__4BE3DE92_730E_4CCF_B1C6_4E879920FE8F__INCLUDED_)
