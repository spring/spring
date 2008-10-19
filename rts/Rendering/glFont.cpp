#include "StdAfx.h"
// glFont.cpp: implementation of the CglFont class.
//
//////////////////////////////////////////////////////////////////////

#include "glFont.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdexcept>
#include "mmgr.h"

#include "Game/Camera.h"
#include "myMath.h"
#include "Platform/FileSystem.h"
#include "System/GlobalUnsynced.h"
#include "System/Util.h"
#include "System/Exceptions.h"

using namespace std;

/*******************************************************************************/
/*******************************************************************************/

CglFont *font, *smallFont;

#define GLYPH_MARGIN 3 // margin between glyphs in texture-atlas

class texture_size_exception : public std::exception
{
};

/*******************************************************************************/
/*******************************************************************************/

/**
 * A utility class for CglFont which collects all glyphs of
 * a font into one square luminance/alpha texture.
 */
class CFontTextureRenderer
{
public:
	CFontTextureRenderer(int width, int height);
	~CFontTextureRenderer();
	void AddGlyph(FT_GlyphSlot slot, int &outX, int &outY);
	GLuint CreateTexture();
private:
	int width, height;
	unsigned char *buffer, *cur;
	int curX, curY;
	int curHeight;		// height of highest glyph in current line

	void BreakLine();
};

CFontTextureRenderer::CFontTextureRenderer(int width, int height)
{
	this->width = width;
	this->height = height;
	buffer = SAFE_NEW unsigned char[2*width*height];		// luminance and alpha per pixel
	memset(buffer, 0xFF00, width*height);
	cur = buffer;
	curX = curY = 0;
	curHeight = 0;
}

CFontTextureRenderer::~CFontTextureRenderer()
{
	if (buffer)
		delete [] buffer;
}

void CFontTextureRenderer::AddGlyph(FT_GlyphSlot slot, int &outX, int &outY)
{
	FT_Bitmap &bmp = slot->bitmap;
	if (bmp.width > width - curX)
		BreakLine();
	if (bmp.width > width - curX)
		throw texture_size_exception();
	if (bmp.rows > height - curY)
		throw texture_size_exception();

	// blit the bitmap into our buffer (row by row to avoid pitch issues)
	for (int y = 0; y < bmp.rows; y++) {
		unsigned char *src = bmp.buffer + y * bmp.pitch;
		unsigned char *dst = cur + 2 * y * width;
		for (int x = 0; x < bmp.width; x++) {
			*dst++ = 255;		// luminance
			*dst++ = *src++;	// alpha
		}
	}
	outX = curX; outY = curY;

	curX += bmp.width + GLYPH_MARGIN;	// leave one pixel space between each glyph
	cur  += 2*(bmp.width + GLYPH_MARGIN); 	// 2channels (luminance and alpha))
	curHeight = max(curHeight, bmp.rows);
}

void CFontTextureRenderer::BreakLine()
{
	curX = 0;
	curY += curHeight + GLYPH_MARGIN;
	curHeight = 0;

	if (curY >= height)
		throw texture_size_exception();

	cur = buffer + curY * 2*width;
}

GLuint CFontTextureRenderer::CreateTexture()
{
	GLuint tex;
	glGenTextures(1, &tex);

	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	if (GLEW_EXT_texture_edge_clamp) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
		width, height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, buffer);
	delete [] buffer;
	buffer = 0;
	if (glGetError() != GL_NO_ERROR)
		throw std::runtime_error("Could not allocate font texture.");
	return tex;
}



CglFont::CglFont(int start, int end, const char* fontfile, float size, int texWidth, int texHeight)
{
	FT_Library library;
	FT_Face face;

	FT_Error error=FT_Init_FreeType(&library);
	if (error) {
		string msg="FT_Init_FreeType failed:";
		msg += GetFTError(error);
		throw std::runtime_error(msg);
	}

	error = FT_New_Face(library, filesystem.LocateFile(fontfile).c_str(), 0, &face);
	if (error) {
		string msg = string(fontfile) + ": FT_New_Face failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
	}

	const float height = gu->viewSizeY*size*64.0f;
	FT_Set_Char_Size(face, (int)(height * gu->aspectRatio/(4.0f/3.0f)), (int)height, 72, 72);

	// clamp the char range
	end = min(254, end);
	start = max(32, start);

	chars = (end - start) + 1;

	charend = end;
	charstart = start;

	glyphs = SAFE_NEW GlyphInfo[chars];

	const float toX = gu->pixelX;
	const float toY = gu->pixelY;

	float descender = FT_MulFix(face->descender, face->size->metrics.y_scale) / 64.0f;	// max font descender in pixels
	fontHeight = FT_MulFix(face->height, face->size->metrics.y_scale) / 64.0f * toY;	// distance between baselines in screen units

	CFontTextureRenderer texRenderer(texWidth, texHeight);

	for (int i = charstart; i <= charend; i++) {
		GlyphInfo *g = &glyphs[i-charstart];

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
		if (error) {
			string msg = "FT_Render_Glyph failed:";
			msg += GetFTError(error);
			throw std::runtime_error(msg);
		}
		FT_GlyphSlot slot = face->glyph;

		int texture_x, texture_y;
		try {
			texRenderer.AddGlyph(slot, texture_x, texture_y);
		} catch (texture_size_exception&) {
			FT_Done_Face(face);			// destructor does not run when re-throwing
			FT_Done_FreeType(library);
			delete [] glyphs;
			throw;
		}

		int bitmap_width = slot->bitmap.width;
		int bitmap_rows = slot->bitmap.rows;
		int bitmap_pitch = slot->bitmap.pitch;
		// Keep sign!
		float ybearing = slot->metrics.horiBearingY / 64.0f;
		float xbearing = slot->metrics.horiBearingX / 64.0f;

		g->advance = slot->advance.x / 64.0f * toX;
		g->height = slot->metrics.height / 64.0f * toY;

		// determine texture coordinates of glyph bitmap in font texture
		g->u0 = (float)texture_x / (float)texWidth;
		g->v0 = (float)texture_y / (float)texHeight;
		g->u1 = g->u0 + (float)bitmap_width / (float)texWidth;
		g->v1 = g->v0 + (float)bitmap_rows / (float)texHeight;

		g->x0 = xbearing * toX;
		g->y0 = (ybearing - descender) * toY;
		g->x1 = (xbearing + bitmap_width) * toX;
		g->y1 = g->y0 - (slot->metrics.height / 64.0f) * toY;
	}

	FT_Done_Face(face);
	FT_Done_FreeType(library);

	fontTexture = texRenderer.CreateTexture();

	outline = false;
	color = outlineColor = 0;
}


CglFont *CglFont::TryConstructFont(std::string fontFile, int start, int end, float size)
{
	CglFont *newFont = 0;
	int texWidth = 128, texHeight = 128;
	while (true) {
		try {
			newFont = SAFE_NEW CglFont(start, end, fontFile.c_str(), size, texWidth, texHeight);
			return newFont;
		} catch(texture_size_exception&) {
			// texture size too small; make higher or wider
			if (texHeight <= texWidth)
				texHeight *= 2;
			else
				texWidth *= 2;
		}
	}
}

CglFont::~CglFont()
{
	delete[] glyphs;
}


static inline int SkipColorCodes(const char* text, int c)
{
	while (text[c] == '\xff') {
		c++; if (text[c] == 0) { return -1; }
		c++; if (text[c] == 0) { return -1; }
		c++; if (text[c] == 0) { return -1; }
		c++; if (text[c] == 0) { return -1; }
	}
	return c;
}

float CglFont::CalcCharWidth (char c) const
{
	const unsigned int ch = (unsigned int)c;
	if ((c >= charstart) && (c <= charend)) {
		return glyphs[ch - charstart].advance;
	} else
		return 0.0f;
}

float CglFont::CalcTextWidth(const char *text) const
{
	float w=0.0f;
	for (int a = 0; text[a]; a++)  {
		a = SkipColorCodes(text, a);
		if (a < 0) {
			break;
		}
		const unsigned int c = (unsigned int)text[a];
		if ((c >= charstart) && (c <= charend)) {
			const float charpart = glyphs[c - charstart].advance;
			w += charpart;// + 0.02f;
		}
	}
	return w;
}

float CglFont::CalcTextHeight(const char *text) const
{
	float h=0.0f;
	for (int a = 0; text[a]; a++)  {
		a = SkipColorCodes(text, a);
		if (a < 0) {
			break;
		}
		const unsigned int c = (unsigned int)text[a];
		if ((c >= charstart) && (c <= charend)) {
			float charpart = glyphs[c - charstart].height;
			if (charpart > h) {
				h = charpart;
			}
		}
	}
	return h;
}


void CglFont::Outline(bool enable, const float *color, const float *outlineColor)
{
	this->outline = enable;
	if (enable) {
		if (color) {		// if color is null, we keep the old settings (which should better not be null)
			this->color = color;
			this->outlineColor = outlineColor ? outlineColor : ChooseOutlineColor(color);
		}
	}
}

void CglFont::OutlineS(bool enable, const float *outlineColor)
{
	this->outline = enable;
	if (enable) {;
		this->outlineColor = outlineColor ? outlineColor : ChooseOutlineColor(color);
	}
}

float CglFont::RenderString(float x, float y, float s, const unsigned char *text) const
{
//	glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT  | GL_ENABLE_BIT | GL_TRANSFORM_BIT);
	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* NOTE:
	 *
	 * Font rendering no longer uses display lists, but immediate mode quads. This might
	 * sound crazy at first, but it's actually faster for these reasons:
	 *
	 * 1. When using DLs, we can not group multiple glyphs into one glBegin/End pair
	 *    because glTranslatef can not go between such a pair.
	 * 2. We can now eliminate all glPushMatrix/PopMatrix pairs related to font rendering
	 *    because the transformations are calculated on the fly. These are just a couple of
	 *    floating point multiplications and shouldn't be too expensive.
	 * 3. There is a certain overhead to using DLs, and having one quad per DL is probably
	 *    not efficient.
	 */

	glBegin(GL_QUADS);
	for (int i = 0; i < strlen((const char*)text); i++) {
		const unsigned int ch = (unsigned char)text[i];
		if ((ch >= charstart) && (ch <= charend)) {
			GlyphInfo *g = &glyphs[ch - charstart];

			glTexCoord2f(g->u0, g->v1); glVertex2f(x+s*g->x0, y+s*g->y1);
			glTexCoord2f(g->u0, g->v0); glVertex2f(x+s*g->x0, y+s*g->y0);
			glTexCoord2f(g->u1, g->v0); glVertex2f(x+s*g->x1, y+s*g->y0);
			glTexCoord2f(g->u1, g->v1); glVertex2f(x+s*g->x1, y+s*g->y1);

			x += s*g->advance;
		}
	}
	glEnd();

	glPopAttrib();

	return x;
}

float CglFont::RenderStringOutlined(float x, float y, float s, const unsigned char *text)
{
	const float shiftX = gu->pixelX, shiftY = gu->pixelY;

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBegin(GL_QUADS);
	for (int i = 0; i < strlen((const char*)text); i++) {
		const unsigned int ch = (unsigned char)text[i];
		if ((ch >= charstart) && (ch <= charend)) {

			GlyphInfo *g = &glyphs[ch - charstart];

			float dx0 = x + s * g->x0, dy0 = y + s * g->y0;
			float dx1 = x + s * g->x1, dy1 = y + s * g->y1;

			glColor4fv(outlineColor);

			// draw 4 offset copies of the character
			glTexCoord2f(g->u0, g->v1); glVertex2f(dx0+shiftX, dy1+shiftY);
			glTexCoord2f(g->u0, g->v0); glVertex2f(dx0+shiftX, dy0+shiftY);
			glTexCoord2f(g->u1, g->v0); glVertex2f(dx1+shiftX, dy0+shiftY);
			glTexCoord2f(g->u1, g->v1); glVertex2f(dx1+shiftX, dy1+shiftY);

			glTexCoord2f(g->u0, g->v1); glVertex2f(dx0+shiftX, dy1-shiftY);
			glTexCoord2f(g->u0, g->v0); glVertex2f(dx0+shiftX, dy0-shiftY);
			glTexCoord2f(g->u1, g->v0); glVertex2f(dx1+shiftX, dy0-shiftY);
			glTexCoord2f(g->u1, g->v1); glVertex2f(dx1+shiftX, dy1-shiftY);

			glTexCoord2f(g->u0, g->v1); glVertex2f(dx0-shiftX, dy1-shiftY);
			glTexCoord2f(g->u0, g->v0); glVertex2f(dx0-shiftX, dy0-shiftY);
			glTexCoord2f(g->u1, g->v0); glVertex2f(dx1-shiftX, dy0-shiftY);
			glTexCoord2f(g->u1, g->v1); glVertex2f(dx1-shiftX, dy1-shiftY);

			glTexCoord2f(g->u0, g->v1); glVertex2f(dx0-shiftX, dy1+shiftY);
			glTexCoord2f(g->u0, g->v0); glVertex2f(dx0-shiftX, dy0+shiftY);
			glTexCoord2f(g->u1, g->v0); glVertex2f(dx1-shiftX, dy0+shiftY);
			glTexCoord2f(g->u1, g->v1); glVertex2f(dx1-shiftX, dy1+shiftY);

			glColor4fv(color);

			// draw the actual character
			glTexCoord2f(g->u0, g->v1); glVertex2f(dx0, dy1);
			glTexCoord2f(g->u0, g->v0); glVertex2f(dx0, dy0);
			glTexCoord2f(g->u1, g->v0); glVertex2f(dx1, dy0);
			glTexCoord2f(g->u1, g->v1); glVertex2f(dx1, dy1);

			x += s*g->advance;
		}
	}
	glEnd();

	glPopAttrib();

	outline = false;		// outlining is valid only for one drawing operation
	return x;
}


void CglFont::glPrintOutlinedAt(float x, float y, float scale, const char* text, const float* normalColor)
{
	Outline(true, normalColor);
	glPrintAt(x, y, scale, text);
}

void CglFont::glPrintOutlinedRight(float x, float y, float scale, const char* text, const float* normalColor)
{
	Outline(true, normalColor);
	glPrintRight(x, y, scale, text);
}

void CglFont::glPrintOutlinedCentered(float x, float y, float scale, const char* text, const float* normalColor)
{
	Outline(true, normalColor);
	glPrintCentered(x, y, scale, text);
}

// macro for formatting printf-style
#define FORMAT_STRING(fmt,out)					\
		char out[512];							\
		va_list	ap;								\
		if (fmt == NULL)						\
			return;								\
		va_start(ap, fmt);						\
		VSNPRINTF(out, sizeof(out), fmt, ap);	\
		va_end(ap);


void CglFont::glWorldPrint(const char *str) const
{
	float w=gu->aspectRatio * CalcTextWidth(str);
	/* Center (screen-wise) the text above the current position. */
	glTranslatef(-0.5f*w*camera->right.x,
	             -0.5f*w*camera->right.y,
	             -0.5f*w*camera->right.z);
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	for (int a = 0; str[a]; a++) {
		unsigned int ch = (unsigned char)str[a];
		if (ch >= charstart && ch <= charend)
			WorldChar(ch);
	}
}

void CglFont::WorldChar(unsigned int ch) const
{
	GlyphInfo *g = &glyphs[ch - charstart];

	glBegin(GL_QUADS);

	const float aspect = gu->aspectRatio;
	const float ux = camera->up.x, uy = camera->up.y, uz = camera->up.z;
	const float rx = aspect*camera->right.x, ry = aspect*camera->right.y, rz = aspect*camera->right.z;

#define CAMCOORDS(cx,cy) cx*rx+cy*ux,cx*ry+cy*uy,cx*rz+cy*uz

	glTexCoord2f(g->u0, g->v1);
	glVertex3f(CAMCOORDS(g->x0, g->y1));

	glTexCoord2f(g->u0, g->v0);
	glVertex3f(CAMCOORDS(g->x0, g->y0));

	glTexCoord2f(g->u1, g->v0);
	glVertex3f(CAMCOORDS(g->x1, g->y0));

	glTexCoord2f(g->u1, g->v1);
	glVertex3f(CAMCOORDS(g->x1, g->y1));

	glEnd();

	float advance = g->advance;
	glTranslatef(advance*rx, advance*ry, advance*rz);
}



// centered version of glPrintAt
void CglFont::glPrintCentered (float x, float y, float s, const char *fmt, ...)
{
	FORMAT_STRING(fmt,text);

	x -= s * 0.5f * CalcTextWidth(text);

	if (outline)
		RenderStringOutlined(x, y, s, (const unsigned char*)text);
	else
		RenderString(x, y, s, (const unsigned char*)text);
}

void CglFont::glPrintAt(GLfloat x, GLfloat y, float s, const char *str)
{
	if (outline)
		RenderStringOutlined(x, y, s, (const unsigned char*)str);
	else
		RenderString(x, y, s, (const unsigned char*)str);
}

// right justified version of glPrintAt
void CglFont::glPrintRight (float x, float y, float s, const char *fmt, ...)
{
	FORMAT_STRING(fmt,text);

	x -= s * CalcTextWidth(text);

	if (outline)
		RenderStringOutlined(x, y, s, (const unsigned char*)text);
	else
		RenderString(x, y, s, (const unsigned char*)text);
}

void CglFont::glPrintColorAt(GLfloat x, GLfloat y, float s, const char *str)
{
	//TODO both glColor and float* color is set, make RenderString respect the float *color
	const float *oldcolor = color;
	float newcolor[4] = {1.0, 1.0, 1.0, 1.0};
	color = const_cast<const float*>(newcolor);
	glColor4fv(color);
	size_t lf;
	string temp=str;
	while((lf=temp.find("\xff"))!=string::npos) {
		if (outline)
		{
			x = RenderStringOutlined(x, y, s, (const unsigned char*)temp.substr(0, lf).c_str());
			outline = true; //HACK RenderStringOutlined resets outline
		}
		else
			x = RenderString(x, y, s, (const unsigned char*)temp.substr(0, lf).c_str());
		temp=temp.substr(lf, string::npos);
		newcolor[0] = ((unsigned char)temp[1])/255.0f;
		newcolor[1] = ((unsigned char)temp[2])/255.0f;
		newcolor[2] = ((unsigned char)temp[3])/255.0f;
		glColor4fv(color);
		temp=temp.substr(4, string::npos);
	}
	if (outline)
		RenderStringOutlined(x, y, s, (const unsigned char*)temp.c_str());
	else
		RenderString(x, y, s, (const unsigned char*)temp.c_str());
	color = oldcolor;
}

void CglFont::glFormatAt(GLfloat x, GLfloat y, float s, const char *fmt, ...)
{
	FORMAT_STRING(fmt,text);
	glPrintAt(x, y, s, text);
}


const float* CglFont::ChooseOutlineColor(const float *textColor)
{
	static const float darkOutline[4]  = { 0.25f, 0.25f, 0.25f, 0.8f };
	static const float lightOutline[4] = { 0.85f, 0.85f, 0.85f, 0.8f };

	const float luminance = (textColor[0] * 0.299f) +
	                        (textColor[1] * 0.587f) +
	                        (textColor[2] * 0.114f);

	if (luminance > 0.25f) {
		return darkOutline;
	} else {
		return lightOutline;
	}
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


const char* CglFont::GetFTError (FT_Error e) const
{
	for (int a=0;errorTable[a].err_msg;a++) {
		if (errorTable[a].err_code == e)
			return errorTable[a].err_msg;
	}
	return "Unknown error";
}
