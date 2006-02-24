#ifndef _GLFONT_H
#define _GLFONT_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include "GL/myGL.h"

class CglFont
{
public:
	void glPrint(const char* fmt, ...);
	void glPrintColor(const char* fmt, ...);
	void glWorldPrint(const char* fmt, ...);
	void glPrintAt(GLfloat x, GLfloat y, float s, const char* fmt, ...);
	float CalcTextWidth (const char *txt);
	float CalcCharWidth (char c);
	void glPrintCentered (float x,float y, float s,const char *fmt,...);
	CglFont(int start, int end);
	~CglFont();
private:
	void printstring(const char *text);
	void WorldChar(char c);
	const char* GetFTError (FT_Error e);
	int chars;
	int charstart;
	int charheight;
	GLuint *textures;
	GLuint listbase;
	int *charWidths;
};
extern CglFont* font;

#endif /* _GLFONT_H */
