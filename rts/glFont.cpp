#include "StdAfx.h"
// glFont.cpp: implementation of the CglFont class.
//
//////////////////////////////////////////////////////////////////////

#include "glFont.h"
#include <stdio.h>
#include <stdarg.h>
#include "Camera.h"
#include <fstream>
#include "FileHandler.h"
//#include <ostream.h>
//#include <istream.h>
//#include "mmgr.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CglFont* font;

#define DRAW_SIZE 1

#define drawfont GLUT_BITMAP_HELVETICA_18

CglFont::CglFont(int start, int num)
{
	for (int i = start; i < num; i++)
		charWidths[i] = glutBitmapWidth(drawfont, i);
}



CglFont::~CglFont()
{
}

void CglFont::printstring(const char *str)
{
	char *p = (char*)str;
	while (*p != '\0')
		glutBitmapCharacter(drawfont, *p++);
}

void CglFont::glPrint(const char *fmt, ...)

{
	char text[256];
	va_list ap;
	if (fmt == NULL)
		return;
	va_start(ap, fmt);
	vsprintf(text, fmt, ap);
	va_end(ap);
	glPushMatrix();
	glRasterPos2i(0,0);
	printstring(text);
	glPopMatrix();
}


/*
 * Whose brilliant idea was it to embed the color in the string?
 */
void CglFont::glPrintColor(const char* fmt, ...)
{
	char text[256];
	va_list ap;
	if (fmt == NULL)
		return;
	va_start(ap, fmt);
	vsprintf(text, fmt, ap);
	va_end(ap);
	glPushMatrix();
	glRasterPos2i(0,0);
	char *p = (char*)text;
	char tmp[256];
	int i = 0;
	while (*p != '\0') {
		if (*p == '\xff') {
			tmp[i++] = '\0';
			printstring(tmp);
			for (int j = 0; j < i; j++)
				tmp[j] = '\0';
			i = 0;
			glColor3f(++*p/255.0f,++*p/255.0f,++*p/255.0f);
			*p++;
		}
		tmp[i++] = *p;
	}
	tmp[i] = '\0';
	printstring(tmp);
	glPopMatrix();
}

void CglFont::glWorldPrint(const char *fmt, ...)
{
	char text[256];
	va_list	ap;
	if (fmt == NULL)
		return;
	va_start(ap, fmt);
	vsprintf(text, fmt, ap);
	va_end(ap);
	glPushMatrix();
	glRasterPos2i(0,0);
	float charpart = (charWidths[text[0]])/32.0f;
	int b = strlen(text);
	glTranslatef(-b*0.5f*DRAW_SIZE*(charpart+0.03f)*camera->right.x,-b*0.5f*DRAW_SIZE*(charpart+0.03f)*camera->right.y,-b*0.5f*DRAW_SIZE*(charpart+0.03f)*camera->right.z);
#if 0
	for (int a = 0; a < b; a++)
		WorldChar(text[a]);
#else
	printstring(text);
#endif
	glPopMatrix();
}

void CglFont::glPrintAt(GLfloat x, GLfloat y, float s, const char *fmt, ...)
{
	char text[256];
	va_list	ap;
	if (fmt == NULL)
		return;
	va_start(ap, fmt);
	vsprintf(text, fmt, ap);
	va_end(ap);
	glPushMatrix();
	glTranslatef(x,y,0.0f);
	glScalef(.02f*s, .03f*s, .01f);
	glRasterPos2i(0,0);
	printstring(text);
	glPopMatrix();
	glLoadIdentity();
}

void CglFont::WorldChar(char c)
{
#if 0
	float charpart=(charWidths[c])/32.0f;
	glBindTexture(GL_TEXTURE_2D, ttextures[c-startchar]);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0,1);
		glVertex3f(0,0,0);
		glTexCoord2f(0,0);
		glVertex3f(DRAW_SIZE*camera->up.x,DRAW_SIZE*camera->up.y,DRAW_SIZE*camera->up.z);
		glTexCoord2f(charpart,1);
		glVertex3f(DRAW_SIZE*charpart*camera->right.x,DRAW_SIZE*charpart*camera->right.y,DRAW_SIZE*charpart*camera->right.z);
		glTexCoord2f(charpart,0);
		glVertex3f(DRAW_SIZE*(camera->up.x+camera->right.x*charpart),DRAW_SIZE*(camera->up.y+camera->right.y*charpart),DRAW_SIZE*(camera->up.z+camera->right.z*charpart));
	glEnd();
	glTranslatef(DRAW_SIZE*(charpart+0.03f)*camera->right.x,DRAW_SIZE*(charpart+0.03f)*camera->right.y,DRAW_SIZE*(charpart+0.03f)*camera->right.z);
#endif
}

