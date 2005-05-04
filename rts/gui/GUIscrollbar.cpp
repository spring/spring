// GUIscrollbar.cpp: implementation of the GUIscrollbar class.

//

//////////////////////////////////////////////////////////////////////



#include "GUIscrollbar.h"



//////////////////////////////////////////////////////////////////////

// Construction/Destruction

//////////////////////////////////////////////////////////////////////



#define SB_WIDTH 16



GUIscrollbar::GUIscrollbar(int x, int y, int h, int m, Functor1<int>clicked):GUIframe(x,y,SB_WIDTH,h), click(clicked), maximum(m)

{

	position=0;

	name="scrollbar";

	downInControl=false;

	

	static GLuint tex=0;

	if(!tex)

		tex=Texture("bitmaps/scrollbar.bmp");

	glBindTexture(GL_TEXTURE_2D, tex);

	

	

	displayList=glGenLists(1);



	glNewList(displayList, GL_COMPILE);





	const double t=SB_WIDTH;

	const double s=64;

	

	#define coordx(a) ((double)(a)/(double)16)

	#define coordy(a) ((double)(a)/(double)64)

	

	glBindTexture(GL_TEXTURE_2D, tex);





	glBegin(GL_QUADS);

	// #

	// .

	// .

		glTexCoord2d(0.0f, 0.0f);

		glVertex3f(0, 0, 0);

		glTexCoord2d(0.0f, coordy(t));

		glVertex3f(0, t, 0);

		glTexCoord2d(coordx(t), coordy(t));

		glVertex3d(t, t, 0);					

		glTexCoord2d(coordx(t), 0.0f);

		glVertex3f(t, 0, 0);



	// .

	// .

	// #

		glTexCoord2d(0.0f, coordy(s-t));

		glVertex3f(0, h-t, 0);

		glTexCoord2d(0.0f, coordy(s));

		glVertex3f(0, h, 0);

		glTexCoord2d(coordx(t), coordy(s));

		glVertex3d(t, h, 0);					

		glTexCoord2d(coordx(t), coordy(s-t));

		glVertex3f(t, h-t, 0);



	// .

	// #

	// .

		glTexCoord2d(0.0f, coordy(t));

		glVertex3f(0, t, 0);

		glTexCoord2d(0.0f, coordy(s-t));

		glVertex3f(0, h-t, 0);

		glTexCoord2d(coordx(t), coordy(s-t));

		glVertex3d(t, h-t, 0);

		glTexCoord2d(coordx(t), coordy(t));

		glVertex3f(t, t, 0);

	glEnd();



	glEndList();

	

}



GUIscrollbar::~GUIscrollbar()

{

	glDeleteLists(displayList,1);

}



static void Box(float x1, float y1, float w1, float h1)

{

	glBegin(GL_QUADS);

		glVertex3f(x1,y1,0);

		glVertex3f(x1,y1+h1,0);

		glVertex3f(x1+w1,y1+h1,0);

		glVertex3f(x1+w1,y1,0);

	glEnd();

	

	glColor4f(1.0,1.0,1.0,GUI_TRANS);

	glBegin(GL_LINE_LOOP);

		glVertex3f(x1,y1,0);

		glVertex3f(x1,y1+h1,0);

		glVertex3f(x1+w1,y1+h1,0);

		glVertex3f(x1+w1,y1,0);

	glEnd();

}



void GUIscrollbar::PrivateDraw()

{

	#define KNOB_SIZE 11

	glCallList(displayList);

	glDisable(GL_TEXTURE_2D);

	int knobY=((h-(KNOB_SIZE*2)-KNOB_SIZE)/(float)maximum)*position+KNOB_SIZE;



	glColor4f(1, 1, 1, 1);

	Box(2, knobY+1, 12, 9);

	glEnable(GL_TEXTURE_2D);

	glColor4f(1.0,1.0,1.0,1.0f);

}



bool GUIscrollbar::MouseDownAction(int x, int y, int button)

{

	if(LocalInside(x, y))

		downInControl=true;

	return MouseMoveAction(x, y, 0, 0, button);

}



bool GUIscrollbar::MouseUpAction(int x, int y, int button)

{

	downInControl=false;

	return false;

}







bool GUIscrollbar::MouseMoveAction(int x1, int y1, int xrel, int yrel, int button)

{

	if(button==1&&LocalInside(x1, y1)&&downInControl)

	{

		if(y1<KNOB_SIZE)

			position=max(position-1, 0);

		else if(y1>h-KNOB_SIZE)

			position=min(position+1, maximum);

		else

		{

			position=((float)(y1-KNOB_SIZE)/(h-(KNOB_SIZE*2)))*maximum+0.5;

		}



		click(position);

		return true;

	}

	return false;

}



void GUIscrollbar::SetPosition(int p)

{

	if(p>0)

	{

		position=p;

		click(position);

	}

}

