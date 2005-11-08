#ifndef _GLFONT_H
#define _GLFONT_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include "myGL.h"

class CglFont
{
public:
	void glPrint(const char* fmt, ...);
	void glPrintColor(const char* fmt, ...);
	void glWorldPrint(const char* fmt, ...);
	void glPrintAt(GLfloat x, GLfloat y, float s, const char* fmt, ...);
	CglFont(int start, int num);
	~CglFont();
	int *charWidths;
private:
	void printstring(const char *text,int *sh = NULL);
	void init_chartex(FT_Face face, char ch, GLuint base, GLuint* texbase);
	void WorldChar(char c);
	const char* GetFTError (FT_Error e);
	int chars;
	int charstart;
	int charheight;
	GLuint *textures;
	GLuint listbase;
	FT_Face face;
	FT_Library library;
};
extern CglFont* font;

#endif /* _GLFONT_H */
