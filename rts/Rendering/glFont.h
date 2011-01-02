/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLFONT_H
#define _GLFONT_H

#include <string>
#include <list>
#include <limits.h> // for INT_MAX

#include "float4.h"

#undef GetCharWidth // winapi.h


static const int FONT_LEFT     = 1 << 0;
static const int FONT_RIGHT    = 1 << 1;
static const int FONT_CENTER   = 1 << 2;
static const int FONT_BASELINE = 1 << 3; //! align to face baseline
static const int FONT_VCENTER  = 1 << 4;
static const int FONT_TOP      = 1 << 5; //! align to text ascender
static const int FONT_BOTTOM   = 1 << 6; //! align to text descender
static const int FONT_ASCENDER = 1 << 7; //! align to face ascender
static const int FONT_DESCENDER= 1 << 8; //! align to face descender

static const int FONT_OUTLINE     = 1 << 9;
static const int FONT_SHADOW      = 1 << 10;
static const int FONT_NORM        = 1 << 11; //! render in 0..1 space instead of 0..vsx|vsy
static const int FONT_SCALE       = 1 << 12; //! given size argument will be treated as scaling and not absolute fontsize

static const int FONT_NEAREST     = 1 << 13; //! round x,y render pos to nearest integer, so there is no interpolation blur for small fontsizes

class CVertexArray;
class CglFont
{
public:
	static CglFont* LoadFont(const std::string& fontFile, int size, int outlinewidth = 2, float outlineweight = 5.0f, int start = 32, int end = 254);
	CglFont(const std::string& fontfile, int size, int outlinewidth, float  outlineweight);
	~CglFont();

	//! The calling of Begin() .. End() is optional,
	//! but can increase the performance of drawing multiple strings a lot (upto 10x)
	void Begin(const bool immediate = false, const bool resetColors = true);
	void End();

	void glWorldPrint(const float3 p, const float size, const std::string& str);
	/**
	 * @param s  absolute font size, or relative scale, if option FONT_SCALE is set
	 * @param options  FONT_SCALE | FONT_NORM |
	 *                 (FONT_LEFT | FONT_CENTER | FONT_RIGHT) |
	 *                 (FONT_BASELINE | FONT_DESCENDER | FONT_VCENTER |
	 *                  FONT_TOP | FONT_ASCENDER | FONT_BOTTOM) |
	 *                 FONT_NEAREST | FONT_OUTLINE | FONT_SHADOW
	 */
	void glPrint(float x, float y, float s, const int& options, const std::string& str);
	void glPrintTable(float x, float y, float s, const int& options, const std::string& str);
	void glFormat(float x, float y, float s, const int& options, const std::string& fmt, ...);
	void glFormat(float x, float y, float s, const int& options, const char* fmt, ...);

	void SetAutoOutlineColor(bool enable); //! auto select outline color for in-text-colorcodes
	void SetTextColor(const float4* color = NULL);
	void SetOutlineColor(const float4* color = NULL);
	void SetColors(const float4* textColor = NULL, const float4* outlineColor = NULL);
	void SetTextColor(const float& r, const float& g, const float& b, const float& a) { const float4 f = float4(r,g,b,a); SetTextColor(&f); };
	void SetOutlineColor(const float& r, const float& g, const float& b, const float& a) { const float4 f = float4(r,g,b,a); SetOutlineColor(&f); };

	//! Adds \n's (and '...' if it would be too high) until the text fits into maxWidth/maxHeight
	int WrapInPlace(std::string& text, float fontSize, const float maxWidth, const float maxHeight = 1e9) const;
	std::list<std::string> Wrap(const std::string& text, float fontSize, const float maxWidth, const float maxHeight = 1e9) const;

	float GetKerning(const unsigned int& left_char, const unsigned int& right_char) const;
	float GetCharacterWidth(const unsigned char c) const;
	float GetTextWidth(const std::string& text) const;
	float GetTextHeight(const std::string& text, float* descender = NULL, int* numLines = NULL) const;
	int   GetTextNumLines(const std::string& text) const;
	static std::string StripColorCodes(const std::string& text);

	inline float GetLineHeight()     const { return lineHeight; }
	inline float GetSize()           const { return fontSize; }
	inline float GetDescender()      const { return fontDescender; }
	inline int   GetOutlineWidth()   const { return outlineWidth; }
	inline float GetOutlineWeight()  const { return outlineWeight; }
	inline std::string GetFilePath() const { return fontPath; }
	inline std::string GetFamily()   const { return fontFamily; }
	inline std::string GetStyle()    const { return fontStyle; }
	int GetCharStart() const { return charstart; }
	int GetCharEnd()   const { return charend; }

	inline unsigned int GetTexture()   const { return fontTexture; }
	inline unsigned int GetTexWidth()  const { return texWidth; }
	inline unsigned int GetTexHeight() const { return texHeight; }

	static const char ColorCodeIndicator = '\xFF'; //FIXME use a non-printable char? (<32)
	static const char ColorResetIndicator = '\x08'; //! =: '\b'

public:
	typedef std::vector<float4> ColorMap;

	struct GlyphInfo
	{
		GlyphInfo()
		{
			u0  = v0  = u1  = v1  = 1.0f;
			x0  = y0  = x1  = y1  = 0.0f;
			us0 = vs0 = us1 = vs1 = 1.0f;
			advance = height = 1.0f;
			descender = 0.0f;

			for (int i = 0; i <= 255; i++) {
				kerning[i] = 1.0f;
			}
		};

		float u0, v0, u1, v1;
		float x0, y0, x1, y1;
		float us0, vs0, us1, vs1; //! shadow texcoords
		float advance, height, descender;
		float kerning[256];
	} glyphs[256];

private:
	static const float4* ChooseOutlineColor(const float4& textColor);

	void RenderString(float x, float y, const float& scaleX, const float& scaleY, const std::string& str);
	void RenderStringShadow(float x, float y, const float& scaleX, const float& scaleY, const std::string& str);
	void RenderStringOutlined(float x, float y, const float& scaleX, const float& scaleY, const std::string& str);

private:
	struct colorcode {
		colorcode() : resetColor(false),pos(0) {};
		bool resetColor;
		float4 color;
		unsigned int pos;
	};
	struct word {
		word() : width(0.0f), text(""), isSpace(false), isLineBreak(false), isColorCode(false), numSpaces(0), pos(0) {};

		float width;
		std::string text;
		bool isSpace;
		bool isLineBreak;
		bool isColorCode;
		unsigned int numSpaces;
		unsigned int pos; //! position in the original text (needed for remerging colorcodes after wrapping; in printable chars (linebreaks and space don't count))
	};
	struct line {
		line() : width(0.0f), cost(0.0f), forceLineBreak(false) {};

		std::list<word>::iterator start, end;
		float width;
		float cost;
		bool forceLineBreak;
	};

	word SplitWord(word& w, float wantedWidth, bool smart = true) const;

	void SplitTextInWords(const std::string& text, std::list<word>* words, std::list<colorcode>* colorcodes) const;
	void RemergeColorCodes(std::list<word>* words, std::list<colorcode>& colorcodes) const;

	void AddEllipsis(std::list<line>& lines, std::list<word>& words, float maxWidth) const;

	void WrapTextConsole(std::list<word>& words, float maxWidth, float maxHeight) const;
	void WrapTextKnuth(std::list<word>& words, float maxWidth, float maxHeight) const;

private:
	float fontSize;
	float fontDescender;
	float lineHeight;
	std::string fontPath;
	std::string fontFamily;
	std::string fontStyle;
	int outlineWidth;
	float outlineWeight;
	int chars;
	unsigned int charstart;
	unsigned int charend;

	unsigned int fontTexture;
	unsigned int texWidth,texHeight;

	ColorMap stripTextColors;
	ColorMap stripOutlineColors;

	bool inBeginEnd;
	CVertexArray* va;
	CVertexArray* va2;

	bool autoOutlineColor; //! auto select outline color for in-text-colorcodes

	bool setColor; //! used for backward compability (so you can call glPrint (w/o BeginEnd and no shadow/outline!) and set the color yourself via glColor)

	float4 textColor;
	float4 outlineColor;

	//! ::ColorResetIndicator will reset to those (they are the colors set when glPrint was called)
	float4 baseTextColor;
	float4 baseOutlineColor;
};

extern CglFont* font;
extern CglFont* smallFont;

#endif /* _GLFONT_H */
