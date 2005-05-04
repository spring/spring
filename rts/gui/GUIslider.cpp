// GUIslider.cpp: implementation of the GUIslider class.
//
//////////////////////////////////////////////////////////////////////

#include "GUIslider.h"
#include "GUIfont.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


GUIslider::GUIslider(int x, int y, int width, int m, Functor2<GUIslider*, int>clicked):GUIframe(x,y,width-20, 10), click(clicked), maximum(m)
{
	position=0;
	name="scrollbar";
	downInControl=false;
}

GUIslider::~GUIslider()
{
}

static void Box(float x1, float y1, float w1, float h1)
{
	glBegin(GL_QUADS);
		glVertex3f(x1,y1,0);
		glVertex3f(x1,y1+h1,0);
		glVertex3f(x1+w1,y1+h1,0);
		glVertex3f(x1+w1,y1,0);
	glEnd();
}

void GUIslider::PrivateDraw()
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor3f(1, 1, 1);
	
	Box(0, 0, w, h);
	
	glColor3f(1, 0, 0);	
	if(position!=0)
		Box(0, 0, (w/(float)maximum)*position, h);
		
	glColor3f(1, 1, 1);

	char buf[20];
	sprintf(buf, "%i", position);
	guifont->Print(w+5, 0, buf);
	glColor3f(1, 1, 1);
}

bool GUIslider::MouseDownAction(int x, int y, int button)
{
	if(button==1&&LocalInside(x, y))
		downInControl=true;

	return MouseMoveAction(x, y, 0, 0, button);
}

bool GUIslider::MouseUpAction(int x, int y, int button)
{
	downInControl=false;
	return false;
}

bool GUIslider::MouseMoveAction(int x1, int y1, int xrel, int yrel, int button)
{
	if(downInControl)
	{
		SetPosition((x1/(float)w)*maximum);
		return true;
	}
	return false;
}

void GUIslider::SetPosition(int p)
{
	if(p<0)
		p=0;
	if(p>maximum)
		p=maximum;
		
	position=p;
	click(this, position);
}

void GUIslider::SetMaximum(int m)
{
	maximum=m;
	
	if(position>maximum)
	{
		position=maximum;
		click(this, position);
	}
}
