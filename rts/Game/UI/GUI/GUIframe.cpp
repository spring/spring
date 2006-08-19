// GUIframe.cpp: implementation of the GUIframe class.
//
//////////////////////////////////////////////////////////////////////

#include "GUIframe.h"
#include <algorithm>
//#include "Game/UI/InfoConsole.h"

GUIframe *mainFrame=NULL;

using namespace std;

GUIresponder::~GUIresponder() {
}
GUIdrawer::~GUIdrawer() {
}
GUIcaption::~GUIcaption() {
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

float GUIframe::s_fGuiOpacity = 0.8f; // static

GUIframe::GUIframe(const int x1, const int y1, const int w1, const int h1):x(x1), y(y1), w(w1), h(h1), parent() 
{
	isMainFrame=false;
	name="frame";
	inputFocus=NULL;
	hidden=false;
}

GUIframe::GUIframe():x(0), y(0), w(500), h(500), parent() 
{
	isMainFrame=true;
	name="frame";
	inputFocus=NULL;
	hidden=false;
}

GUIframe::~GUIframe()
{
	while(children.size())
	{
		delete children.back();
		children.pop_back();
	}
}

void GUIframe::InitMainFrame()
{
	mainFrame=new GUIframe(0, 0, gu->screenx, gu->screeny);
}

void GUIframe::AddChild(GUIframe *baby)
{
	children.push_back(baby);
	baby->ChangeParent(this);
}

void GUIframe::AddSibling(GUIframe *sister)
{
	parent->AddChild(sister);
}

void GUIframe::Draw() 
{
	glTranslatef(x,y,0);

	PrivateDraw();

	std::list<GUIframe*>::iterator current;

	for(current=children.begin(); current!=children.end(); current++)
	{
		if(!(*current)->isHidden())
			(*current)->Draw();
	}

	glTranslatef(-x,-y,0);
}

GUIframe* GUIframe::ControlAtPos(int x1, int y1)
{
	// ControlAtPos expects to get the relative coordinates.
	GUIframe* res=NULL;
	list<GUIframe*>::iterator current;

	for(current=children.begin(); current!=children.end(); current++)
	{
		if(!(*current)->isHidden())
		{
			GUIframe *tmp=(*current)->ControlAtPos(x1-x,y1-y);
			if(tmp)
				res=tmp;
		}
	}
	if(!res)
	{
		if(IsInside(x1, y1))
			res=this;
	}
	return res;
}

bool GUIframe::MouseDown(int x1, int y1, int button)
{
	// MouseDown expects to get the relative coordinates.
	bool isResponsible=false;

	std::list<GUIframe*>::reverse_iterator current;
	for(current=children.rbegin(); current!=children.rend()&&!isResponsible; current++)
	{
		if(!(*current)->isHidden())
			isResponsible=(*current)->MouseDown(x1-x,y1-y, button);
	}
	if(!isResponsible)
	{
		isResponsible=MouseDownAction(x1-x, y1-y, button);
	}
	return isResponsible;
}

bool GUIframe::MouseMove(int x1, int y1, int xrel, int yrel, int buttons)
{
	// MouseMove expects to get the relative coordinates.
	bool isResponsible=false;
	list<GUIframe*>::reverse_iterator current;

	for(current=children.rbegin(); current!=children.rend(); current++)
	{
		if(!(*current)->isHidden())
		{
			isResponsible|=(*current)->MouseMove(x1-x,y1-y, xrel, yrel, buttons);
		}
	}
	if(!isResponsible)
	{
		isResponsible|=MouseMoveAction(x1-x,y1-y, xrel, yrel, buttons);
	}
	return isResponsible;
}

bool GUIframe::MouseUp(int x1, int y1, int button)
{
	// MouseUp expects to get the relative coordinates.
	bool isResponsible=false;

	list<GUIframe*>::reverse_iterator current;
	for(current=children.rbegin(); current!=children.rend()&&!isResponsible; current++)
	{
		if(!(*current)->isHidden())
			isResponsible=(*current)->MouseUp(x1-x,y1-y, button);
	}
	if(!isResponsible)
	{
		isResponsible=MouseUpAction(x1-x,y1-y, button);
	}
	return isResponsible;
}

bool GUIframe::Event(const std::string& event)
{
	bool isResponsible=false;

	list<GUIframe*>::reverse_iterator current;
	for(current=children.rbegin(); current!=children.rend()&&!isResponsible; current++)
	{
		if(!(*current)->isHidden())
			isResponsible=(*current)->Event(event);
	}
	if(!isResponsible)
	{
		isResponsible=EventAction(event);
	}
	return isResponsible;
}

bool GUIframe::KeyDown(const int key)
{
	if(inputFocus)
	{
		if(inputFocus==this)
			return KeyAction(key);
		else
			return inputFocus->KeyDown(key);
	}
	return false;
}

bool GUIframe::Character(const char c)
{
	if(inputFocus)
	{
		if(inputFocus==this)
			return CharacterAction(c);
		else
			return inputFocus->Character(c);
	}
	return false;
}

void GUIframe::ChangeParent(GUIframe *mommy)
{
	parent=mommy;
}

void GUIframe::ToggleHidden()
{
	hidden=!hidden;
}

void GUIframe::PrivateDraw()
{
}

bool GUIframe::MouseDownAction(int x, int y, int button)
{
	return false;
}

bool GUIframe::MouseUpAction(int x, int y, int button)
{
	return false;
}

bool GUIframe::MouseMoveAction(int x, int y, int xrel, int yrel, int button)
{
	return false;
}

bool GUIframe::KeyAction(const int key)
{
	return false;
}

bool GUIframe::CharacterAction(const char c)
{
	return false;
}

bool GUIframe::EventAction(const std::string& event)
{
	if(event=="hide")
	{
		if(!isHidden())
			ToggleHidden();
		return true;
	}
	if(event=="show")
	{
		if(isHidden())
			ToggleHidden();
		Raise();
		return true;
	}
	if(event=="togglehidden")
	{
		ToggleHidden();
		return true;
	}
	
	return false;
}


void GUIframe::SizeChanged()
{	
}


void GUIframe::RemoveChild(GUIframe *item)
{
	children.remove(item);
}

void GUIframe::MakeChildFocus(GUIframe *child)
{
	if(parent)
		parent->MakeChildFocus(this);
	inputFocus = child;
}

void GUIframe::TakeFocus()
{
	inputFocus=this;
	if(parent)
		parent->MakeChildFocus(this);
}

void GUIframe::ReleaseFocus()
{
	inputFocus=NULL;
	if(parent)
		parent->MakeChildFocus(NULL);
}

bool GUIframe::HasFocus()
{
	// if a child has focus, we haven't
	if(inputFocus!=this) return false;

	// if the whole focus tree points to us, we have focus
	GUIframe *last=this;
	GUIframe *cur=parent;
	while(cur)
	{
		if(cur->inputFocus!=last)
			return false;
		last=cur;
		cur=cur->parent;
	}
	return true;
}

void GUIframe::RaiseChild(const GUIframe* child)
{
	list<GUIframe*>::iterator p=std::find(children.begin(), children.end(), child);
	if(p!=children.end())
	{
		GUIframe *temp=*p;
		children.erase(p);
		children.push_back(temp);
	}
}

bool GUIframe::isTopmost()
{
	if(!parent) return false;

	list<GUIframe*>::reverse_iterator i=parent->children.rbegin();
	for(;i!=children.rend()&&(*i)->isHidden();i++) {}

	if(*i==this) return true;
	return false;
}

string GUIframe::Tooltip()
{
	return tooltip;
}





#include "Rendering/Textures/Bitmap.h"


GLuint Texture(const std::string& name, const vector<paletteentry_s>* pvTransparentColors)
{
	CBitmap bitmap(name);
	if ( pvTransparentColors != NULL )
	{
		vector < paletteentry_s >::const_iterator currIter = pvTransparentColors->begin(), 
			eIter = pvTransparentColors->end();
		for ( ; currIter != eIter; currIter++ )
		{
			bitmap.SetTransparent( currIter->peRed, currIter->peGreen, currIter->peBlue );
		}
	}
	return bitmap.CreateTexture(false);
};


void DrawThemeRect(int edge, int size, int w, int h)
{
	const float t=edge;
	const float s=size;
	
	#define coordx(a) ((float)(a)/(float)s)
	#define coordy(a) ((float)(a)/(float)s)
	

	glBegin(GL_QUADS);
	// #..
	// ...
	// ...		
		glTexCoord2d(0.0f, 0.0f);
		glVertex3f(0, 0, 0);
		glTexCoord2d(0.0f, coordy(t));
		glVertex3f(0, t, 0);
		glTexCoord2d(coordx(t), coordy(t));
		glVertex3d(t, t, 0);					
		glTexCoord2d(coordx(t), 0.0f);
		glVertex3f(t, 0, 0);
	
	// ...
	// ...
	// #..
		glTexCoord2f(0.0f, coordy(s-t));
		glVertex3f(0, h-t, 0);
		glTexCoord2f(0.0f, coordy(s));
		glVertex3f(0, h, 0);		
		glTexCoord2f(coordx(t), coordy(s));
		glVertex3f(t, h, 0);
		glTexCoord2f(coordx(t), coordy(s-t));
		glVertex3f(t, h-t, 0);				

	
	// ..#
	// ...
	// ...;
		glTexCoord2f(coordx(s-t), 0.0f);
		glVertex3f(w-t, 0, 0);
		glTexCoord2f(coordx(s-t), coordy(t));
		glVertex3f(w-t, t, 0);		
		glTexCoord2f(coordx(s), coordy(t));
		glVertex3f(w, t, 0);
		glTexCoord2f(coordx(s), 0.0f);
		glVertex3f(w, 0, 0);							

	
	// ...
	// ...
	// ..#
		glTexCoord2f(coordx(s-t), coordy(s-t));
		glVertex3f(w-t, h-t, 0);
		glTexCoord2f(coordx(s-t), coordy(s));
		glVertex3f(w-t, h, 0);		
		glTexCoord2f(coordx(s), coordy(s));
		glVertex3f(w, h, 0);
		glTexCoord2f(coordx(s), coordy(s-t));
		glVertex3f(w, h-t, 0);				


	// .#.
	// ...
	// ...
		glTexCoord2f(coordx(t), 0.0f);
		glVertex3f(t, 0, 0);
		glTexCoord2f(coordx(t), coordy(t));
		glVertex3f(t, t, 0);		
		glTexCoord2f(coordx(s-t), coordy(t));
		glVertex3f(w-t, t, 0);
		glTexCoord2f(coordx(s-t), 0.0f);
		glVertex3f(w-t, 0, 0);
		
	// ...
	// ...
	// .#.
		glTexCoord2f(coordx(t), coordy(s-t));
		glVertex3f(t, h-t, 0);
		glTexCoord2f(coordx(t), coordy(s));
		glVertex3f(t, h, 0);		
		glTexCoord2f(coordx(s-t), coordy(s));
		glVertex3f(w-t, h, 0);
		glTexCoord2f(coordx(s-t), coordy(s-t));
		glVertex3f(w-t, h-t, 0);

	
	// ...
	// #..
	// ...
		glTexCoord2f(0, coordy(t));
		glVertex3f(0, t, 0);
		glTexCoord2f(0, coordy(s-t));
		glVertex3f(0, h-t, 0);		
		glTexCoord2f(coordx(t), coordy(s-t));
		glVertex3f(t, h-t, 0);
		glTexCoord2f(coordx(t), coordy(t));
		glVertex3f(t, t, 0);
		



	
	// ...
	// ..#
	// ...
		glTexCoord2f(coordx(s-t), coordy(t));
		glVertex3f(w-t, t, 0);
		glTexCoord2f(coordx(s-t), coordy(s-t));
		glVertex3f(w-t, h-t, 0);		
		glTexCoord2f(coordx(s), coordy(s-t));
		glVertex3f(w, h-t, 0);
		glTexCoord2f(coordx(s), coordy(t));
		glVertex3f(w, t, 0);
		
	// ...
	// .#.
	// ...
		glTexCoord2d(coordx(t), coordy(t));
		glVertex3f(t, t, 0);
		glTexCoord2f(coordx(t), coordy(s-t));
		glVertex3f(t, h-t, 0);
		glTexCoord2f(coordx(s-t), coordy(s-t));
		glVertex3f(w-t, h-t, 0);
		glTexCoord2f(coordx(s-t), coordy(t));
		glVertex3f(w-t, t, 0);

	glEnd();	


	#undef coordx
	#undef coordy	
}

