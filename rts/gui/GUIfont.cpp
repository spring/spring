#include "GUIfont.h"



// FreeType Headers
#include <ft2build.h>
#include FT_FREETYPE_H

#include "myGL.h"

#include <string>
#include <stdexcept>

using namespace std;

#define TEXSIZE 256

GUIfont *guifont=NULL;

GUIfont::GUIfont(const std::string& fontFilename,int fontsize)
{
	int tex_x=0,tex_y=0;
	int tex[TEXSIZE*TEXSIZE];

	FT_Face face;
	FT_Library library;
	FT_GlyphSlot slot = face->glyph;

	memset(tex,0,TEXSIZE*TEXSIZE*4);
	charHeight=0;

	if(FT_Init_FreeType(&library))
		throw std::runtime_error("Failed to init freetype2 library");

	if(FT_New_Face(library, fontFilename.c_str(), 0, &face))
		throw std::runtime_error("Failed to load font "+ fontFilename);

	FT_Set_Char_Size(face,fontsize << 6,fontsize << 6,96,96);

	displaylist=glGenLists(256);

	for (unsigned int c=0; c<256; c++) 
	{
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			throw std::runtime_error("FT_Load_Char failed");

		int width  = slot->bitmap.width;
		int height = slot->bitmap.rows;
		if (height>charHeight) charHeight=height;

		if (tex_x+width>=256) 
		{
			tex_y += (int)charHeight+1;
			tex_x=0;
		}

		// render glyph bitmap to texture
		for (int y=0; y<slot->bitmap.rows; y++) 
		{
			for (int x=0; x<slot->bitmap.width; x++) 
			{
				char shade=slot->bitmap.buffer[x+y*slot->bitmap.pitch];
				int color=(shade << 24) | 0x00ffffff;
				tex[(tex_x+x)+(tex_y+y)*TEXSIZE] = color;
		
			}
		}

		charWidth[c]=slot->advance.x / (float)0x10000;

		// set up display list
		
		// tex coords
		float tx1=tex_x/(float)TEXSIZE;
		float ty1=tex_y/(float)TEXSIZE;
		float tx2=(tex_x+width)/(float)TEXSIZE;
		float ty2=(tex_y+height)/(float)TEXSIZE;

		// vertex coords
		float vx1=slot->bitmap_left;
		float vy1=-slot->bitmap_top;
		float vx2=slot->bitmap_left+width;
		float vy2=height-slot->bitmap_top;

		glNewList(displaylist+c,GL_COMPILE);

		glBegin(GL_QUADS);
			glTexCoord2f(tx1, ty1);
			glVertex2f(vx1, vy1);

			glTexCoord2f(tx1, ty2);			
			glVertex2f(vx1, vy2);

			glTexCoord2f(tx2, ty2);
			glVertex2f(vx2, vy2);

			glTexCoord2f(tx2, ty1);
			glVertex2f(vx2, vy1);
		glEnd();

		glTranslatef(charWidth[c],0,0);

		glEndList();

		tex_x += width+1;

		// discard glyph
		//FT_Done_Glyph(glyph);
	}
	// create texture
	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, TEXSIZE, TEXSIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);

	FT_Done_FreeType(library);
}


void GUIfont::Print(float x, float y, const char* fmt,...)
{
	char		text[512];								// Holds Our String

	va_list		ap;										// Pointer To List Of Arguments



	if (fmt == NULL)									// If There's No Text

		return;											// Do Nothing



	va_start(ap, fmt);									// Parses The String For Variables

	vsprintf(text, fmt, ap);						// And Converts Symbols To Actual Numbers

	va_end(ap);											// Results Are Stored In Text


	glPushMatrix();

	glTranslatef(x,y+charHeight*0.8f,0);

	Print(text);
	glPopMatrix();
}

void GUIfont::Print(float x, float y, const string& text)
{
	glPushMatrix();

	glTranslatef(x,y+charHeight*0.8f,0);

	Print(text);
	glPopMatrix();
}

void GUIfont::Print(float x, float y, float size, const string& text)
{
	glPushMatrix();

	glTranslatef(x,y+charHeight*0.8f,0);
	
	glScalef(size, size, size);

	Print(text);
	glPopMatrix();
}

void GUIfont::PrintColor(float x, float y, const string& text)
{
	glPushMatrix();

	glTranslatef(x,y+charHeight*0.8f,0);

	PrintColor(text);
	glPopMatrix();
}


void GUIfont::Print(const string& text)
{
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_LIST_BIT);

	glListBase(displaylist);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, texture);

	glCallLists(text.size(),GL_UNSIGNED_BYTE,text.c_str());

	glPopAttrib();
}

void GUIfont::PrintColor(const string& text)
{
	glPushAttrib(GL_COLOR_BUFFER_BIT);

	glColor3f(1, 1, 1);
	size_t lf;
	string temp=text;
	while((lf=temp.find("\xff"))!=string::npos)
	{
		Print(temp.substr(0, lf));
		temp=temp.substr(lf, string::npos);

		float r=((unsigned char)temp[1])/255.0;
		float g=((unsigned char)temp[2])/255.0;
		float b=((unsigned char)temp[3])/255.0;

		glColor3f(r, g, b);
		temp=temp.substr(4, string::npos);
	}
	Print(temp);

	glPopAttrib();
}

float GUIfont::GetWidth(const string& text) const
{
	float width=0;
	for (unsigned char *p=(unsigned char*)text.c_str(); *p; p++)
		width += charWidth[*p];
	return width;
}

float GUIfont::GetHeight() const
{
	return charHeight;
}
