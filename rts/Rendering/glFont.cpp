#include "StdAfx.h"
// glFont.cpp: implementation of the CglFont class.
//
//////////////////////////////////////////////////////////////////////

#include "glFont.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdexcept>
#include "Game/Camera.h"
#include "myMath.h"
#include "Game/UI/InfoConsole.h"
#include "Platform/FileSystem.h"
#include "mmgr.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CglFont* font;

typedef struct 
{
	int bitmap_width;
	int bitmap_rows;
	FT_Pos ybearing;
	FT_Pos xbearing;
	FT_Pos advance_x;
	FT_Pos height;
	unsigned char * bitmap_buffer;
	int bitmap_pitch; 
} StoredGlyph;


#define DRAW_SIZE 1
#define FONTSIZE 20
#define FONTFILE "Luxi.ttf"

CglFont::CglFont(int start, int end)
{
	FT_Library library;
	FT_Face face;

	FT_Error error=FT_Init_FreeType(&library);
	if (error) {
		string msg="FT_Init_FreeType failed:";
		msg += GetFTError(error);
		throw std::runtime_error(msg);
	}

	bool success = false;
	std::vector<std::string> filenames = filesystem.GetNativeFilenames(FONTFILE);
	for (std::vector<std::string>::iterator it = filenames.begin(); it != filenames.end(); ++it) {
		if (!FT_New_Face(library, it->c_str(), 0, &face)) {
			success = true;
			break;
		}
	}
	if (!success) {
		string msg = FONTFILE ": FT_New_Face failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
	}

	FT_Set_Char_Size(face, FONTSIZE << 6, FONTSIZE << 6, 96,96);

	chars = end-start;

	charstart = start;
	charWidths = new int[chars];
	charHeights = new int[chars];
	textures = new GLuint[chars];

	listbase = glGenLists(chars);
	glGenTextures(chars,textures);
	
	StoredGlyph * glyphs = new StoredGlyph[chars];

	int maxabove = 0;
	int maxbelow = 0;
	int maxleft = 0;
	int maxright = 0;
	
	for (unsigned char i = start; i < end; i++) {
		StoredGlyph * g = &(glyphs[i-charstart]);
	
		// retrieve glyph index from character code
		FT_UInt glyph_index = FT_Get_Char_Index(face, i);
		
		error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
		if ( error ) {
			string msg = "FT_Load_Glyph failed:";
			msg += GetFTError(error);
			throw std::runtime_error(msg);
		}
		// convert to an anti-aliased bitmap
		error = FT_Render_Glyph( face->glyph, ft_render_mode_normal);
		if ( error ) {
			string msg = "FT_Render_Glyph failed:";
			msg += GetFTError(error);
			throw std::runtime_error(msg);
		}
		FT_GlyphSlot slot = face->glyph;
		
		g->bitmap_width = slot->bitmap.width;
		g->bitmap_rows = slot->bitmap.rows;
		g->bitmap_pitch = slot->bitmap.pitch;
		// Keep sign!
		g->ybearing = slot->metrics.horiBearingY / 64;	
		g->xbearing = slot->metrics.horiBearingX / 64;
		g->advance_x = slot->advance.x;
		g->height = slot->metrics.height;
		
		g->bitmap_buffer = new unsigned char[slot->bitmap.rows * slot->bitmap.width];
		memcpy(g->bitmap_buffer, 
			slot->bitmap.buffer, 
			slot->bitmap.width * slot->bitmap.rows);
		
		/* Vertical bearing. */
		
		int above = 0;
		int below = 0;
		
		if (g->ybearing >= 0) {
			above = g->ybearing;
			if (g->bitmap_rows > g->ybearing) {
				below = g->bitmap_rows - g->ybearing;
			}
			/* Otherwise, it does not extend below the line. */
		} else {
			/* If the bearing defines the top, then the character 
			   does not extend above the baseline. */
		
			below = abs(g->ybearing) + g->bitmap_rows;
		}
		
		maxabove = max(above, maxabove);
		maxbelow = max(below, maxbelow);
				
		/* Horizontal bearing. */
		
		int left = 0;
		int right = 0;
		
		if (g->xbearing >= 0) {
			right = g->xbearing + g->bitmap_width;
		} else {
			left = abs(g->xbearing);
			/* If the character extends back across the origin's
			   Y axis, then there's also a right component. */
			if (g->bitmap_width > left) {
				right = g->bitmap_width - left;
			}
		}
		
		maxleft = max(left, maxleft);
		maxright = max(right, maxright);
	}
	
	int texsize = max(maxleft + maxright, maxabove + maxbelow);
	/* Make texture sizes a power of two */
	int p2 = 1;
	while (p2 < texsize) p2 <<= 1;
	texsize = p2;

	/* Now copy each bitmap into a texture of the same size, 
	   but position them so the bottom of the e, g and k all
	   start on the same pixel, so they line up properly at
	   the right size. */
	
	// variable sized arrays on stack is a GCC-specific feature unfortunately...
	unsigned char *tex = new unsigned char[texsize*texsize*2];
	
	for (unsigned char ch = start; ch < end; ch++) {
		StoredGlyph * g = &(glyphs[ch-charstart]);
		
		/* Copy the glyph into position. */
						
		signed int move_down = maxabove - g->ybearing;
		signed int move_right = g->xbearing;
			
		for (int y = 0; y < texsize; y++) {
			for (int x = 0; x < texsize; x++) {
				unsigned char pel_val;
				
				int buf_y = y - move_down;
				int buf_x = x - move_right;
				
				if (buf_x < 0)
					pel_val = 0;
				else if (buf_x >= g->bitmap_width)
					pel_val = 0;
				else if (buf_y < 0)
					pel_val = 0;
				else if (buf_y >= g->bitmap_rows)
					pel_val = 0;
				else
					pel_val = g->bitmap_buffer[g->bitmap_pitch*buf_y+buf_x];
				
				
				tex[(y*texsize+x)*2+0] = 255;
				tex[(y*texsize+x)*2+1] = pel_val;
			}
		}
		
		/* Upload the glyph's bitmap. */
		
		glBindTexture(GL_TEXTURE_2D,textures[ch-charstart]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 
			0, GL_RGBA, 
			texsize, texsize, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, tex);
		
		/* Upload drawing instructions for the glyph. */
		
		glNewList(listbase+ch,GL_COMPILE);
		glBindTexture(GL_TEXTURE_2D,textures[ch-charstart]);
		charWidths[ch-charstart] = g->advance_x / 2 / texsize;		
		charHeights[ch-charstart] = g->height / 2 / texsize;		

		float x = (charWidths[ch-charstart]) / 32.0f;
		const float y = 1 - 1.0 / 64;
		glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2f(0, y); glVertex3f(0, 0, 0);
			glTexCoord2f(0, 0); glVertex3f(0, DRAW_SIZE, 0);
			glTexCoord2f(x, y); glVertex3f(DRAW_SIZE * x, 0, 0);
			glTexCoord2f(x, 0); glVertex3f(DRAW_SIZE * x, DRAW_SIZE, 0);
		glEnd();
		glTranslatef(DRAW_SIZE * (x + 0.02f), 0, 0);
		glEndList();		
		
		delete[] g->bitmap_buffer;
		g->bitmap_buffer = NULL;
	}

	delete[] tex;
	
	delete[] glyphs;
	FT_Done_Face(face);
	FT_Done_FreeType(library);
}

CglFont::~CglFont()
{
	glDeleteLists(listbase,chars);
	glDeleteTextures(chars,textures);
	delete [] textures;
	delete [] charWidths;
	delete [] charHeights;
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
	for (int i = 0; i < strlen(text); i++) {
		char ch = text[i];
		if (text[i] >= charstart && text[i] <= charstart + chars)
		{
			glCallList(text[i]+listbase);
		}
	}
	glPopAttrib();
}

void CglFont::glPrint(const char *fmt, ...)
{
	char text[512];
	va_list	ap;
	if (fmt == NULL)
		return;
	va_start(ap, fmt);
	VSNPRINTF(text, sizeof(text), fmt, ap);
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
	VSNPRINTF(text, sizeof(text), fmt, ap);
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

float CglFont::CalcCharWidth (char c)
{
	if(c < charstart || c >= chars + charstart)
		return 0.03 + charWidths[charstart]/32.0f;
	return 0.03 + charWidths[c-charstart]/32.0f;
}

float CglFont::CalcTextWidth (const char *text)
{
	float w=0.0f;
	for (int a=0;text[a];a++)  {
		float charpart = (charWidths[text[a]-charstart])/32.0f;
		w += charpart+0.03f;
	}
	return w;
}

float CglFont::CalcTextHeight(const char *text)
{
	float h=0.0f;
	for (int a=0;text[a];a++)  {
		float charpart = (charHeights[text[a]-charstart])/32.0f;
		if (charpart>h)
		{
			h = charpart;
		}
		;
	}
	return h+0.03f;
}

void CglFont::glWorldPrint(const char *fmt, ...)
{
	char text[512];
	va_list	ap;
	if (fmt == NULL)
		return;
	va_start(ap, fmt);
	VSNPRINTF(text, sizeof(text), fmt, ap);
	va_end(ap);
	glPushMatrix();
	glRasterPos2i(0,0);
	float w=CalcTextWidth (text);
	/* Center (screen-wise) the text above the current position. */
	glTranslatef(-0.5f*DRAW_SIZE*w*camera->right.x,-0.5f*DRAW_SIZE*w*camera->right.y,-0.5f*DRAW_SIZE*w*camera->right.z);
	for (int a = 0; text[a]; a++)
		WorldChar(text[a]);
	glPopMatrix();
}

// centered version of glPrintAt
void CglFont::glPrintCentered (float x, float y, float s, const char *fmt, ...)
{
	char text[512];
	va_list	ap;
	if (fmt == NULL)
		return;
	va_start(ap, fmt);
	VSNPRINTF(text, sizeof(text), fmt, ap);
	va_end(ap);

	glPushMatrix();
	glTranslatef(x,y,0.0f);
	glScalef(0.02f*s,0.025f*s,1.0f);
	glTranslatef(-0.5f*font->CalcTextWidth(text),0.0f,0.0f);
	printstring(text);
	glPopMatrix();
}

// right justified version of glPrintAt
void CglFont::glPrintRight (float x, float y, float s, const char *fmt, ...)
{
	char text[512];
	va_list	ap;
	if (fmt == NULL)
		return;
	va_start(ap, fmt);
	VSNPRINTF(text, sizeof(text), fmt, ap);
	va_end(ap);

	glPushMatrix();
	glTranslatef(x,y,0.0f);
	glScalef(0.02f*s,0.025f*s,1.0f);
	glTranslatef(-font->CalcTextWidth(text),0.0f,0.0f);
	printstring(text);
	glPopMatrix();
}

void CglFont::glPrintAt(GLfloat x, GLfloat y, float s, const char *fmt, ...)
{
	char text[512];
	va_list	ap;
	if (fmt == NULL)
		return;
	va_start(ap, fmt);
	VSNPRINTF(text, sizeof(text), fmt, ap);
	va_end(ap);
	glPushMatrix();
	glTranslatef(x, y, 0.0f);
	glScalef(.02f * s, .03f * s, .01f);
	printstring(text);
	glPopMatrix();
}

void CglFont::WorldChar(char c)
{
	float charpart=(charWidths[c-charstart])/32.0f;
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



#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, 0 } };
  struct ErrorString
  {
	int          err_code;
	const char*  err_msg;
  } static errorTable[] =
#include FT_ERRORS_H


const char* CglFont::GetFTError (FT_Error e)
{
	for (int a=0;errorTable[a].err_msg;a++) {
		if (errorTable[a].err_code == e)
			return errorTable[a].err_msg;
	}
	return "Unknown error";
}
