#ifndef _GLFONT_H
#define _GLFONT_H

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
	int charWidths[256];
private:
	void printstring(const char *str);
	void WorldChar(char c);
};
extern CglFont* font;

#endif /* _GLFONT_H */
