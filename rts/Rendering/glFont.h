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
	void glPrintRight (float x,float y, float s, const char *fmt,...);
	void glPrintCentered (float x,float y, float s, const char *fmt,...);
	void glPrintOutlined(const char* text, float shiftX, float shiftY,
	                     const float* normalColor, const float* outlineColor);
	float CalcTextWidth (const char *txt);
	float CalcCharWidth (char c);
	float CalcTextHeight(const char *text);

	CglFont(int start, int end, const char* fontfile);
	~CglFont();
	
	int GetCharStart() const { return charstart; }
	int GetCharEnd()   const { return charend; }

	void glPrintRaw(const char* text) {
		printstring((const unsigned char*)text);
	}
	
private:
	void printstring(const unsigned char *text);
	void WorldChar(char c);
	const char* GetFTError (FT_Error e);
	int chars;
	int charstart;
	int charend;
	int charheight;
	GLuint *textures;
	GLuint listbase;
	GLuint listbaseNoshift;
	int *charWidths;
	int *charHeights;
};
extern CglFont* font;

#endif /* _GLFONT_H */
