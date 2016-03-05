/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "glFont.h"
#include "FontLogSection.h"
#include <stdarg.h>
#include <stdexcept>

#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VertexArray.h"
#include "System/Log/ILog.h"
#include "System/Color.h"
#include "System/Exceptions.h"
#include "System/myMath.h"
#include "System/Util.h"

#undef GetCharWidth // winapi.h


bool CglFont::threadSafety = false;

/*******************************************************************************/
/*******************************************************************************/

CglFont* font = nullptr;
CglFont* smallFont = nullptr;

static const unsigned char nullChar = 0;
static const float4        white(1.00f, 1.00f, 1.00f, 0.95f);
static const float4  darkOutline(0.05f, 0.05f, 0.05f, 0.95f);
static const float4 lightOutline(0.95f, 0.95f, 0.95f, 0.8f);

static const float darkLuminosity = 0.05 +
	0.2126f * std::pow(darkOutline[0], 2.2) +
	0.7152f * std::pow(darkOutline[1], 2.2) +
	0.0722f * std::pow(darkOutline[2], 2.2);

/*******************************************************************************/
/*******************************************************************************/

CglFont::CglFont(const std::string& fontfile, int size, int _outlinewidth, float _outlineweight)
: CTextWrap(fontfile,size,_outlinewidth,_outlineweight)
, fontPath(fontfile)
, inBeginEnd(false)
, autoOutlineColor(true)
, setColor(false)
{
	textColor    = white;
	outlineColor = darkOutline;
}

CglFont* CglFont::LoadFont(const std::string& fontFile, int size, int outlinewidth, float outlineweight)
{
	try {
		CglFont* newFont = new CglFont(fontFile, size, outlinewidth, outlineweight);
		return newFont;
	} catch (const content_error& ex) {
		LOG_L(L_ERROR, "Failed creating font: %s", ex.what());
		return NULL;
	}
}


CglFont::~CglFont()
{ }


/*******************************************************************************/
/*******************************************************************************/

template <typename T>
static inline int SkipColorCodes(const std::u8string& text, T pos)
{
	while (text[pos] == CglFont::ColorCodeIndicator) {
		pos += 4;
		if (pos >= text.size()) { return -1; }
	}
	return pos;
}


template <typename T>
static inline bool SkipColorCodesAndNewLines(const std::u8string& text, T* pos, float4* color, bool* colorChanged, int* skippedLines, float4* colorReset)
{
	const size_t length = text.length();
	(*colorChanged) = false;
	(*skippedLines) = 0;
	while (*pos < length) {
		const char8_t& chr = text[*pos];
		switch(chr) {
			case CglFont::ColorCodeIndicator:
				*pos += 4;
				if ((*pos) < length) {
					(*color)[0] = text[(*pos) - 3] / 255.0f;
					(*color)[1] = text[(*pos) - 2] / 255.0f;
					(*color)[2] = text[(*pos) - 1] / 255.0f;
					*colorChanged = true;
				}
				break;

			case CglFont::ColorResetIndicator:
				(*pos)++;
				(*color) = *colorReset;
				*colorChanged = true;
				break;

			case 0x0d: // CR
				(*skippedLines)++;
				(*pos)++;
				if (*pos < length && text[*pos] == 0x0a) { // CR+LF
					(*pos)++;
				}
				break;

			case 0x0a: // LF
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

std::string CglFont::StripColorCodes_(const std::u8string& text)
{
	const size_t len = text.size();

	std::string nocolor;
	nocolor.reserve(len);
	for (int i = 0; i < len; i++) {
		if (text[i] == ColorCodeIndicator) {
			i += 3;
		} else {
			nocolor += text[i];
		}
	}
	return nocolor;
}


float CglFont::GetCharacterWidth(const char32_t c)
{
	return GetGlyph(c).advance;
}


float CglFont::GetTextWidth_(const std::u8string& text)
{
	if (text.empty()) return 0.0f;

	float w = 0.0f;
	float maxw = 0.0f;

	const GlyphInfo* prv_g=NULL;
	const GlyphInfo* cur_g;

	int pos = 0;
	while (pos < text.length()) {
		const char32_t u = Utf8GetNextChar(text, pos);

		switch (u) {
			// inlined colorcode
			case ColorCodeIndicator:
				pos = SkipColorCodes(text, pos - 1);
				if (pos<0) {
					pos = text.length();
				}
				break;

			// reset color
			case ColorResetIndicator:
				break;

			// newline
			case 0x0d: // CR+LF
				if (pos < text.length() && text[pos] == 0x0a)
					pos++;
			case 0x0a: // LF
				if (prv_g)
					w += prv_g->advance;
				if (w > maxw)
					maxw = w;
				w = 0.0f;
				prv_g = NULL;
				break;

			// printable char
			default:
				cur_g = &GetGlyph(u);
				if (prv_g) w += GetKerning(*prv_g, *cur_g);
				prv_g = cur_g;
		}
	}

	if (prv_g)
		w += prv_g->advance;
	if (w > maxw)
		maxw = w;

	return maxw;
}


float CglFont::GetTextHeight_(const std::u8string& text, float* descender, int* numLines)
{
	if (text.empty()) {
		if (descender) *descender = 0.0f;
		if (numLines) *numLines = 0;
		return 0.0f;
	}

	float h = 0.0f, d = GetLineHeight() + GetDescender();
	unsigned int multiLine = 1;

	int pos = 0;
	while (pos < text.length()) {
		const char32_t u = Utf8GetNextChar(text, pos);
		switch(u) {
			// inlined colorcode
			case ColorCodeIndicator:
				pos = SkipColorCodes(text, pos - 1);
				if (pos<0) {
					pos = text.length();
				}
				break;

			// reset color
			case ColorResetIndicator:
				break;

			// newline
			case 0x0d: // CR+LF
				if (pos < text.length() && text[pos] == 0x0a)
					++pos;
			case 0x0a: // LF
				multiLine++;
				d = GetLineHeight() + GetDescender();
				break;

			// printable char
			default:
				const GlyphInfo& g = GetGlyph(u);
				if (g.descender < d) d = g.descender;
				if (multiLine < 2 && g.height > h) h = g.height; // only calc height for the first line
		}
	}

	if (multiLine>1) d -= (multiLine-1) * GetLineHeight();
	if (descender) *descender = d;
	if (numLines) *numLines = multiLine;

	return h;
}


int CglFont::GetTextNumLines_(const std::u8string& text)
{
	if (text.empty())
		return 0;

	int lines = 1;

	for (int pos = 0 ; pos < text.length(); pos++) {
		const char8_t& c = text[pos];
		switch(c) {
			// inlined colorcode
			case ColorCodeIndicator:
				pos = SkipColorCodes(text, pos);
				if (pos<0) {
					pos = text.length();
				} else {
					pos--;
				}
				break;

			// reset color
			case ColorResetIndicator:
				break;

			// newline
			case 0x0d:
				if (pos+1 < text.length() && text[pos+1] == 0x0a)
					pos++;
			case 0x0a:
				lines++;
				break;

			//default:
		}
	}

	return lines;
}


std::list<std::string> CglFont::SplitIntoLines(const std::u8string& text)
{
	std::list<std::string> lines;

	if (text.empty())
		return lines;

	lines.push_back("");
	std::list<std::string> colorCodeStack;
	for (int pos = 0 ; pos < text.length(); pos++) {
		const char8_t& c = text[pos];
		switch(c) {
			// inlined colorcode
			case ColorCodeIndicator:
				if ((pos + 3) < text.length()) {
					colorCodeStack.push_back(text.substr(pos, 4));
					lines.back() += colorCodeStack.back();
					pos += 3;
				}
				break;

			// reset color
			case ColorResetIndicator:
				colorCodeStack.pop_back();
				lines.back() += c;
				break;

			// newline
			case 0x0d:
				if (pos+1 < text.length() && text[pos+1] == 0x0a)
					pos++;
			case 0x0a:
				lines.push_back("");
				for (auto& color: colorCodeStack)
					lines.back() = color;
				break;

			default:
				lines.back() += c;
		}
	}

	return lines;
}


/*******************************************************************************/
/*******************************************************************************/

void CglFont::SetAutoOutlineColor(bool enable)
{
	if (threadSafety)
		vaMutex.lock();
	autoOutlineColor = enable;
	if (threadSafety)
		vaMutex.unlock();
}


void CglFont::SetTextColor(const float4* color)
{
	if (color == NULL) color = &white;

	if (threadSafety)
		vaMutex.lock();
	if (inBeginEnd && !(*color==textColor)) {
		if (va.drawIndex() == 0 && !stripTextColors.empty()) {
			stripTextColors.back() = *color;
		} else {
			stripTextColors.push_back(*color);
			va.EndStrip();
		}
	}

	textColor = *color;
	if (threadSafety)
		vaMutex.unlock();
}


void CglFont::SetOutlineColor(const float4* color)
{
	if (color == NULL) color = ChooseOutlineColor(textColor);

	if (threadSafety)
		vaMutex.lock();
	if (inBeginEnd && !(*color==outlineColor)) {
		if (va2.drawIndex() == 0 && !stripOutlineColors.empty()) {
			stripOutlineColors.back() = *color;
		} else {
			stripOutlineColors.push_back(*color);
			va2.EndStrip();
		}
	}

	outlineColor = *color;
	if (threadSafety)
		vaMutex.unlock();
}


void CglFont::SetColors(const float4* _textColor, const float4* _outlineColor)
{
	SetTextColor(_textColor);
	SetOutlineColor(_outlineColor);
}


const float4* CglFont::ChooseOutlineColor(const float4& textColor)
{
	const float luminosity = 0.05 +
				 0.2126f * std::pow(textColor[0], 2.2) +
				 0.7152f * std::pow(textColor[1], 2.2) +
				 0.0722f * std::pow(textColor[2], 2.2);

	const float lumdiff = std::max(luminosity,darkLuminosity) / std::min(luminosity,darkLuminosity);
	if (lumdiff > 5.0f) {
		return &darkOutline;
	} else {
		return &lightOutline;
	}
}


/*******************************************************************************/
/*******************************************************************************/

void CglFont::Begin(const bool immediate, const bool resetColors)
{
	if (threadSafety)
		vaMutex.lock();

	if (inBeginEnd) {
		LOG_L(L_ERROR, "called Begin() multiple times");
		if (threadSafety)
			vaMutex.unlock();
		return;
	}


	autoOutlineColor = true;

	setColor = !immediate;
	if (resetColors) {
		SetColors(); // reset colors
	}

	inBeginEnd = true;

	va.Initialize();
	va2.Initialize();
	stripTextColors.clear();
	stripOutlineColors.clear();
	stripTextColors.push_back(textColor);
	stripOutlineColors.push_back(outlineColor);

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void CglFont::End()
{
	if (!inBeginEnd) {
		LOG_L(L_ERROR, "called End() without Begin()");
		return;
	}
	inBeginEnd = false;

	if (va.drawIndex() == 0) {
		glPopAttrib();
		if (threadSafety)
			vaMutex.unlock();
		return;
	}

	GLboolean inListCompile;
	glGetBooleanv(GL_LIST_INDEX, &inListCompile);
	if (!inListCompile) {
		UpdateTexture();
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, GetTexture());

	// Because texture size can change, texture coordinats are absolute in texels.
	// We could use also just use GL_TEXTURE_RECTANGLE
	// but then all shaders would need to detect so and use different funcs & types if supported -> more work
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glCallList(textureSpaceMatrix);
	glMatrixMode(GL_MODELVIEW);

	if (va2.drawIndex() > 0) {
		if (stripOutlineColors.size() > 1) {
			ColorMap::iterator sci = stripOutlineColors.begin();
			va2.DrawArray2dT(GL_QUADS,TextStripCallback,&sci);
		} else {
			glColor4fv(outlineColor);
			va2.DrawArray2dT(GL_QUADS);
		}
	}

	if (stripTextColors.size() > 1) {
		ColorMap::iterator sci = stripTextColors.begin();
		va.DrawArray2dT(GL_QUADS,TextStripCallback,&sci);//FIXME calls a 0 length strip!
	} else {
		if (setColor) glColor4fv(textColor);
		va.DrawArray2dT(GL_QUADS);
	}

	// pop texture matrix
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glPopAttrib();
	if (threadSafety)
		vaMutex.unlock();
}


/*******************************************************************************/
/*******************************************************************************/

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
	const float lineHeight_ = scaleY * GetLineHeight();
	unsigned int length = (unsigned int)str.length();
	const std::u8string& ustr = toustring(str);

	va.EnlargeArrays(length * 4, 0, VA_SIZE_2DT);

	int skippedLines;
	bool colorChanged;
	const GlyphInfo* g = NULL;
	float4 newColor = textColor;
	char32_t c;
	int i = 0;

	do {
		const bool endOfString = SkipColorCodesAndNewLines(ustr, &i, &newColor, &colorChanged, &skippedLines, &baseTextColor);

		if (endOfString)
			return;

		c = Utf8GetNextChar(str,i);

		if (colorChanged) {
			if (autoOutlineColor) {
				SetColors(&newColor,NULL);
			} else {
				SetTextColor(&newColor);
			}
		}
		const GlyphInfo* c_g = &GetGlyph(c);
		if (skippedLines>0) {
			x  = startx;
			y -= skippedLines * lineHeight_;
		} else if (g) {
			x += scaleX * GetKerning(*g, *c_g);
		}

		g = c_g;

		const auto&  tc = g->texCord;
		const float dx0 = (scaleX * g->size.x0()) + x, dy0 = (scaleY * g->size.y0()) + y;
		const float dx1 = (scaleX * g->size.x1()) + x, dy1 = (scaleY * g->size.y1()) + y;

		va.AddVertexQ2dT(dx0, dy1, tc.x0(), tc.y1());
		va.AddVertexQ2dT(dx0, dy0, tc.x0(), tc.y0());
		va.AddVertexQ2dT(dx1, dy0, tc.x1(), tc.y0());
		va.AddVertexQ2dT(dx1, dy1, tc.x1(), tc.y1());
	} while(true);
}


void CglFont::RenderStringShadow(float x, float y, const float& scaleX, const float& scaleY, const std::string& str)
{
	const float shiftX = scaleX*0.1, shiftY = scaleY*0.1;
	const float ssX = (scaleX/fontSize) * GetOutlineWidth(), ssY = (scaleY/fontSize) * GetOutlineWidth();

	const float startx = x;
	const float lineHeight_ = scaleY * GetLineHeight();
	unsigned int length = (unsigned int)str.length();
	const std::u8string& ustr = toustring(str);

	va.EnlargeArrays(length * 4, 0, VA_SIZE_2DT);
	va2.EnlargeArrays(length * 4, 0, VA_SIZE_2DT);

	int skippedLines;
	bool colorChanged;
	const GlyphInfo* g = NULL;
	float4 newColor = textColor;
	char32_t c;
	int i = 0;

	do {
		const bool endOfString = SkipColorCodesAndNewLines(ustr, &i, &newColor, &colorChanged, &skippedLines, &baseTextColor);

		if (endOfString)
			return;

		c = Utf8GetNextChar(str,i);

		if (colorChanged) {
			if (autoOutlineColor) {
				SetColors(&newColor,NULL);
			} else {
				SetTextColor(&newColor);
			}
		}

		const GlyphInfo* c_g = &GetGlyph(c);
		if (skippedLines>0) {
			x  = startx;
			y -= skippedLines * lineHeight_;
		} else if (g) {
			x += scaleX * GetKerning(*g, *c_g);
		}

		g = c_g;

		const auto&  tc = g->texCord;
		const auto& stc = g->shadowTexCord;
		const float dx0 = (scaleX * g->size.x0()) + x, dy0 = (scaleY * g->size.y0()) + y;
		const float dx1 = (scaleX * g->size.x1()) + x, dy1 = (scaleY * g->size.y1()) + y;

		// draw shadow
		va2.AddVertexQ2dT(dx0+shiftX-ssX, dy1-shiftY-ssY, stc.x0(), stc.y1());
		va2.AddVertexQ2dT(dx0+shiftX-ssX, dy0-shiftY+ssY, stc.x0(), stc.y0());
		va2.AddVertexQ2dT(dx1+shiftX+ssX, dy0-shiftY+ssY, stc.x1(), stc.y0());
		va2.AddVertexQ2dT(dx1+shiftX+ssX, dy1-shiftY-ssY, stc.x1(), stc.y1());

		// draw the actual character
		va.AddVertexQ2dT(dx0, dy1, tc.x0(), tc.y1());
		va.AddVertexQ2dT(dx0, dy0, tc.x0(), tc.y0());
		va.AddVertexQ2dT(dx1, dy0, tc.x1(), tc.y0());
		va.AddVertexQ2dT(dx1, dy1, tc.x1(), tc.y1());
	} while(true);
}

void CglFont::RenderStringOutlined(float x, float y, const float& scaleX, const float& scaleY, const std::string& str)
{
	const float shiftX = (scaleX/fontSize) * GetOutlineWidth(), shiftY = (scaleY/fontSize) * GetOutlineWidth();

	const float startx = x;
	const float lineHeight_ = scaleY * GetLineHeight();
	const std::u8string& ustr = toustring(str);
	const size_t length = str.length();

	va.EnlargeArrays(length * 4, 0, VA_SIZE_2DT);
	va2.EnlargeArrays(length * 4, 0, VA_SIZE_2DT);

	int skippedLines;
	bool colorChanged;
	const GlyphInfo* g = NULL;
	float4 newColor = textColor;
	char32_t c;
	int i = 0;

	do {
		const bool endOfString = SkipColorCodesAndNewLines(ustr, &i, &newColor, &colorChanged, &skippedLines, &baseTextColor);

		if (endOfString)
			return;

		c = Utf8GetNextChar(str,i);

		if (colorChanged) {
			if (autoOutlineColor) {
				SetColors(&newColor,NULL);
			} else {
				SetTextColor(&newColor);
			}
		}

		const GlyphInfo* c_g = &GetGlyph(c);
		if (skippedLines>0) {
			x  = startx;
			y -= skippedLines * lineHeight_;
		} else if (g) {
			x += scaleX * GetKerning(*g, *c_g);
		}

		g = c_g;

		const auto&  tc = g->texCord;
		const auto& stc = g->shadowTexCord;
		const float dx0 = (scaleX * g->size.x0()) + x, dy0 = (scaleY * g->size.y0()) + y;
		const float dx1 = (scaleX * g->size.x1()) + x, dy1 = (scaleY * g->size.y1()) + y;

		// draw outline
		va2.AddVertexQ2dT(dx0-shiftX, dy1-shiftY, stc.x0(), stc.y1());
		va2.AddVertexQ2dT(dx0-shiftX, dy0+shiftY, stc.x0(), stc.y0());
		va2.AddVertexQ2dT(dx1+shiftX, dy0+shiftY, stc.x1(), stc.y0());
		va2.AddVertexQ2dT(dx1+shiftX, dy1-shiftY, stc.x1(), stc.y1());

		// draw the actual character
		va.AddVertexQ2dT(dx0, dy1, tc.x0(), tc.y1());
		va.AddVertexQ2dT(dx0, dy0, tc.x0(), tc.y0());
		va.AddVertexQ2dT(dx1, dy0, tc.x1(), tc.y0());
		va.AddVertexQ2dT(dx1, dy1, tc.x1(), tc.y1());
	} while(true);
}


void CglFont::glWorldPrint(const float3& p, const float size, const std::string& str)
{
	glPushMatrix();
	glTranslatef(p.x, p.y, p.z);
	glMultMatrixf(camera->GetBillBoardMatrix());
	Begin(false, false);
	glPrint(0.0f, 0.0f, size, FONT_DESCENDER | FONT_CENTER | FONT_OUTLINE, str);
	End();
	glPopMatrix();
}


void CglFont::glPrint(float x, float y, float s, const int options, const std::string& text)
{
	// s := scale or absolute size?
	if (options & FONT_SCALE) {
		s *= fontSize;
	}

	float sizeX = s, sizeY = s;

	// render in normalized coords (0..1) instead of screencoords (0..~1024)
	if (options & FONT_NORM) {
		sizeX *= globalRendering->pixelX;
		sizeY *= globalRendering->pixelY;
	}

	// horizontal alignment (FONT_LEFT is default)
	if (options & FONT_CENTER) {
		x -= sizeX * 0.5f * GetTextWidth(text);
	} else if (options & FONT_RIGHT) {
		x -= sizeX * GetTextWidth(text);
	}


	// vertical alignment
	y += sizeY * GetDescender(); // move to baseline (note: descender is negative)
	if (options & FONT_BASELINE) {
		// nothing
	} else if (options & FONT_DESCENDER) {
		y -= sizeY * GetDescender();
	} else if (options & FONT_VCENTER) {
		float textDescender;
		y -= sizeY * 0.5f * GetTextHeight(text,&textDescender);
		y -= sizeY * 0.5f * textDescender;
	} else if (options & FONT_TOP) {
		y -= sizeY * GetTextHeight(text);
	} else if (options & FONT_ASCENDER) {
		y -= sizeY * GetDescender();
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

	// backup text & outline colors (also ::ColorResetIndicator will reset to those)
	baseTextColor = textColor;
	baseOutlineColor = outlineColor;

	// immediate mode?
	const bool immediate = !inBeginEnd;
	if (immediate) {
		Begin(!(options & (FONT_OUTLINE | FONT_SHADOW)));
	}


	// select correct decoration RenderString function
	if (options & FONT_OUTLINE) {
		RenderStringOutlined(x, y, sizeX, sizeY, text);
	} else if (options & FONT_SHADOW) {
		RenderStringShadow(x, y, sizeX, sizeY, text);
	} else {
		RenderString(x, y, sizeX, sizeY, text);
	}


	// immediate mode?
	if (immediate) {
		End();
	}

	// reset text & outline colors (if changed via in text colorcodes)
	SetColors(&baseTextColor,&baseOutlineColor);
}

void CglFont::glPrintTable(float x, float y, float s, const int options, const std::string& text)
{
	std::vector<std::string> coltext;
	coltext.push_back("");

	std::vector<SColor> colColor;
	SColor defaultcolor(0,0,0);
	defaultcolor[0] = ColorCodeIndicator;
	for (int i = 0; i < 3; ++i)
		defaultcolor[i+1] = (unsigned char)(textColor[i] * 255.0f);
	colColor.push_back(defaultcolor);
	SColor curcolor(defaultcolor);

	int col = 0;
	int row = 0;
	for (int pos = 0; pos < text.length(); pos++) {
		const unsigned char& c = text[pos];
		switch(c) {
			// inline colorcodes
			case ColorCodeIndicator:
				for (int i = 0; i < 4 && pos < text.length(); ++i, ++pos) {
					coltext[col] += text[pos];
					curcolor[i] = text[pos];
				}
				colColor[col] = curcolor;
				--pos;
				break;

			// column separator is `\t`==`horizontal tab`
			case '\t':
				++col;
				if (col >= coltext.size()) {
					coltext.push_back("");
					for(int i = 0; i < row; ++i)
						coltext[col] += 0x0a;
					colColor.push_back(defaultcolor);
				}
				if (colColor[col] != curcolor) {
					for(int i = 0; i < 4; ++i)
						coltext[col] += curcolor[i];
					colColor[col] = curcolor;
				}
				break;

			// newline
			case 0x0d: // CR+LF
				if (pos+1 < text.length() && text[pos + 1] == 0x0a)
					pos++;
			case 0x0a: // LF
				for (int i = 0; i < coltext.size(); ++i)
					coltext[i] += 0x0a;
				if (colColor[0] != curcolor) {
					for(int i = 0; i < 4; ++i)
						coltext[0] += curcolor[i];
					colColor[0] = curcolor;
				}
				col = 0;
				++row;
				break;

			// printable char
			default:
				coltext[col] += c;
		}
	}

	float totalWidth = 0.0f;
	float maxHeight = 0.0f;
	float minDescender = 0.0f;
	std::vector<float> colWidths(coltext.size(), 0.0f);
	for (int i = 0; i < coltext.size(); ++i) {
		float colwidth = GetTextWidth(coltext[i]);
		colWidths[i] = colwidth;
		totalWidth += colwidth;
		float textDescender;
		float textHeight = GetTextHeight(coltext[i], &textDescender);
		if (textHeight > maxHeight)
			maxHeight = textHeight;
		if (textDescender < minDescender)
			minDescender = textDescender;
	}

	// s := scale or absolute size?
	float ss = s;
	if (options & FONT_SCALE) {
		ss *= fontSize;
	}

	float sizeX = ss, sizeY = ss;

	// render in normalized coords (0..1) instead of screencoords (0..~1024)
	if (options & FONT_NORM) {
		sizeX *= globalRendering->pixelX;
		sizeY *= globalRendering->pixelY;
	}

	// horizontal alignment (FONT_LEFT is default)
	if (options & FONT_CENTER) {
		x -= sizeX * 0.5f * totalWidth;
	} else if (options & FONT_RIGHT) {
		x -= sizeX * totalWidth;
	}

	// vertical alignment
	if (options & FONT_BASELINE) {
		// nothing
	} else if (options & FONT_DESCENDER) {
		y -= sizeY * GetDescender();
	} else if (options & FONT_VCENTER) {
		y -= sizeY * 0.5f * maxHeight;
		y -= sizeY * 0.5f * minDescender;
	} else if (options & FONT_TOP) {
		y -= sizeY * maxHeight;
	} else if (options & FONT_ASCENDER) {
		y -= sizeY * GetDescender();
		y -= sizeY;
	} else if (options & FONT_BOTTOM) {
		y -= sizeY * minDescender;
	}

	for (int i = 0; i < coltext.size(); ++i) {
		glPrint(x, y, s, (options | FONT_BASELINE) & ~(FONT_RIGHT | FONT_CENTER), coltext[i]);
		x += sizeX * colWidths[i];
	}
}

void CglFont::glFormat(float x, float y, float s, const int options, const char* fmt, ...)
{
	char out[512];
	va_list ap;
	if (fmt == NULL) return;
	va_start(ap, fmt);
	VSNPRINTF(out, sizeof(out), fmt, ap);
	va_end(ap);
	glPrint(x, y, s, options, std::string(out));
}


