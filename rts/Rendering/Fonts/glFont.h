/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLFONT_H
#define _GLFONT_H

#include <string>
#include <deque>

#include "TextWrap.h"
#include "ustring.h"

#include "Rendering/GL/VertexArray.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "System/float4.h"
#include "System/Threading/SpringThreading.h"

#undef GetCharWidth // winapi.h

static constexpr int FONT_LEFT     = 1 << 0;
static constexpr int FONT_RIGHT    = 1 << 1;
static constexpr int FONT_CENTER   = 1 << 2;
static constexpr int FONT_BASELINE = 1 << 3; // align to face baseline
static constexpr int FONT_VCENTER  = 1 << 4;
static constexpr int FONT_TOP      = 1 << 5; // align to text ascender
static constexpr int FONT_BOTTOM   = 1 << 6; // align to text descender
static constexpr int FONT_ASCENDER = 1 << 7; // align to face ascender
static constexpr int FONT_DESCENDER= 1 << 8; // align to face descender

static constexpr int FONT_OUTLINE     = 1 << 9;
static constexpr int FONT_SHADOW      = 1 << 10;
static constexpr int FONT_NORM        = 1 << 11; // render in 0..1 space instead of 0..vsx|vsy
static constexpr int FONT_SCALE       = 1 << 12; // given size argument will be treated as scaling and not absolute fontsize

static constexpr int FONT_NEAREST     = 1 << 13; // round x,y render pos to nearest integer, so there is no interpolation blur for small fontsizes

static constexpr int FONT_BUFFERED    = 1 << 14; // make glFormat append to buffer outside a {Begin,End}GL4 pair
static constexpr int FONT_FINISHED    = 1 << 15; // make DrawBufferedGL4 force a finish


namespace Shader {
	struct IProgramObject;
};

class CglFont : public CTextWrap
{
public:
	static bool LoadConfigFonts();
	static bool LoadCustomFonts(const std::string& smallFontFile, const std::string& largeFontFile);
	static CglFont* LoadFont(const std::string& fontFile, bool small);
	static CglFont* LoadFont(const std::string& fontFile, int size, int outlinewidth = 2, float outlineweight = 5.0f);

	CglFont(const std::string& fontFile, int size, int outlinewidth, float outlineweight);
	~CglFont();

	// calling Begin() .. End() is optional, but can increase the
	// performance of drawing multiple strings a lot (upto 10x)
	void Begin(const bool immediate = false, const bool resetColors = true);
	void End();


	void BeginGL4() { BeginGL4(&primaryBuffer.GetShader()); }
	void EndGL4() { EndGL4(&primaryBuffer.GetShader()); }
	void BeginGL4(Shader::IProgramObject* shader);
	void EndGL4(Shader::IProgramObject* shader);

	void DrawBufferedGL4() { DrawBufferedGL4(&primaryBuffer.GetShader()); }
	void DrawBufferedGL4(Shader::IProgramObject* shader);


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
	void SetTextColor(const float4* color = nullptr);
	void SetOutlineColor(const float4* color = nullptr);
	void SetColors(const float4* textColor = nullptr, const float4* outlineColor = nullptr);
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
	inline float GetTextHeight(const std::string& text, float* descender = nullptr, int* numLines = nullptr);
	inline static int GetTextNumLines(const std::string& text);
	inline static std::string StripColorCodes(const std::string& text);
	static std::deque<std::string> SplitIntoLines(const std::u8string&);

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
	float GetTextHeight_(const std::u8string& text, float* descender = nullptr, int* numLines = nullptr);
	static int GetTextNumLines_(const std::u8string& text);
	static std::string StripColorCodes_(const std::u8string& text);

public:
	typedef std::vector<float4> ColorMap;
	typedef void (*ColorCallback)(const float*);

	void SetColorCallback(ColorCallback cb) { colorCallback = cb; }
	// called from drawArrays through a callback
	void SetNextColor();

private:
	std::string fontPath;


	ColorMap stripTextColors;
	ColorMap stripOutlineColors;

	ColorMap::iterator colorIterator;
	ColorCallback colorCallback;

	// used by {Begin,End}
	CVertexArray va;
	CVertexArray va2;


	// used by {Begin,End}GL4
	GL::RenderDataBuffer primaryBuffer;
	GL::RenderDataBuffer outlineBuffer;

	VA_TYPE_2dTC* primaryBufferPtr = nullptr;
	VA_TYPE_2dTC* primaryBufferPos = nullptr;
	VA_TYPE_2dTC* outlineBufferPtr = nullptr;
	VA_TYPE_2dTC* outlineBufferPos = nullptr;


	spring::recursive_mutex bufferMutex;

	bool inBeginEnd = false; // implies bufferMutex is locked
	bool inBeginEndGL4 = false; // implies bufferMutex is locked
	bool bufferedWriteGL4 = false;
	bool pendingFinishGL4 = false;

	bool autoOutlineColor = false; //! auto select outline color for in-text-colorcodes
	bool setColor = true; //! used for backward compability (so you can call glPrint (w/o BeginEnd and no shadow/outline!) and set the color yourself via glColor)

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
