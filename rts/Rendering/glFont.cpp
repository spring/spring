/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "glFont.h"
#include <string>
#include <cstring> // for memset, memcpy
#include <stdio.h>
#include <stdarg.h>
#include <stdexcept>
#ifndef   HEADLESS
#include <ft2build.h>
#include FT_FREETYPE_H
#endif // HEADLESS

#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/LogOutput.h"
#include "System/myMath.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/GlobalUnsynced.h"
#include "System/Util.h"
#include "System/Exceptions.h"
#include "System/mmgr.h"
#include "System/float4.h"
#include "bitops.h"

#undef GetCharWidth // winapi.h

using std::string;

/*******************************************************************************/
/*******************************************************************************/

CglFont* font;
CglFont* smallFont;

#define GLYPH_MARGIN 2 //! margin between glyphs in texture-atlas

static const unsigned char nullChar = 0;
static const float4        white(1.00f, 1.00f, 1.00f, 0.95f);
static const float4  darkOutline(0.05f, 0.05f, 0.05f, 0.95f);
static const float4 lightOutline(0.95f, 0.95f, 0.95f, 0.8f);

static const float darkLuminosity = 0.05 +
	0.2126f * math::powf(darkOutline[0], 2.2) +
	0.7152f * math::powf(darkOutline[1], 2.2) +
	0.0722f * math::powf(darkOutline[2], 2.2);

/*******************************************************************************/
/*******************************************************************************/

/**
 * Those chars aren't part of the ISO-8859 and because of that not mapped
 * 1:1 into unicode. So we need this table for translation.
 *
 * copied from:
 * http://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1252.TXT
 */

int WinLatinToUnicode_[32] = {
	/*0x80*/	0x20AC,	//EURO SIGN
	/*0x81*/	   0x0,	//UNDEFINED
	/*0x82*/	0x201A,	//SINGLE LOW-9 QUOTATION MARK
	/*0x83*/	0x0192,	//LATIN SMALL LETTER F WITH HOOK
	/*0x84*/	0x201E,	//DOUBLE LOW-9 QUOTATION MARK
	/*0x85*/	0x2026,	//HORIZONTAL ELLIPSIS
	/*0x86*/	0x2020,	//DAGGER
	/*0x87*/	0x2021,	//DOUBLE DAGGER
	/*0x88*/	0x02C6,	//MODIFIER LETTER CIRCUMFLEX ACCENT
	/*0x89*/	0x2030,	//PER MILLE SIGN
	/*0x8A*/	0x0160,	//LATIN CAPITAL LETTER S WITH CARON
	/*0x8B*/	0x2039,	//SINGLE LEFT-POINTING ANGLE QUOTATION MARK
	/*0x8C*/	0x0152,	//LATIN CAPITAL LIGATURE OE
	/*0x8D*/	   0x0,	//UNDEFINED
	/*0x8E*/	0x017D,	//LATIN CAPITAL LETTER Z WITH CARON
	/*0x8F*/	   0x0,	//UNDEFINED
	/*0x90*/	   0x0,	//UNDEFINED
	/*0x91*/	0x2018,	//LEFT SINGLE QUOTATION MARK
	/*0x92*/	0x2019,	//RIGHT SINGLE QUOTATION MARK
	/*0x93*/	0x201C,	//LEFT DOUBLE QUOTATION MARK
	/*0x94*/	0x201D,	//RIGHT DOUBLE QUOTATION MARK
	/*0x95*/	0x2022,	//BULLET
	/*0x96*/	0x2013,	//EN DASH
	/*0x97*/	0x2014,	//EM DASH
	/*0x98*/	0x02DC,	//SMALL TILDE
	/*0x99*/	0x2122,	//TRADE MARK SIGN
	/*0x9A*/	0x0161,	//LATIN SMALL LETTER S WITH CARON
	/*0x9B*/	0x203A,	//SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
	/*0x9C*/	0x0153,	//LATIN SMALL LIGATURE OE
	/*0x9D*/	   0x0,	//UNDEFINED
	/*0x9E*/	0x017E,	//LATIN SMALL LETTER Z WITH CARON
	/*0x9F*/	0x0178	//LATIN CAPITAL LETTER Y WITH DIAERESIS
};

inline static int WinLatinToUnicode(int c)
{
	if (c>=0x80 && c<=0x9F) {
		return WinLatinToUnicode_[ c - 0x80 ];
	}
	return c; //! all other chars are mapped 1:1
}

/*******************************************************************************/
/*******************************************************************************/

class texture_size_exception : public std::exception
{
};


#ifndef   HEADLESS
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


static const char* GetFTError(FT_Error e)
{
	for (int a = 0; errorTable[a].err_msg; ++a) {
		if (errorTable[a].err_code == e)
			return errorTable[a].err_msg;
	}
	return "Unknown error";
}
#endif // HEADLESS

/*******************************************************************************/
/*******************************************************************************/

/**
 * A utility class for CglFont which collects all glyphs of
 * a font into one alpha-only texture.
 */
class CFontTextureRenderer
{
public:
	CFontTextureRenderer(int _outlinewidth, int _outlineweight)
		: texWidth(0), texHeight(0),
		outlinewidth(_outlinewidth), outlineweight(_outlineweight),
		atlas(NULL),
		cur(NULL),
		curX(0), curY(0),
		curHeight(0),
		numGlyphs(0),
		maxGlyphWidth(0), maxGlyphHeight(0),
		numPixels(0)
	{
	};

	GLuint CreateTexture();

	void AddGlyph(unsigned int& index, int& xsize, int& ysize, unsigned char* pixels, int& pitch);
	void GetGlyph(unsigned int& index, CglFont::GlyphInfo* g) const;

	int texWidth, texHeight;
private:
	void CopyGlyphsIntoBitmapAtlas(bool outline = false);
	void ApproximateTextureWidth(int* width, int* height);

private:
	struct GlyphInfo {
		GlyphInfo() : pixels(NULL), xsize(0), ysize(0), u(INT_MAX),v(INT_MAX),us(INT_MAX),vs(INT_MAX) {};
		~GlyphInfo() {delete[] pixels;};
		unsigned char* pixels;
		int xsize;
		int ysize;
		int u,v;
		int us,vs;
	} glyphs[256];

private:
	std::list<int2> sortByHeight;

	int outlinewidth, outlineweight;

	unsigned char* atlas;
	unsigned char* cur;
	int curX, curY;
	int curHeight;         //! height of highest glyph in current line

	unsigned int numGlyphs;
	unsigned int maxGlyphWidth, maxGlyphHeight;
	unsigned int numPixels;
};


void CFontTextureRenderer::CopyGlyphsIntoBitmapAtlas(bool outline)
{
	const int border    = (outline) ? outlinewidth : 0;
	const int twoborder = 2 * border;

	std::list<int2>::iterator gi, finish;
	std::list<int2>::reverse_iterator rgi, rfinish;
	if (outline) {
		gi     = sortByHeight.begin();
		finish = sortByHeight.end();
	} else {
		rgi     = sortByHeight.rbegin();
		rfinish = sortByHeight.rend();
	}

	for(; (outline && gi != finish) || (!outline && rgi != rfinish); (outline) ? (void *)&++gi : (void *)&++rgi) {
		GlyphInfo& g = glyphs[(outline) ? gi->x : rgi->x];

		if (g.xsize==0 || g.ysize==0)
			continue;

		if (g.xsize + twoborder > texWidth - curX) {
			curX = 0;
			curY += curHeight + GLYPH_MARGIN;

			curHeight = 0;
			cur = atlas + curY * texWidth;
			if (g.xsize + twoborder > texWidth - curX) {
				delete[] atlas;
				throw texture_size_exception();
			}
		}

		if (g.ysize + twoborder > texHeight - curY) {
			unsigned int oldTexHeight = texHeight;
			unsigned char* oldAtlas = atlas;
			texHeight <<= 2;
			atlas = new unsigned char[texWidth * texHeight];
			memcpy(atlas, oldAtlas, texWidth * oldTexHeight);
			memset(atlas + texWidth * oldTexHeight, 0, texWidth * (texHeight - oldTexHeight));
			delete[] oldAtlas;
			cur = atlas + curY * texWidth;
			if (texHeight>2048) {
				delete[] atlas;
				throw texture_size_exception();
			}
		}

		//! blit the bitmap into our buffer
		for (int y = 0; y < g.ysize; y++) {
			const unsigned char* src = g.pixels + y * g.xsize;
			unsigned char* dst = cur + (y + border) * texWidth + border;
			memcpy(dst, src, g.xsize);
		}

		if (outline) {
			g.us = curX; g.vs = curY;
		} else {
			g.u = curX; g.v = curY;
		}

		curX += g.xsize + GLYPH_MARGIN + twoborder;  //! leave a margin between each glyph
		cur  += g.xsize + GLYPH_MARGIN + twoborder;
		curHeight = std::max(curHeight, g.ysize + twoborder);
	}
}



void CFontTextureRenderer::GetGlyph(unsigned int& index, CglFont::GlyphInfo* g) const
{
	const GlyphInfo& gi = glyphs[index];
	g->u0 = gi.u / (float)texWidth;
	g->v0 = gi.v / (float)texHeight;
	g->u1 = (gi.u+gi.xsize) / (float)texWidth;
	g->v1 = (gi.v+gi.ysize) / (float)texHeight;
	g->us0 = gi.us / (float)texWidth;
	g->vs0 = gi.vs / (float)texHeight;
	g->us1 = (gi.us+gi.xsize+2*outlinewidth) / (float)texWidth;
	g->vs1 = (gi.vs+gi.ysize+2*outlinewidth) / (float)texHeight;
}


void CFontTextureRenderer::AddGlyph(unsigned int& index, int& xsize, int& ysize, unsigned char* pixels, int& pitch)
{
	GlyphInfo& g = glyphs[index];
	g.xsize = xsize;
	g.ysize = ysize;
	g.pixels = new unsigned char[xsize*ysize];

	for (int y = 0; y < ysize; y++) {
		const unsigned char* src = pixels + y * pitch;
		unsigned char* dst = g.pixels + y * xsize;
		memcpy(dst,src,xsize);
	}

	numGlyphs++;

	//! 1st approach to approximate needed texture size
	numPixels += (xsize + GLYPH_MARGIN) * (ysize + GLYPH_MARGIN);
	numPixels += (xsize + 2 * outlinewidth + GLYPH_MARGIN) * (ysize + 2 * outlinewidth + GLYPH_MARGIN);

	//! needed for the 2nd approach
	if (xsize>maxGlyphWidth)  maxGlyphWidth  = xsize;
	if (ysize>maxGlyphHeight) maxGlyphHeight = ysize;

	sortByHeight.push_back(int2(index,ysize));
}


void CFontTextureRenderer::ApproximateTextureWidth(int* width, int* height)
{
	//! 2nd approach to approximate needed texture size
	unsigned int numPixels2 = numGlyphs * (maxGlyphWidth + GLYPH_MARGIN) * (maxGlyphHeight + GLYPH_MARGIN);
	numPixels2 += numGlyphs * (maxGlyphWidth + 2 * outlinewidth + GLYPH_MARGIN) * (maxGlyphHeight + 2 * outlinewidth + GLYPH_MARGIN);

	/**
	 * 1st approach is too fatuous (it assumes there is no wasted space in the atlas)
	 * 2nd approach is too pessimistic (it just takes the largest glyph and multiplies it with the num of glyphs)
	 *
	 * So what do we do? .. YEAH! we just take the average of them ;)
	 */
	unsigned int numPixelsAvg = (numPixels + numPixels2) / 2;

	*width  = next_power_of_2(math::ceil(streflop::sqrtf( (float)numPixelsAvg )));
	*height = next_power_of_2(math::ceil( (float)numPixelsAvg / (float)*width ));

	if (*width > 2048)
		throw texture_size_exception();
}


static inline bool sortByHeightFunc(int2 first, int2 second)
{
	  return (first.y < second.y);
}


GLuint CFontTextureRenderer::CreateTexture()
{
//FIXME add support to expand the texture size to the right and not only to the bottom
	ApproximateTextureWidth(&texWidth,&texHeight);

	atlas = new unsigned char[texWidth * texHeight];
	memset(atlas,0,texWidth * texHeight);
	cur = atlas;

	//! sort the glyphs by height to gain a few pixels
	sortByHeight.sort(sortByHeightFunc);

	//! insert `outlined/shadowed` glyphs
	CopyGlyphsIntoBitmapAtlas(true);

	//! blur `outlined/shadowed` glyphs
	CBitmap blurbmp;
	blurbmp.channels = 1;
	blurbmp.xsize = texWidth;
	blurbmp.ysize = curY+curHeight;
	if (blurbmp.mem == NULL) {
		blurbmp.mem = atlas;
		blurbmp.Blur(outlinewidth,outlineweight);
		blurbmp.mem = NULL;
	}

	//! adjust outline weight
	/*for (int y = 0; y < curY+curHeight; y++) {
		unsigned char* src = atlas + y * texWidth;
		for (int x = 0; x < texWidth; x++) {
			unsigned char luminance = src[x];
			src[x] = (unsigned char)std::min(0xFF, outlineweight * luminance);
		}
	}*/

	//! insert `normal` glyphs
	CopyGlyphsIntoBitmapAtlas();

	//! generate the ogl texture
	texHeight = curY + curHeight;
	if (!globalRendering->supportNPOTs)
		texHeight = next_power_of_2(texHeight);
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	if (GLEW_ARB_texture_border_clamp) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	}
	else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}
	static const GLfloat borderColor[4] = {1.0f,1.0f,1.0f,0.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, texWidth, texHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, atlas);
	delete[] atlas;

	return tex;
}

/*******************************************************************************/
/*******************************************************************************/

CglFont::CglFont(const std::string& fontfile, int size, int _outlinewidth, float _outlineweight):
	fontSize(size),
	fontPath(fontfile),
	outlineWidth(_outlinewidth),
	outlineWeight(_outlineweight),
	inBeginEnd(false),
	autoOutlineColor(true),
	setColor(false)
{
	va  = new CVertexArray();
	va2 = new CVertexArray();

	if (size<=0)
		size = 14;

	const float invSize = 1.0f / size;
	const float normScale = invSize / 64.0f;

	//! setup character range
	charstart = 32;
	charend   = 254; //! char 255 = colorcode
	chars     = (charend - charstart) + 1;

#ifndef   HEADLESS
	FT_Library library;
	FT_Face face;

	//! initialize Freetype2 library
	FT_Error error = FT_Init_FreeType(&library);
	if (error) {
		string msg = "FT_Init_FreeType failed:";
		msg += GetFTError(error);
		throw std::runtime_error(msg);
	}

	//! load font via VFS
	CFileHandler* f = new CFileHandler(fontPath);
	if (!f->FileExists()) {
		//! check in 'fonts/', too
		if (fontPath.substr(0, 6) != "fonts/") {
			delete f;
			fontPath = "fonts/" + fontPath;
			f = new CFileHandler(fontPath);
		}

		if (!f->FileExists()) {
			delete f;
			FT_Done_FreeType(library);
			throw content_error("Couldn't find font '" + fontfile + "'.");
		}
	}
	int filesize = f->FileSize();
	FT_Byte* buf = new FT_Byte[filesize];
	f->Read(buf,filesize);
	delete f;

	//! create face
	error = FT_New_Memory_Face(library, buf, filesize, 0, &face);
	if (error) {
		FT_Done_FreeType(library);
		delete[] buf;
		string msg = fontfile + ": FT_New_Face failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
	}

	//! set render size
	error = FT_Set_Pixel_Sizes(face, 0, size);
	if (error) {
		FT_Done_Face(face);
		FT_Done_FreeType(library);
		delete[] buf;
		string msg = fontfile + ": FT_Set_Pixel_Sizes failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
	}

	//! get font information
	fontFamily = face->family_name;
	fontStyle  = face->style_name;

	//! font's descender & height (in pixels)
	fontDescender = normScale * FT_MulFix(face->descender, face->size->metrics.y_scale);
	//lineHeight    = invSize * (FT_MulFix(face->height, face->size->metrics.y_scale) / 64.0f);
	//lineHeight    = invSize * math::ceil(FT_MulFix(face->height, face->size->metrics.y_scale) / 64.0f);
	lineHeight = face->height / face->units_per_EM;
	//lineHeight = invSize * face->size->metrics.height / 64.0f;

	if (lineHeight<=0.0f) {
		lineHeight = 1.25 * invSize * (face->bbox.yMax - face->bbox.yMin);
	}

	//! used to create the glyph textureatlas
	CFontTextureRenderer texRenderer(outlineWidth, outlineWeight);

	for (unsigned int i = charstart; i <= charend; i++) {
		GlyphInfo* g = &glyphs[i];

		//! translate WinLatin (codepage-1252) to Unicode (used by freetype)
		int unicode = WinLatinToUnicode(i);

		//! convert to an anti-aliased bitmap
		error = FT_Load_Char(face, unicode, FT_LOAD_RENDER);
		if ( error ) {
			continue;
		}
		FT_GlyphSlot slot = face->glyph;

		//! Keep sign!
		const float ybearing = slot->metrics.horiBearingY * normScale;
		const float xbearing = slot->metrics.horiBearingX * normScale;

		g->advance   = slot->advance.x * normScale;
		g->height    = slot->metrics.height * normScale;
		g->descender = ybearing - g->height;

		g->x0 = xbearing;
		g->y0 = ybearing - fontDescender;
		g->x1 = (xbearing + slot->metrics.width * normScale);
		g->y1 = g->y0 - g->height;

		texRenderer.AddGlyph(i, slot->bitmap.width, slot->bitmap.rows, slot->bitmap.buffer, slot->bitmap.pitch);
	}

	//! create font atlas texture
	fontTexture = texRenderer.CreateTexture();
	texWidth  = texRenderer.texWidth;
	texHeight = texRenderer.texHeight;

	//! get glyph's uv coords in the atlas
	for (unsigned int i = charstart; i <= charend; i++) {
		texRenderer.GetGlyph(i,&glyphs[i]);
	}

	//! generate kerning tables
	for (unsigned int i = charstart; i <= charend; i++) {
		GlyphInfo& g = glyphs[i];
		int unicode = WinLatinToUnicode(i);
		FT_UInt left_glyph = FT_Get_Char_Index(face, unicode);
		for (unsigned int j = 0; j <= 255; j++) {
			unicode = WinLatinToUnicode(j);
			FT_UInt right_glyph = FT_Get_Char_Index(face, unicode);
			FT_Vector kerning;
			kerning.x = kerning.y = 0.0f;
			FT_Get_Kerning(face, left_glyph, right_glyph, FT_KERNING_DEFAULT, &kerning);
			g.kerning[j] = g.advance + normScale * static_cast<float>(kerning.x);
		}
	}

	//! initialize null char
	GlyphInfo& g = glyphs[0];
	g.height = g.descender = g.advance = 0.0f;
	for (unsigned int j = 0; j <= 255; j++) {
		g.kerning[j] = 0.0f;
	}

	FT_Done_Face(face);
	FT_Done_FreeType(library);
	delete[] buf;
#else  // HEADLESS
	fontFamily = "NONE";
	fontStyle  = "NONE";
	fontDescender = 0.0f;
	lineHeight = 0.0f;
	fontTexture = 0;
	texWidth  = 0;
	texHeight = 0;
#endif // HEADLESS

	textColor    = white;
	outlineColor = darkOutline;
}


CglFont* CglFont::LoadFont(const std::string& fontFile, int size, int outlinewidth, float outlineweight, int start, int end)
{
	try {
		CglFont* newFont = new CglFont(fontFile, size, outlinewidth, outlineweight);
		return newFont;
	} catch (texture_size_exception&) {
		logOutput.Print("FONT-ERROR: Couldn't create GlyphAtlas! (try to reduce reduce font size/outlinewidth)");
		return NULL;
	} catch (content_error& e) {
		logOutput.Print(std::string(e.what()));
		return NULL;
	}
}


CglFont::~CglFont()
{
	glDeleteTextures(1, &fontTexture);

	stripTextColors.clear();
	stripOutlineColors.clear();

	delete va;
	delete va2;
}


/*******************************************************************************/
/*******************************************************************************/

template <typename T>
static inline int SkipColorCodesOld(const std::string& text, T pos)
{
	while (text[pos] == CglFont::ColorCodeIndicator) {
		pos += 4;
		if (pos >= text.size()) { return -1; }
	}
	return pos;
}


template <typename T>
static inline int SkipColorCodes(const std::string& text, T* pos, float4* color)
{
	int colorFound = 0;
	while (text[(*pos)] == CglFont::ColorCodeIndicator) {
		(*pos) += 4;
		if ((*pos) >= text.size()) {
			return -(1 + colorFound);
		} else {
			(*color)[0] = ((unsigned char) text[(*pos)-3]) / 255.0f;
			(*color)[1] = ((unsigned char) text[(*pos)-2]) / 255.0f;
			(*color)[2] = ((unsigned char) text[(*pos)-1]) / 255.0f;
			colorFound = 1;
		}
	}
	return colorFound;
}


/**
 * @brief SkipNewLine
 * @param text
 * @param pos index in the string
 * @return <0 := end of string; returned value is: -(skippedLines + 1)
 *         else: number of skipped lines (can be zero)
 */
template <typename T>
static inline int SkipNewLine(const std::string& text, T* pos)
{
	const size_t length = text.length();
	int skippedLines = 0;
	while (*pos < length) {
		const char& chr = text[*pos];
		switch(chr) {
			case '\x0d': //! CR
				skippedLines++;
				(*pos)++;
				if (*pos < length && text[*pos] == '\x0a') { //! CR+LF
					(*pos)++;
				}
				break;

			case '\x0a': //! LF
				skippedLines++;
				(*pos)++;
				break;

			default:
				return skippedLines;
		}
	}
	return -(1 + skippedLines);
}


template <typename T>
static inline bool SkipColorCodesAndNewLines(const std::string& text, T* pos, float4* color, bool* colorChanged, int* skippedLines, float4* colorReset)
{
	const size_t length = text.length();
	(*colorChanged) = false;
	(*skippedLines) = 0;
	while (*pos < length) {
		const char& chr = text[*pos];
		switch(chr) {
			case CglFont::ColorCodeIndicator:
				*pos += 4;
				if ((*pos) < length) {
					(*color)[0] = ((unsigned char) text[(*pos) - 3]) / 255.0f;
					(*color)[1] = ((unsigned char) text[(*pos) - 2]) / 255.0f;
					(*color)[2] = ((unsigned char) text[(*pos) - 1]) / 255.0f;
					*colorChanged = true;
				}
				break;

			case CglFont::ColorResetIndicator:
				(*pos)++;
				(*color) = *colorReset;
				*colorChanged = true;
				break;

			case '\x0d': //! CR
				(*skippedLines)++;
				(*pos)++;
				if (*pos < length && text[*pos] == '\x0a') { //! CR+LF
					(*pos)++;
				}
				break;

			case '\x0a': //! LF
				(*skippedLines)++;
				(*pos)++;
				break;

			default:
				return false;
		}
	}
	return true;
}


static inline void TextStripCallback(void* data)
{
	CglFont::ColorMap::iterator& sci = *reinterpret_cast<CglFont::ColorMap::iterator*>(data);
	glColor4fv(*sci++);
}


/*******************************************************************************/
/*******************************************************************************/

std::string CglFont::StripColorCodes(const std::string& text)
{
	const size_t len = text.size();

	std::string nocolor;
	nocolor.reserve(len);
	for (unsigned int i = 0; i < len; i++) {
		if (text[i] == ColorCodeIndicator) {
			i += 3;
		} else {
			nocolor += text[i];
		}
	}
	return nocolor;
}


float CglFont::GetKerning(const unsigned int& left_char, const unsigned int& right_char) const
{
	return glyphs[left_char].kerning[right_char];
}


float CglFont::GetCharacterWidth(const unsigned char c) const
{
	return glyphs[c].advance;
}


float CglFont::GetTextWidth(const std::string& text) const
{
	if (text.size()==0) return 0.0f;

	float w = 0.0f;
	float maxw = 0.0f;

	const unsigned char* prv_char = &nullChar;
	const unsigned char* cur_char;

	for (int pos = 0; pos < text.length(); pos++) {
		const char& c = text[pos];
		switch(c) {
			//! inlined colorcode
			case ColorCodeIndicator:
				pos = SkipColorCodesOld(text, pos);
				if (pos<0) {
					pos = text.length();
				} else {
					pos--;
				}
				break;

			//! reset color
			case ColorResetIndicator:
				break;

			//! newline
			case '\x0d': //! CR+LF
				if (pos+1 < text.length() && text[pos+1] == '\x0a')
					pos++;
			case '\x0a': //! LF
				w += glyphs[*prv_char].kerning[0];
				if (w > maxw)
					maxw = w;
				w = 0.0f;
				prv_char = &nullChar;
				break;

			//! printable char
			default:
				cur_char = reinterpret_cast<const unsigned char*>(&c);
				w += glyphs[*prv_char].kerning[*cur_char];
				prv_char = cur_char;
		}
	}

	w += glyphs[*prv_char].kerning[0];
	if (w > maxw)
		maxw = w;

	return maxw;
}


float CglFont::GetTextHeight(const std::string& text, float* descender, int* numLines) const
{
	if (text.empty()) {
		if (descender) *descender = 0.0f;
		if (numLines) *numLines = 0;
		return 0.0f;
	}

	float h = 0.0f, d = lineHeight + fontDescender;
	unsigned int multiLine = 1;

	for (int pos = 0 ; pos < text.length(); pos++) {
		const char& c = text[pos];
		switch(c) {
			//! inlined colorcode
			case ColorCodeIndicator:
				pos = SkipColorCodesOld(text, pos);
				if (pos<0) {
					pos = text.length();
				} else {
					pos--;
				}
				break;

			//! reset color
			case ColorResetIndicator:
				break;

			//! newline
			case '\x0d': //! CR+LF
				if (pos+1 < text.length() && text[pos+1] == '\x0a')
					pos++;
			case '\x0a': //! LF
				multiLine++;
				d = lineHeight + fontDescender;
				break;

			//! printable char
			default:
				const unsigned char* uc = reinterpret_cast<const unsigned char*>(&c);
				const GlyphInfo* g = &glyphs[ *uc ];
				if (g->descender < d) d = g->descender;
				if (multiLine < 2 && g->height > h) h = g->height; //! only calc height for the first line
		}
	}

	if (multiLine>1) d -= (multiLine-1) * lineHeight;
	if (descender) *descender = d;
	if (numLines) *numLines = multiLine;

	return h;
}


int CglFont::GetTextNumLines(const std::string& text) const
{
	if (text.empty())
		return 0;

	int lines = 1;

	for (int pos = 0 ; pos < text.length(); pos++) {
		const char& c = text[pos];
		switch(c) {
			//! inlined colorcode
			case ColorCodeIndicator:
				pos = SkipColorCodesOld(text, pos);
				if (pos<0) {
					pos = text.length();
				} else {
					pos--;
				}
				break;

			//! reset color
			case ColorResetIndicator:
				break;

			//! newline
			case '\x0d':
				if (pos+1 < text.length() && text[pos+1] == '\x0a')
					pos++;
			case '\x0a':
				lines++;
				break;

			//default:
		}
	}

	return lines;
}

/*******************************************************************************/
/*******************************************************************************/

/**
 * @brief IsUpperCase
 * @return true if the given uchar is an uppercase character (WinLatin charmap)
 */
static inline bool IsUpperCase(const unsigned char& c)
{
	return (c >= 0x41 && c <= 0x5A) ||
	       (c >= 0xC0 && c <= 0xD6) ||
	       (c >= 0xD8 && c <= 0xDE) ||
	       (c == 0x8A) ||
	       (c == 0x8C) ||
	       (c == 0x8E) ||
	       (c == 0x9F);
}


/**
 * @brief GetPenalty
 * @param c character at %strpos% in the word
 * @param strpos position of c in the word
 * @param strlen total length of the word
 * @return penalty (smaller is better) to split a word at that position
 */
static inline float GetPenalty(const unsigned char& c, unsigned int strpos, unsigned int strlen)
{
	const float dist = strlen - strpos;

	if (dist > (strlen / 2) && dist < 4) {
		return 1e9;
	} else if (c >= 0x61 && c <= 0x7A) {
		//! lowercase char
		return 1.0f + (strlen - strpos);
	} else if (c >= 0x30 && c <= 0x39) {
		//! is number
		return 1.0f + (strlen - strpos)*0.9;
	} else if (IsUpperCase(c)) {
		//! uppercase char
		return 1.0f + (strlen - strpos)*0.75;
	} else {
		//! any special chars
		return Square(dist / 4);
	}
}


CglFont::word CglFont::SplitWord(CglFont::word& w, float wantedWidth, bool smart) const
{
	//! returns two pieces 'L'eft and 'R'ight of the split word (returns L, *wi becomes R)

	word w2;
	w2.pos = w.pos;

	const float spaceAdvance = glyphs[0x20].advance;
	if (w.isLineBreak) {
		//! shouldn't happen
		w2 = w;
		w.isSpace = true;
	} else if (w.isSpace) {
		int split = (int)math::floor(wantedWidth / spaceAdvance);
		w2.isSpace   = true;
		w2.numSpaces = split;
		w2.width     = spaceAdvance * w2.numSpaces;
		w.numSpaces -= split;
		w.width      = spaceAdvance * w.numSpaces;
		w.pos       += split;
	} else {
		if (!smart) {
			float width = 0.0f;
			unsigned int i = 0;
			const unsigned char* c = reinterpret_cast<const unsigned char*>(&w.text[i]);
			do {
				const GlyphInfo& g = glyphs[*c];
				++i;

				if (i < w.text.length()) {
					c = reinterpret_cast<const unsigned char*>(&w.text[i]);
					width += g.kerning[*c];
				} else {
					width += g.kerning[0x20];
				}

				if (width > wantedWidth) {
					w2.text  = w.text.substr(0,i - 1);
					w2.width = GetTextWidth(w2.text);
					w.text.erase(0,i - 1);
					w.width  = GetTextWidth(w.text);
					w.pos   += i - 1;
					return w2;
				}
			} while(i < w.text.length());
		}

		if(
		   (wantedWidth < 8 * spaceAdvance) ||
		   (w.text.length() < 1)
		) {
			w2.isSpace = true;
			return w2;
		}

		float width = 0.0f;
		unsigned int i = 0;
		float min_penalty = 1e9;
		unsigned int goodbreak = 0;
		const unsigned char* c = reinterpret_cast<const unsigned char*>(&w.text[i]);
		do {
			const unsigned char* co = c;
			unsigned int io = i;
			const GlyphInfo& g = glyphs[*c];
			++i;

			if (i < w.text.length()) {
				c = reinterpret_cast<const unsigned char*>(&w.text[i]);
				width += g.kerning[*c];
			} else {
				width += g.kerning[0x20];
			}

			if (width > wantedWidth) {
				w2.text  = w.text.substr(0,goodbreak);
				w2.width = GetTextWidth(w2.text);
				w.text.erase(0,goodbreak);
				w.width  = GetTextWidth(w.text);
				w.pos   += goodbreak;
				break;
			}

			float penalty = GetPenalty(*co, io, w.text.length());
			if (penalty < min_penalty) {
				min_penalty = penalty;
				goodbreak   = i;
			}
		} while(i < w.text.length());
	}
	return w2;
}


void CglFont::AddEllipsis(std::list<line>& lines, std::list<word>& words, float maxWidth) const
{
	const float ellipsisAdvance = glyphs[0x85].advance;
	const float spaceAdvance = glyphs[0x20].advance;

	if (ellipsisAdvance > maxWidth)
		return;

	line* l = &(lines.back());

	//! If the last line ends with a linebreak, remove it
	std::list<word>::iterator wi_end = l->end;
	if (wi_end->isLineBreak) {
		if (l->start == l->end || l->end == words.begin()) {
			//! there is just the linebreak in that line, so replace linebreak with just a null space
			word w;
			w.pos       = wi_end->pos;
			w.isSpace   = true;
			w.numSpaces = 0;
			l->start = words.insert(wi_end,w);
			l->end = l->start;

			words.erase(wi_end);
		} else {
			wi_end = words.erase(wi_end);
			l->end = --wi_end;
		}
	}

	//! remove as many words until we have enough free space for the ellipsis
	while (l->end != l->start) {
		word& w = *l->end;

		//! we have enough free space
		if (l->width + ellipsisAdvance < maxWidth)
			break;

		//! we can cut the last word to get enough freespace (so show as many as possible characters of that word)
		if (
			((l->width - w.width + ellipsisAdvance) < maxWidth) &&
			(w.width > ellipsisAdvance)
		) {
			break;
		}

		l->width -= w.width;
		l->end--;
	}

	//! we don't even have enough space for the ellipsis
	word& w = *l->end;
	if ((l->width - w.width) + ellipsisAdvance > maxWidth)
		return;

	//! sometimes words aren't hyphenated for visual aspects
	//! but if we put an ellipsis in there, it is better to show as many as possible characters of those words
	std::list<word>::iterator nextwi(l->end); nextwi++;
	if (
	    (!l->forceLineBreak) &&
	    (nextwi != words.end()) &&
	    (w.isSpace || w.isLineBreak) &&
	    (l->width + ellipsisAdvance < maxWidth) &&
	    !(nextwi->isSpace || nextwi->isLineBreak)
	) {
		float spaceLeft = maxWidth - (l->width + ellipsisAdvance);
		l->end = words.insert( nextwi, SplitWord(*nextwi, spaceLeft, false) );
		l->width += l->end->width;
	}

	//! the last word in the line needs to be cut
	if (l->width + ellipsisAdvance > maxWidth) {
		word& w = *l->end;
		l->width -= w.width;
		float spaceLeft = maxWidth - (l->width + ellipsisAdvance);
		l->end = words.insert( l->end, SplitWord(w, spaceLeft, false) );
		l->width += l->end->width;
	}

	//! put in a space between words and the ellipsis (if there is enough space)
	if (l->forceLineBreak && !l->end->isSpace) {
		if (l->width + ellipsisAdvance + spaceAdvance <= maxWidth) {
			word space;
			space.isSpace = true;
			space.numSpaces = 1;
			space.width = spaceAdvance;
			std::list<word>::iterator wi(l->end);
			l->end++;
			if (l->end == words.end()) {
				space.pos = wi->pos + wi->text.length() + 1;
			} else {
				space.pos = l->end->pos;
			}
			l->end = words.insert( l->end, space );
			l->width += l->end->width;
		}
	}

	//! add our ellipsis
	word ellipsis;
	ellipsis.text  = "\x85";
	ellipsis.width = ellipsisAdvance;
	std::list<word>::iterator wi(l->end);
	l->end++;
	if (l->end == words.end()) {
		ellipsis.pos = wi->pos + wi->text.length() + 1;
	} else {
		ellipsis.pos = l->end->pos;
	}
	l->end = words.insert( l->end, ellipsis );
	l->width += l->end->width;
}


void CglFont::WrapTextConsole(std::list<word>& words, float maxWidth, float maxHeight) const
{
	if (words.empty())
		return;

	const bool splitAllWords = false;
	const unsigned int maxLines = (unsigned int)math::floor(std::max(0.0f, maxHeight / lineHeight ));

	line* currLine;
	word linebreak;
	linebreak.isLineBreak = true;

	bool addEllipsis = false;
	bool currLineValid = false; //! true if there was added any data to the current line

	std::list<word>::iterator wi = words.begin();

	std::list<word> splitWords;
	std::list<line> lines;
		lines.push_back(line());
		currLine = &(lines.back());
		currLine->start = words.begin();

	for (; ;) {
		currLineValid = true;
		if (wi->isLineBreak) {
			currLine->forceLineBreak = true;
			currLine->end = wi;

			//! start a new line after the '\n'
			lines.push_back(line());
				currLineValid = false;
				currLine = &(lines.back());
				currLine->start = wi;
				currLine->start++;
		} else {
			currLine->width += wi->width;
			currLine->end = wi;

			if (currLine->width > maxWidth) {
				currLine->width -= wi->width;

				//! line grew too long by adding the last word, insert a LineBreak
				const bool splitLastWord = (wi->width > (0.5 * maxWidth));
				const float freeWordSpace = (maxWidth - currLine->width);

				if (splitAllWords || splitLastWord) {
					//! last word W is larger than 0.5 * maxLineWidth, split it into
					//! get 'L'eft and 'R'ight parts of the split (wL becomes Left, *wi becomes R)

					//! turns *wi into R
					word wL = SplitWord(*wi, freeWordSpace);

					if (splitLastWord && wL.width == 0.0f) {
						//! With smart splitting it can happen that the word isn't splitted at all,
						//! this can cause a race condition when the word is longer than maxWidth.
						//! In this case we have to force an unaesthetic split.
						wL = SplitWord(*wi, freeWordSpace, false);
					}

					//! increase by the width of the L-part of *wi
					currLine->width += wL.width;

					//! insert the L-part right before R
					wi = words.insert(wi, wL);
					wi++;
				}

				//! insert the forced linebreak (either after W or before R)
				linebreak.pos = wi->pos;
				currLine->end = words.insert(wi, linebreak);

				while (wi != words.end() && wi->isSpace)
					wi = words.erase(wi);

				lines.push_back(line());
					currLineValid = false;
					currLine = &(lines.back());
					currLine->start = wi;
					wi--; //! compensate the wi++ downwards
			}
		}

		wi++;

		if (wi == words.end()) {
			break;
		}

		if (lines.size() > maxLines) {
			addEllipsis = true;
			break;
		}
	}

	

	//! empty row
	if (!currLineValid || (currLine->start == words.end() && !currLine->forceLineBreak)) {
		lines.pop_back();
		currLine = &(lines.back());
	}

	//! if we had to cut the text because of missing space, add an ellipsis
	if (addEllipsis)
		AddEllipsis(lines, words, maxWidth);

	wi = currLine->end; wi++;
	wi = words.erase(wi, words.end());
}


void CglFont::WrapTextKnuth(std::list<word>& words, float maxWidth, float maxHeight) const
{
	//todo: FINISH ME!!! (Knuths algorithm would try to share deadspace between lines, with the smallest sum of (deadspace of line)^2)
}


void CglFont::SplitTextInWords(const std::string& text, std::list<word>* words, std::list<colorcode>* colorcodes) const
{
	const unsigned int length = (unsigned int)text.length();
	const float spaceAdvance = glyphs[0x20].advance;

	words->push_back(word());
	word* w = &(words->back());

	unsigned int numChar = 0;
	for (int pos = 0; pos < length; pos++) {
		const char& c = text[pos];
		switch(c) {
			//! space
			case '\x20':
				if (!w->isSpace) {
					if (w->isSpace) {
						w->width = spaceAdvance * w->numSpaces;
					} else if (!w->isLineBreak) {
						w->width = GetTextWidth(w->text);
					}
					words->push_back(word());
					w = &(words->back());
					w->isSpace = true;
					w->pos     = numChar;
				}
				w->numSpaces++;
				break;

			//! inlined colorcodes
			case ColorCodeIndicator:
				{
					colorcodes->push_back(colorcode());
					colorcode& cc = colorcodes->back();
					cc.pos = numChar;
					SkipColorCodes(text, &pos, &(cc.color));
					if (pos<0) {
						pos = length;
					} else {
						//! SkipColorCodes jumps 1 too far (it jumps on the first non
						//! colorcode char, but our for-loop will still do "pos++;")
						pos--;
					}
				} break;
			case ColorResetIndicator:
				{
					colorcode* cc = &colorcodes->back();
					if (cc->pos != numChar) {
						colorcodes->push_back(colorcode());
						cc = &colorcodes->back();
						cc->pos = numChar;
					}
					cc->resetColor = true;
				} break;

			//! newlines
			case '\x0d': //! CR+LF
				if (pos+1 < length && text[pos+1] == '\x0a')
					pos++;
			case '\x0a': //! LF
				if (w->isSpace) {
					w->width = spaceAdvance * w->numSpaces;
				} else if (!w->isLineBreak) {
					w->width = GetTextWidth(w->text);
				}
				words->push_back(word());
				w = &(words->back());
				w->isLineBreak = true;
				w->pos = numChar;
				break;

			//! printable chars
			default:
				if (w->isSpace || w->isLineBreak) {
					if (w->isSpace) {
						w->width = spaceAdvance * w->numSpaces;
					} else if (!w->isLineBreak) {
						w->width = GetTextWidth(w->text);
					}
					words->push_back(word());
					w = &(words->back());
					w->pos = numChar;
				}
				w->text += c;
				numChar++;
		}
	}
	if (w->isSpace) {
		w->width = spaceAdvance * w->numSpaces;
	} else if (!w->isLineBreak) {
		w->width = GetTextWidth(w->text);
	}
}


void CglFont::RemergeColorCodes(std::list<word>* words, std::list<colorcode>& colorcodes) const
{
	std::list<word>::iterator wi = words->begin();
	std::list<word>::iterator wi2 = words->begin();
	std::list<colorcode>::iterator ci;
	for (ci = colorcodes.begin(); ci != colorcodes.end(); ++ci) {
		while(wi != words->end() && wi->pos <= ci->pos) {
			wi2 = wi;
			wi++;
		}

		word wc;
		wc.pos = ci->pos;
		wc.isColorCode = true;

		if (ci->resetColor) {
			wc.text = ColorResetIndicator;
		} else {
			wc.text = ColorCodeIndicator;
			wc.text += (unsigned char)(255 * ci->color[0]);
			wc.text += (unsigned char)(255 * ci->color[1]);
			wc.text += (unsigned char)(255 * ci->color[2]);
		}


		if (wi2->isSpace || wi2->isLineBreak) {
			while(wi2 != words->end() && (wi2->isSpace || wi2->isLineBreak))
				wi2++;

			if (wi == words->end() && (wi2->pos + wi2->numSpaces) < ci->pos) {
				return;
			}

			wi2 = words->insert(wi2, wc);
		} else {
			if (wi == words->end() && (wi2->pos + wi2->text.size()) < (ci->pos + 1)) {
				return;
			}

			size_t pos = ci->pos - wi2->pos;
			if (pos < wi2->text.size() && pos > 0) {
				word w2;
				w2.text = wi2->text.substr(0,pos);
				w2.pos = wi2->pos;
				wi2->text.erase(0,pos);
				wi2->pos += pos;
				wi2 = words->insert(wi2, wc);
				wi2 = words->insert(wi2, w2);
			} else {
				wi2 = words->insert(wi2, wc);
			}
		}
		wi = wi2;
		wi++;
	}
}


int CglFont::WrapInPlace(std::string& text, float _fontSize, const float maxWidth, const float maxHeight) const
{
	//todo: make an option to insert '-' for word wrappings (and perhaps try to syllabificate)

	if (_fontSize <= 0.0f)
		_fontSize = fontSize;

	const float maxWidthf  = maxWidth / _fontSize;
	const float maxHeightf = maxHeight / _fontSize;

	std::list<word> words;
	std::list<colorcode> colorcodes;

	SplitTextInWords(text, &words, &colorcodes);
	WrapTextConsole(words, maxWidthf, maxHeightf);
	//WrapTextKnuth(&lines, words, maxWidthf, maxHeightf);
	RemergeColorCodes(&words, colorcodes);

	//! create the wrapped string
	text = "";
	unsigned int numlines = 0;
	if (!words.empty()) {
		numlines++;
		for (std::list<word>::iterator wi = words.begin(); wi != words.end(); ++wi) {
			if (wi->isSpace) {
				for (unsigned int j = 0; j < wi->numSpaces; ++j) {
					text += " ";
				}
			} else if (wi->isLineBreak) {
				text += "\x0d\x0a";
				numlines++;
			} else {
				text += wi->text;
			}
		}
	}

	return numlines;
}


std::list<std::string> CglFont::Wrap(const std::string& text, float _fontSize, const float maxWidth, const float maxHeight) const
{
	//todo: make an option to insert '-' for word wrappings (and perhaps try to syllabificate)

	if (_fontSize <= 0.0f)
		_fontSize = fontSize;

	const float maxWidthf  = maxWidth / _fontSize;
	const float maxHeightf = maxHeight / _fontSize;

	std::list<word> words;
	std::list<colorcode> colorcodes;

	SplitTextInWords(text, &words, &colorcodes);
	WrapTextConsole(words, maxWidthf, maxHeightf);
	//WrapTextKnuth(&lines, words, maxWidthf, maxHeightf);
	RemergeColorCodes(&words, colorcodes);

	//! create the string lines of the wrapped text
	std::list<word>::iterator lastColorCode = words.end();
	std::list<std::string> strlines;
	if (!words.empty()) {
		strlines.push_back("");
		std::string* sl = &strlines.back();
		for (std::list<word>::iterator wi = words.begin(); wi != words.end(); ++wi) {
			if (wi->isSpace) {
				for (unsigned int j = 0; j < wi->numSpaces; ++j) {
					*sl += " ";
				}
			} else if (wi->isLineBreak) {
				strlines.push_back("");
				sl = &strlines.back();
				if (lastColorCode != words.end())
					*sl += lastColorCode->text;
			} else {
				*sl += wi->text;
				if (wi->isColorCode)
					lastColorCode = wi;
			}
		}
	}

	return strlines;
}


/*******************************************************************************/
/*******************************************************************************/

void CglFont::SetAutoOutlineColor(bool enable)
{
	autoOutlineColor = enable;
}


void CglFont::SetTextColor(const float4* color)
{
	if (color == NULL) color = &white;

	if (inBeginEnd && !(*color==textColor)) {
		if ((va->stripArrayPos - va->stripArray) != (va->drawArrayPos - va->drawArray)) {
			stripTextColors.push_back(*color);
			va->EndStrip();
		} else {
			float4& back = stripTextColors.back();
			back = *color;
		}
	}

	textColor = *color;
}


void CglFont::SetOutlineColor(const float4* color)
{
	if (color == NULL) color = ChooseOutlineColor(textColor);

	if (inBeginEnd && !(*color==outlineColor)) {
		if ((va2->stripArrayPos - va2->stripArray) != (va2->drawArrayPos - va2->drawArray)) {
			stripOutlineColors.push_back(*color);
			va2->EndStrip();
		} else {
			float4& back = stripOutlineColors.back();
			back = *color;
		}
	}

	outlineColor = *color;
}


void CglFont::SetColors(const float4* _textColor, const float4* _outlineColor)
{
	if (_textColor == NULL) _textColor = &white;
	if (_outlineColor == NULL) _outlineColor = ChooseOutlineColor(*_textColor);

	if (inBeginEnd) {
		if (!(*_textColor==textColor)) {
			if ((va->stripArrayPos - va->stripArray) != (va->drawArrayPos - va->drawArray)) {
				stripTextColors.push_back(*_textColor);
				va->EndStrip();
			} else {
				float4& back = stripTextColors.back();
				back = *_textColor;
			}
		}
		if (!(*_outlineColor==outlineColor)) {
			if ((va2->stripArrayPos - va2->stripArray) != (va2->drawArrayPos - va2->drawArray)) {
				stripOutlineColors.push_back(*_outlineColor);
				va2->EndStrip();
			} else {
				float4& back = stripOutlineColors.back();
				back = *_outlineColor;
			}
		}
	}

	textColor    = *_textColor;
	outlineColor = *_outlineColor;
}


const float4* CglFont::ChooseOutlineColor(const float4& textColor)
{
	const float luminosity = 0.05 +
		0.2126f * math::powf(textColor[0], 2.2) +
		0.7152f * math::powf(textColor[1], 2.2) +
		0.0722f * math::powf(textColor[2], 2.2);

	const float lumdiff = std::max(luminosity,darkLuminosity) / std::min(luminosity,darkLuminosity);
	if (lumdiff > 5.0f) {
		return &darkOutline;
	} else {
		return &lightOutline;
	}
}


void CglFont::Begin(const bool immediate, const bool resetColors)
{
	if (inBeginEnd) {
		logOutput.Print("FontError: called Begin() multiple times");
		return;
	}

	autoOutlineColor = true;

	setColor = !immediate;
	if (resetColors) {
		SetColors(); //! reset colors
	}

	inBeginEnd = true;

	va->Initialize();
	va2->Initialize();
	stripTextColors.clear();
	stripOutlineColors.clear();
	stripTextColors.push_back(textColor);
	stripOutlineColors.push_back(outlineColor);
}


void CglFont::End()
{
	if (!inBeginEnd) {
		logOutput.Print("FontError: called End() without Begin()");
		return;
	}
	inBeginEnd = false;

	if (va->drawIndex()==0) {
		return;
	}

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (va2->drawIndex() > 0) {
		if (stripOutlineColors.size() > 1) {
			ColorMap::iterator sci = stripOutlineColors.begin();
			va2->DrawArray2dT(GL_QUADS,TextStripCallback,&sci);
		} else {
			glColor4fv(outlineColor);
			va2->DrawArray2dT(GL_QUADS);
		}
	}

	if (stripTextColors.size() > 1) {
		ColorMap::iterator sci = stripTextColors.begin();
		va->DrawArray2dT(GL_QUADS,TextStripCallback,&sci);
	} else {
		if (setColor) glColor4fv(textColor);
		va->DrawArray2dT(GL_QUADS);
	}

	glPopAttrib();
}


void CglFont::RenderString(float x, float y, const float& scaleX, const float& scaleY, const std::string& str)
{
	/**
	 * NOTE:
	 * Font rendering does not use display lists, but VAs. It's actually faster
	 * (450% faster with a 7600GT!) for these reasons:
	 *
	 * 1. When using DLs, we can not group multiple glyphs into one glBegin/End pair
	 *    because glTranslatef can not go between such a pair.
	 * 2. We can now eliminate all glPushMatrix/PopMatrix pairs related to font rendering
	 *    because the transformations are calculated on the fly. These are just a couple of
	 *    floating point multiplications and shouldn't be too expensive.
	 */

	const float startx = x;
	const float lineHeight_ = scaleY * lineHeight;
	unsigned int length = (unsigned int)str.length();

	va->EnlargeArrays(length * 16 * sizeof(float), 0);

	int skippedLines;
	bool endOfString, colorChanged;
	const GlyphInfo* g = NULL;
	const unsigned char* c;
	float4 newColor; newColor[3] = 1.0f;
	unsigned int i = 0;

	do {
		endOfString = SkipColorCodesAndNewLines(str, &i, &newColor, &colorChanged, &skippedLines, &baseTextColor);

		if (endOfString)
			return;

		c  = reinterpret_cast<const unsigned char*>(&str[i]);
		++i;

		if (colorChanged) {
			if (autoOutlineColor) {
				SetColors(&newColor,NULL);
			} else {
				SetTextColor(&newColor);
			}
		}

		if (skippedLines>0) {
			x  = startx;
			y -= skippedLines * lineHeight_;
		} else if (g) {
			x += scaleX * g->kerning[*c];
		}

		g = &glyphs[*c];

		va->AddVertex2dQT(x+scaleX*g->x0, y+scaleY*g->y1, g->u0, g->v1);
		va->AddVertex2dQT(x+scaleX*g->x0, y+scaleY*g->y0, g->u0, g->v0);
		va->AddVertex2dQT(x+scaleX*g->x1, y+scaleY*g->y0, g->u1, g->v0);
		va->AddVertex2dQT(x+scaleX*g->x1, y+scaleY*g->y1, g->u1, g->v1);
	} while(true);
}


void CglFont::RenderStringShadow(float x, float y, const float& scaleX, const float& scaleY, const std::string& str)
{
	const float shiftX = scaleX*0.1, shiftY = scaleY*0.1;
	const float ssX = (scaleX/fontSize)*outlineWidth, ssY = (scaleY/fontSize)*outlineWidth;

	const float startx = x;
	const float lineHeight_ = scaleY * lineHeight;
	unsigned int length = (unsigned int)str.length();

	va->EnlargeArrays(length * 16 * sizeof(float), 0);
	va2->EnlargeArrays(length * 16 * sizeof(float), 0);

	int skippedLines;
	bool endOfString, colorChanged;
	const GlyphInfo* g = NULL;
	const unsigned char* c;
	float4 newColor; newColor[3] = 1.0f;
	unsigned int i = 0;

	do {
		endOfString = SkipColorCodesAndNewLines(str, &i, &newColor, &colorChanged, &skippedLines, &baseTextColor);

		if (endOfString)
			return;

		c  = reinterpret_cast<const unsigned char*>(&str[i]);
		++i;

		if (colorChanged) {
			if (autoOutlineColor) {
				SetColors(&newColor,NULL);
			} else {
				SetTextColor(&newColor);
			}
		}

		if (skippedLines>0) {
			x  = startx;
			y -= skippedLines * lineHeight_;
		} else if (g) {
			x += scaleX * g->kerning[*c];
		}

		g = &glyphs[*c];

		const float dx0 = x + scaleX * g->x0, dy0 = y + scaleY * g->y0;
		const float dx1 = x + scaleX * g->x1, dy1 = y + scaleY * g->y1;

		//! draw shadow
		va2->AddVertex2dQT(dx0+shiftX-ssX, dy1-shiftY-ssY, g->us0, g->vs1);
		va2->AddVertex2dQT(dx0+shiftX-ssX, dy0-shiftY+ssY, g->us0, g->vs0);
		va2->AddVertex2dQT(dx1+shiftX+ssX, dy0-shiftY+ssY, g->us1, g->vs0);
		va2->AddVertex2dQT(dx1+shiftX+ssX, dy1-shiftY-ssY, g->us1, g->vs1);

		//! draw the actual character
		va->AddVertex2dQT(dx0, dy1, g->u0, g->v1);
		va->AddVertex2dQT(dx0, dy0, g->u0, g->v0);
		va->AddVertex2dQT(dx1, dy0, g->u1, g->v0);
		va->AddVertex2dQT(dx1, dy1, g->u1, g->v1);
	} while(true);
}


void CglFont::RenderStringOutlined(float x, float y, const float& scaleX, const float& scaleY, const std::string& str)
{
	const float shiftX = (scaleX/fontSize)*outlineWidth, shiftY = (scaleY/fontSize)*outlineWidth;

	const float startx = x;
	const float lineHeight_ = scaleY * lineHeight;
	unsigned int length = (unsigned int)str.length();

	va->EnlargeArrays(length * 16 * sizeof(float), 0);
	va2->EnlargeArrays(length * 16 * sizeof(float), 0);

	int skippedLines;
	bool endOfString, colorChanged;
	const GlyphInfo* g = NULL;
	const unsigned char* c;
	float4 newColor; newColor[3] = 1.0f;
	unsigned int i = 0;

	do {
		endOfString = SkipColorCodesAndNewLines(str, &i, &newColor, &colorChanged, &skippedLines, &baseTextColor);

		if (endOfString)
			return;

		c  = reinterpret_cast<const unsigned char*>(&str[i]);
		++i;

		if (colorChanged) {
			if (autoOutlineColor) {
				SetColors(&newColor,NULL);
			} else {
				SetTextColor(&newColor);
			}
		}

		if (skippedLines>0) {
			x  = startx;
			y -= skippedLines * lineHeight_;
		} else if (g) {
			x += scaleX * g->kerning[*c];
		}

		g = &glyphs[*c];

		const float dx0 = x + scaleX * g->x0, dy0 = y + scaleY * g->y0;
		const float dx1 = x + scaleX * g->x1, dy1 = y + scaleY * g->y1;

		//! draw outline
		va2->AddVertex2dQT(dx0-shiftX, dy1-shiftY, g->us0, g->vs1);
		va2->AddVertex2dQT(dx0-shiftX, dy0+shiftY, g->us0, g->vs0);
		va2->AddVertex2dQT(dx1+shiftX, dy0+shiftY, g->us1, g->vs0);
		va2->AddVertex2dQT(dx1+shiftX, dy1-shiftY, g->us1, g->vs1);

		//! draw the actual character
		va->AddVertex2dQT(dx0, dy1, g->u0, g->v1);
		va->AddVertex2dQT(dx0, dy0, g->u0, g->v0);
		va->AddVertex2dQT(dx1, dy0, g->u1, g->v0);
		va->AddVertex2dQT(dx1, dy1, g->u1, g->v1);
	} while(true);
}


void CglFont::glWorldPrint(const float3 p, const float size, const std::string& str)
{

	glPushMatrix();
		glTranslatef(p.x, p.y, p.z);
		glCallList(CCamera::billboardList);
		Begin(false,false);
			glPrint(0.0f, 0.0f, size, FONT_DESCENDER | FONT_CENTER | FONT_OUTLINE, str);
		End();
	glPopMatrix();
}


void CglFont::glPrint(float x, float y, float s, const int& options, const std::string& text)
{
	//! s := scale or absolute size?
	if (options & FONT_SCALE) {
		s *= fontSize;
	}

	float sizeX = s, sizeY = s;

	//! render in normalized coords (0..1) instead of screencoords (0..~1024)
	if (options & FONT_NORM) {
		sizeX *= globalRendering->pixelX;
		sizeY *= globalRendering->pixelY;
	}

	//! horizontal alignment (FONT_LEFT is default)
	if (options & FONT_CENTER) {
		x -= sizeX * 0.5f * GetTextWidth(text);
	} else if (options & FONT_RIGHT) {
		x -= sizeX * GetTextWidth(text);
	}


	//! vertical alignment
	y += sizeY * fontDescender; //! move to baseline (note: descender is negative)
	if (options & FONT_BASELINE) {
		//! nothing
	} else if (options & FONT_DESCENDER) {
		y -= sizeY * fontDescender;
	} else if (options & FONT_VCENTER) {
		float textDescender;
		y -= sizeY * 0.5f * GetTextHeight(text,&textDescender);
		y -= sizeY * 0.5f * textDescender;
	} else if (options & FONT_TOP) {
		y -= sizeY * GetTextHeight(text);
	} else if (options & FONT_ASCENDER) {
		y -= sizeY * fontDescender;
		y -= sizeY;
	} else if (options & FONT_BOTTOM) {
		float textDescender;
		GetTextHeight(text,&textDescender);
		y -= sizeY * textDescender;
	}

	if (options & FONT_NEAREST) {
		x = (int)x;
		y = (int)y;
	}

	//! backup text & outline colors (also ::ColorResetIndicator will reset to those)
	baseTextColor = textColor;
	baseOutlineColor = outlineColor;

	//! immediate mode?
	const bool immediate = !inBeginEnd;
	if (immediate) {
		Begin(!(options & (FONT_OUTLINE | FONT_SHADOW)));
	}


	//! select correct decoration RenderString function
	if (options & FONT_OUTLINE) {
		RenderStringOutlined(x, y, sizeX, sizeY, text);
	} else if (options & FONT_SHADOW) {
		RenderStringShadow(x, y, sizeX, sizeY, text);
	} else {
		RenderString(x, y, sizeX, sizeY, text);
	}


	//! immediate mode?
	if (immediate) {
		End();
	}

	//! reset text & outline colors (if changed via in text colorcodes)
	SetColors(&baseTextColor,&baseOutlineColor);
}

void CglFont::glPrintTable(float x, float y, float s, const int& options, const std::string& text) {
	int col = 0;
	int row = 0;
	std::vector<std::string> coltext;
	std::vector<int> coldata;
	coltext.reserve(text.length());
	coltext.push_back("");
	unsigned char curcolor[4];
	unsigned char defaultcolor[4];
	defaultcolor[0] = ColorCodeIndicator;
	for(int i = 0; i < 3; ++i)
		defaultcolor[i+1] = (unsigned char)(textColor[i]*255.0f);
	coldata.push_back(*(int *)&defaultcolor);
	for(int i = 0; i < 4; ++i)
		curcolor[i] = defaultcolor[i];

	for (int pos = 0; pos < text.length(); pos++) {
		const char& c = text[pos];
		switch(c) {
			//! inline colorcodes
			case ColorCodeIndicator:
				for(int i = 0; i < 4 && pos < text.length(); ++i, ++pos) {
					coltext[col] += text[pos];
					((unsigned char *)curcolor)[i] = text[pos];
				}
				coldata[col] = *(int *)curcolor;
				--pos;
				break;

			//! column separator is `\t`==`horizontal tab`
			case '\x09':
				++col;
				if(col >= coltext.size()) {
					coltext.push_back("");
					for(int i = 0; i < row; ++i)
						coltext[col] += '\x0a';
					coldata.push_back(*(int *)&defaultcolor);
				}
				if(coldata[col] != *(int *)curcolor) {
					for(int i = 0; i < 4; ++i)
						coltext[col] += curcolor[i];
					coldata[col] = *(int *)curcolor;
				}
				break;

			//! newline
			case '\x0d': //! CR+LF
				if (pos+1 < text.length() && text[pos + 1] == '\x0a')
					pos++;
			case '\x0a': //! LF
				for(int i = 0; i < coltext.size(); ++i)
					coltext[i] += '\x0a';
				if(coldata[0] != *(int *)curcolor) {
					for(int i = 0; i < 4; ++i)
						coltext[0] += curcolor[i];
					coldata[0] = *(int *)curcolor;
				}
				col = 0;
				++row;
				break;

			//! printable char
			default:
				coltext[col] += c;
		}
	}

	float totalWidth = 0.0f;
	float maxHeight = 0.0f;
	float minDescender = 0.0f;
	for(int i = 0; i < coltext.size(); ++i) {
		float colwidth = GetTextWidth(coltext[i]);
		coldata[i] = *(int *)&colwidth;
		totalWidth += colwidth;
		float textDescender;
		float textHeight = GetTextHeight(coltext[i], &textDescender);
		if(textHeight > maxHeight)
			maxHeight = textHeight;
		if(textDescender < minDescender)
			minDescender = textDescender;
	}

	//! s := scale or absolute size?
	float ss = s;
	if (options & FONT_SCALE) {
		ss *= fontSize;
	}

	float sizeX = ss, sizeY = ss;

	//! render in normalized coords (0..1) instead of screencoords (0..~1024)
	if (options & FONT_NORM) {
		sizeX *= globalRendering->pixelX;
		sizeY *= globalRendering->pixelY;
	}

	//! horizontal alignment (FONT_LEFT is default)
	if (options & FONT_CENTER) {
		x -= sizeX * 0.5f * totalWidth;
	} else if (options & FONT_RIGHT) {
		x -= sizeX * totalWidth;
	}

	//! vertical alignment
	if (options & FONT_BASELINE) {
		//! nothing
	} else if (options & FONT_DESCENDER) {
		y -= sizeY * fontDescender;
	} else if (options & FONT_VCENTER) {
		y -= sizeY * 0.5f * maxHeight;
		y -= sizeY * 0.5f * minDescender;
	} else if (options & FONT_TOP) {
		y -= sizeY * maxHeight;
	} else if (options & FONT_ASCENDER) {
		y -= sizeY * fontDescender;
		y -= sizeY;
	} else if (options & FONT_BOTTOM) {
		y -= sizeY * minDescender;
	}

	for(int i = 0; i < coltext.size(); ++i) {
		glPrint(x, y, s, (options | FONT_BASELINE) & ~(FONT_RIGHT | FONT_CENTER), coltext[i]);
		int colwidth = coldata[i];
		x += sizeX * *(float *)&colwidth;
	}
}


//! macro for formatting printf-style
#define FORMAT_STRING(lastarg,fmt,out)                         \
		char out[512];                         \
		va_list ap;                            \
		if (fmt == NULL) return;               \
		va_start(ap, lastarg);                 \
		VSNPRINTF(out, sizeof(out), fmt, ap);  \
		va_end(ap);

void CglFont::glFormat(float x, float y, float s, const int& options, const char* fmt, ...)
{
	FORMAT_STRING(fmt,fmt,text);
	glPrint(x, y, s, options, string(text));
}


void CglFont::glFormat(float x, float y, float s, const int& options, const string& fmt, ...)
{
	FORMAT_STRING(fmt,fmt.c_str(),text);
	glPrint(x, y, s, options, string(text));
}
