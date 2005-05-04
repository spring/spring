// GUIimage.cpp: implementation of the GUIimage class.
//
//////////////////////////////////////////////////////////////////////

#include "GUIimage.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

GUIimage::GUIimage(int x, int y, int w, int h, const std::string& image): GUIframe(x, y, w, h)
{
	SetImage(image);
}

GUIimage::~GUIimage()
{

}

void GUIimage::PrivateDraw()
{
	glBindTexture(GL_TEXTURE_2D, texture);

	glColor3f(1,1,1);

	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(0,0);
		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(0,h);
		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(w,h);
		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(w,0);
	glEnd();
}

void GUIimage::SetImage(const std::string& image)
{
	texture=Texture(image);
}
