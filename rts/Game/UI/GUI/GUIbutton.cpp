// GUIbutton.cpp: implementation of the GUIbutton class.
//
//////////////////////////////////////////////////////////////////////

#include "GUIbutton.h"
#include "GUIfont.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

GUIbutton::GUIbutton(int x, int y, const string& title, Functor1<GUIbutton*> callback):GUIframe(x,y,w,20), click(callback)
{
	displayList=glGenLists(1);
	
	isPressed=false;
	isInside=false;
	autosizing=true;
	centerText=true;
	highlight=false;

	SetCaption(title);
}

GUIbutton::~GUIbutton()
{
	glDeleteLists(displayList,1);
}

void GUIbutton::SetSize(int w1, int h1)
{
	if(w1==0)
		autosizing=true;
	else
		autosizing=false;
	h=max(20, h1);
	
	if(autosizing)
	{
		w=(int)(guifont->GetWidth(caption.c_str()))+14;
	}
	else
		w=w1;
	BuildList();
}

void GUIbutton::SetCaption(const string& capt)
{
	caption=capt;
	
	if(autosizing)
	{
		w=(int)(guifont->GetWidth(caption.c_str()))+14;
	}
	
	BuildList();
}

void GUIbutton::PrivateDraw()
{
	// this saves the color buffer bit, which includes alpha blending data
	glPushAttrib(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
	glEnable( GL_ALPHA_TEST );
	glAlphaFunc ( GL_GREATER, 0.05f );

	if(isInside)
		glColor4f(0.7f,0.7f,0.7f,1.0f);
	else
		glColor4f(1.0f,1.0f,1.0f,1.0f);

	glCallList(displayList);

	glPopAttrib();
}


bool GUIbutton::MouseDownAction(int x, int y, int button)
{
	if(LocalInside(x, y)&&button==1)
	{
		isPressed=true;
		isInside=true;
	}
	return isPressed;
}

bool GUIbutton::MouseMoveAction(int x, int y, int xrel, int yrel, int button)
{
	if(isPressed)
	{
		if(LocalInside(x, y))
			isInside=true;
		else
			isInside=false;
	}
	return isPressed;
}

bool GUIbutton::MouseUpAction(int x, int y, int button)
{
	if(isPressed)
	{
		if(LocalInside(x, y))
			Action();
		isPressed=false;
		isInside=false;
		return true;
	}
	return false;
}

void GUIbutton::BuildList()
{
	static GLuint tex=0;
	if(!tex)
	{
		vector < paletteentry_s > vTransparentColors;
		paletteentry_s peBlack;
		peBlack.peBlue = peBlack.peGreen = peBlack.peRed = 0;
		peBlack.peFlags = 0;
		vTransparentColors.push_back( peBlack );
		tex=Texture("bitmaps/button.bmp", &vTransparentColors);
	}

	glNewList(displayList, GL_COMPILE);

	glBindTexture(GL_TEXTURE_2D, tex);

	DrawThemeRect(10, 32, w, h);
	
	if(highlight)
		glColor4f(1.0f,0.4f,0.4f,1.0f);
		
	if(centerText)
		guifont->output((w-guifont->GetWidth(caption.c_str()))/2.0f, (h-guifont->GetHeight())/2.0f, caption.c_str());
	else
		guifont->output(7, (h-guifont->GetHeight())/2.0f, caption.c_str());

	glEndList();
}

