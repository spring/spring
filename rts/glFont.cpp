#include "StdAfx.h"
// glFont.cpp: implementation of the CglFont class.
//
//////////////////////////////////////////////////////////////////////

#include "glFont.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdexcept>
#include "Camera.h"
//#include "mmgr.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CglFont* font;

#define DRAW_SIZE 1
#define FONTSIZE 20
#define FONTFILE "Vera.ttf"

/*
 * Bit twiddle to round up to
 * the next power of 2
 */
static inline int pow2(int a)
{
	int v = a;
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

CglFont::CglFont(int start, int num)
{
	chars = num-start;
	charstart = start;
	charWidths = new int[chars];
	textures = new GLuint[chars];
	if (FT_Init_FreeType(&library))
		throw std::runtime_error("FT_Init_FreeType failed");
	if (FT_New_Face(library,FONTFILE,0,&face))
		throw std::runtime_error("FT_New_Face failed");
	FT_Set_Char_Size(face, FONTSIZE << 6, FONTSIZE << 6, 96,96);
	listbase = glGenLists(chars);
	glGenTextures(chars,textures);
	for (unsigned char i = start; i < num; i++)
		init_chartex(face,i,listbase,textures);
}

CglFont::~CglFont()
{
	FT_Done_Face(face);
	FT_Done_FreeType(library);
	glDeleteLists(listbase,chars);
	glDeleteTextures(chars,textures);
	delete [] textures;
	delete [] charWidths;
}

void CglFont::init_chartex(FT_Face face, char ch, GLuint base, GLuint* texbase)
{
	FT_GlyphSlot slot = face->glyph;
	if (FT_Load_Char(face,ch,FT_LOAD_RENDER))
		throw std::runtime_error("FT_Load_Char failed");
	int width = pow2(slot->bitmap.width);
	charWidths[ch-charstart] = width;
	int height = pow2(slot->bitmap.rows);
	charheight = height;
	GLubyte* expdata = new GLubyte[2*width*height];
	for(int j=0; j <height;j++) {
		for(int i=0; i < width; i++) {
			expdata[2*(i+j*width)] = expdata[2*(i+j*width)+1] = (i>=slot->bitmap.width || j>=slot->bitmap.rows || !slot->bitmap.buffer[i+slot->bitmap.width*j]) ? 0 : slot->bitmap.buffer[i+slot->bitmap.width*j];
		}
	}
	glBindTexture(GL_TEXTURE_2D,texbase[ch]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, expdata);
	delete [] expdata;
	glNewList(base+ch,GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D,texbase[ch]);
	glTranslatef(slot->bitmap_left,0,0);
	glPushMatrix();
	glTranslatef(0,slot->bitmap_top-slot->bitmap.rows,0);
	float x=(float)slot->bitmap.width / (float)width;
	float y=(float)slot->bitmap.rows / (float)height;
	glBegin(GL_QUADS);
		glTexCoord2d(0,0); glVertex2f(0,slot->bitmap.rows);
		glTexCoord2d(0,y); glVertex2f(0,0);
		glTexCoord2d(x,y); glVertex2f(slot->bitmap.width,0);
		glTexCoord2d(x,0); glVertex2f(slot->bitmap.width,slot->bitmap.rows);
	glEnd();
	glPopMatrix();
	glTranslatef(slot->advance.x >> 6,0,0);
	glBitmap(0,0,0,0,slot->advance.x >> 6,0,NULL);
	glEndList();
}

void CglFont::printstring(const char *text)
{
	glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT  | GL_ENABLE_BIT | GL_TRANSFORM_BIT);	
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	
	float modelviewmatrix[16];	
	glGetFloatv(GL_MODELVIEW_MATRIX, modelviewmatrix);
	glPushMatrix();
	glMultMatrixf(modelviewmatrix);
	int previous = 0;
	for (int i = 0; i < strlen(text); i++) {
		FT_Vector delta;
		FT_Get_Kerning(face,FT_Get_Char_Index(face,previous),FT_Get_Char_Index(face,text[i]),FT_KERNING_UNSCALED,&delta);
		glTranslatef(delta.x>>6,0,0);
		glCallList(text[i]+listbase);
		previous = text[i];
	}
	glPopMatrix();
	glPopAttrib();		
}

void CglFont::glPrint(const char *fmt, ...)
{
	char text[512];
	va_list	ap;
	if (fmt == NULL)
		return;
	va_start(ap, fmt);
	vsprintf(text, fmt, ap);
	va_end(ap);
	glPushMatrix();
	printstring(text);
	glPopMatrix();
}

void CglFont::glPrintColor(const char* fmt, ...)
{
	char text[512];
	va_list ap;
	if (fmt == NULL)
		return;
	va_start(ap, fmt);
	vsprintf(text, fmt, ap);
	va_end(ap);
	glPushAttrib(GL_COLOR_BUFFER_BIT);
	glPushMatrix();
	glColor3f(1, 1, 1);
	size_t lf;
	string temp=text;
	while((lf=temp.find("\xff"))!=string::npos) {
		printstring(temp.substr(0, lf).c_str());
		temp=temp.substr(lf, string::npos);
		float r=((unsigned char)temp[1])/255.0;
		float g=((unsigned char)temp[2])/255.0;
		float b=((unsigned char)temp[3])/255.0;
		glColor3f(r, g, b);
		temp=temp.substr(4, string::npos);
	}
	printstring(temp.c_str());
	glPopMatrix();
	glPopAttrib();
}

void CglFont::glWorldPrint(const char *fmt, ...)
{
	char text[512];
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
	for (int a = 0; a < b; a++)
		WorldChar(text[a]);
	glPopMatrix();
}

void CglFont::glPrintAt(GLfloat x, GLfloat y, float s, const char *fmt, ...)
{
	char text[512];
	va_list	ap;
	if (fmt == NULL)
		return;
	va_start(ap, fmt);
	vsprintf(text, fmt, ap);
	va_end(ap);
	glPushMatrix();
	glTranslatef(x,y,0.0f);
	glScalef(.02f*s, .03f*s, .01f);
	printstring(text);
	glPopMatrix();
	glLoadIdentity();
}

void CglFont::WorldChar(char c)
{
	float charpart=(charWidths[c])/32.0f;
	glBindTexture(GL_TEXTURE_2D, textures[c-charstart]);
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
}

