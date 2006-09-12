#include "GUIpane.h"

GUIpane::GUIpane(int x, int y, int w, int h) : GUIframe(x, y, w, h)
{
	displayList=glGenLists(1);

	dragging=false;
	resizing=false;
	canDrag=false;
	canResize=false;
	drawFrame=true;
	downInside=false;

	minW=50;
	minH=50;
	
	BuildList();
}

GUIpane::~GUIpane()
{
	glDeleteLists(displayList, 1);
}

void GUIpane::PrivateDraw()
{
	if(drawFrame)
		glCallList(displayList);
}

void GUIpane::SizeChanged()
{
	BuildList();
}

bool GUIpane::MouseDownAction(int x, int y, int button)
{
	if(!LocalInside(x, y)) 
		return false;
		
	downInside=true;

	parent->RaiseChild(this);

	if(!resizing&&!dragging&&button==2)
	{
		if((x>w-35)&&(y>h-35)&&canResize)
			resizing=true;
		else if(canDrag)
			dragging=true;
	}
	return true;
}

bool GUIpane::MouseMoveAction(int x, int y, int xrel, int yrel, int buttons)
{
	if(buttons==2)
	{
		if(resizing)
		{
			w+=xrel;
			h+=yrel;
			if(w<minW) w=minW;
			if(h<minH) h=minH;
			SizeChanged();
			return true;
		}
		else if(dragging)
		{
			this->x+=xrel;
			this->y+=yrel;
			return true;
		}
	}
	// no buttons pressed or neither dragging nor resizing:
	// this event doesn't belong to us
	dragging=false;
	resizing=false;
	return false;
}

bool GUIpane::MouseUpAction(int x, int y, int button)
{
	bool res=dragging|resizing|downInside;
	dragging=false;
	resizing=false;
	downInside=false;
	return res;
}


void GUIpane::BuildList()
{
	static GLuint tex=0;

	if(!tex)
		tex=Texture("bitmaps/pane.bmp");

	glNewList(displayList, GL_COMPILE);
	
	glBindTexture(GL_TEXTURE_2D, tex);
	
	glPushAttrib(GL_CURRENT_BIT);
	
	glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
	
	DrawThemeRect(20, 128, w, h);
	
	glPopAttrib();

	glEndList();
}
