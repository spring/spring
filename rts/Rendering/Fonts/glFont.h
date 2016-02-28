/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLFONT_H
#define _GLFONT_H

#include <string>
#include <list>

#include "TextWrap.h"
#include "ustring.h"

#include "Rendering/GL/VertexArray.h"
#include "System/float4.h"
#include "System/Threading/SpringMutex.h"

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



class CglFont : public CTextWrap
{
public:
	static CglFont* LoadFont(const std::string& fontFile, int size, int outlinewidth = 2, float outlineweight = 5.0f);
	CglFont(const std::string& fontfile, int size, int outlinewidth, float  outlineweight);
	virtual ~CglFont();

	//! The calling of Begin() .. End() is optional,
	//! but can increase the performance of drawing multiple strings a lot (upto 10x)
	void Begin(const bool immediate = false, const bool resetColors = true);
	void End();

	void glWorldPrint(const float3& p, const float size, const std::string& str);
	/**
	 * @param s  absolute font size, or relative scale, if option FONT_SCALE is set
	 * @param options  FONT_SCALE | FONT_NORM |
	 *                 (FONT_LEFT | FONT_CENTER | FONT_RIGHT) |
	 *                 (FONT_BASELINE | FONT_DESCENDER | FONT_VCENTER |
	 *                  FONT_TOP | FONT_ASCENDER | FONT_BOTTOM) |
	 *                 FONT_NEAREST | FONT_OUTLINE | FONT_SHADOW
	 */
	void glPrint(float x, float y, float s, const int options, const std::string& str);
	void glPrintTable(float x, float y, float s, const int options, const std::string& str);
	void glFormat(float x, float y, float s, const int options, const char* fmt, ...);

	void SetAutoOutlineColor(bool enable); //! auto select outline color for in-text-colorcodes
	void SetTextColor(const float4* color = NULL);
	void SetOutlineColor(const float4* color = NULL);
	void SetColors(const float4* textColor = NULL, const float4* outlineColor = NULL);
	void SetTextColor(const float& r, const float& g, const float& b, const float& a) { const float4 f = float4(r,g,b,a); SetTextColor(&f); };
	void SetOutlineColor(const float& r, const float& g, const float& b, const float& a) { const float4 f = float4(r,g,b,a); SetOutlineColor(&f); };

	float GetCharacterWidth(const char32_t c);
#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 6)
	#define _final
#else
	#define _final final
#endif
	inline float GetTextWidth(const std::string& text) _final;
#undef _final
	inline float GetTextHeight(const std::string& text, float* descender = NULL, int* numLines = NULL);
	inline static int GetTextNumLines(const std::string& text);
	inline static std::string StripColorCodes(const std::string& text);
	static std::list<std::string> SplitIntoLines(const std::u8string&);

	const std::string& GetFilePath() const { return fontPath; }

	static const char8_t ColorCodeIndicator  = 0xFF;
	static const char8_t ColorResetIndicator = 0x08; //! =: '\\b'
	static bool threadSafety;
private:
	static const float4* ChooseOutlineColor(const float4& textColor);

	void RenderString(float x, float y, const float& scaleX, const float& scaleY, const std::string& str);
	void RenderStringShadow(float x, float y, const float& scaleX, const float& scaleY, const std::string& str);
	void RenderStringOutlined(float x, float y, const float& scaleX, const float& scaleY, const std::string& str);

private:
	float GetTextWidth_(const std::u8string& text);
	float GetTextHeight_(const std::u8string& text, float* descender = NULL, int* numLines = NULL);
	static int GetTextNumLines_(const std::u8string& text);
	static std::string StripColorCodes_(const std::u8string& text);

public:
	typedef std::vector<float4> ColorMap;

private:
	std::string fontPath;

	ColorMap stripTextColors;
	ColorMap stripOutlineColors;

	CVertexArray va;
	CVertexArray va2;

	spring::recursive_mutex vaMutex;

	bool inBeginEnd;
	bool autoOutlineColor; //! auto select outline color for in-text-colorcodes
	bool setColor; //! used for backward compability (so you can call glPrint (w/o BeginEnd and no shadow/outline!) and set the color yourself via glColor)

	float4 textColor;
	float4 outlineColor;

	//! \::ColorResetIndicator will reset to those (they are the colors set when glPrint was called)
	float4 baseTextColor;
	float4 baseOutlineColor;
};


extern CglFont* font;
extern CglFont* smallFont;


// wrappers
float CglFont::GetTextWidth(const std::string& text)
{
	return GetTextWidth_(toustring(text));
}
float CglFont::GetTextHeight(const std::string& text, float* descender, int* numLines)
{
	return GetTextHeight_(toustring(text), descender, numLines);
}
int   CglFont::GetTextNumLines(const std::string& text)
{
	return GetTextNumLines_(toustring(text));
}
std::string CglFont::StripColorCodes(const std::string& text)
{
	return StripColorCodes_(toustring(text));
}

#endif /* _GLFONT_H */
