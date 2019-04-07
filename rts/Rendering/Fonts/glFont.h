/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLFONT_H
#define _GLFONT_H

#include <string>
#include <deque>

#include "TextWrap.h"
#include "ustring.h"

#include "Rendering/GL/RenderDataBuffer.hpp"
#include "System/float4.h"
#include "System/Threading/SpringThreading.h"

#undef GetCharWidth // winapi.h

static constexpr int FONT_LEFT      = 1 << 0;
static constexpr int FONT_RIGHT     = 1 << 1;
static constexpr int FONT_CENTER    = 1 << 2;
static constexpr int FONT_BASELINE  = 1 << 3; // align to face baseline
static constexpr int FONT_VCENTER   = 1 << 4;
static constexpr int FONT_TOP       = 1 << 5; // align to text ascender
static constexpr int FONT_BOTTOM    = 1 << 6; // align to text descender
static constexpr int FONT_ASCENDER  = 1 << 7; // align to face ascender
static constexpr int FONT_DESCENDER = 1 << 8; // align to face descender

static constexpr int FONT_OUTLINE   = 1 << 9;
static constexpr int FONT_SHADOW    = 1 << 10;
static constexpr int FONT_NORM      = 1 << 11; // render in 0..1 space instead of 0..vsx|vsy
static constexpr int FONT_SCALE     = 1 << 12; // given size argument will be treated as scaling and not absolute fontsize

static constexpr int FONT_NEAREST   = 1 << 13; // round x,y render pos to nearest integer, so there is no interpolation blur for small fontsizes

static constexpr int FONT_BUFFERED  = 1 << 14; // make glFormat append to buffer outside a {Begin,End}GL4 pair


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
	static void ReallocAtlases(bool pre);
	static void SwapBuffers();

	CglFont(const std::string& fontFile, int size, int outlinewidth, float outlineweight);
	~CglFont();


	void BeginGL4() { BeginGL4(defShader); }
	void EndGL4() { EndGL4(defShader); }
	void BeginGL4(Shader::IProgramObject* shader);
	void EndGL4(Shader::IProgramObject* shader);

	void DrawBufferedGL4() { DrawBufferedGL4(defShader); }
	void DrawBufferedGL4(Shader::IProgramObject* shader);

	void SwapBuffersGL4();

	void ResetBufferGL4(bool outline) {
		const unsigned int bi = GetBufferIdx(outline);

		prvBufferPos[bi] = mapBufferPtr[bi];
		curBufferPos[bi] = mapBufferPtr[bi];
	}
	void ResetBuffersGL4() {
		ResetBufferGL4(false);
		ResetBufferGL4( true);
	}


	void glWorldBegin() { glWorldBegin(defShader); }
	void glWorldEnd() { glWorldEnd(defShader); }
	void glWorldBegin(Shader::IProgramObject* shader);
	void glWorldEnd(Shader::IProgramObject* shader);
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

	void SetAutoOutlineColor(bool enable); // auto-select outline color for in-text-colorcodes
	void SetTextColor(const float4* color = nullptr);
	void SetOutlineColor(const float4* color = nullptr);
	void SetColors(const float4* textColor = nullptr, const float4* outlineColor = nullptr);
	void SetTextColor(float r, float g, float b, float a) { const float4 f{r, g, b, a}; SetTextColor(&f); };
	void SetOutlineColor(float r, float g, float b, float a) { const float4 f{r, g, b, a}; SetOutlineColor(&f); };
	void SetTextDepth(float z) { textDepth.x = z; }
	void SetOutlineDepth(float z) { textDepth.y = z; }

	float GetCharacterWidth(const char32_t c);

	inline float GetTextWidth(const std::string& text) override;
	inline float GetTextHeight(const std::string& text, float* descender = nullptr, int* numLines = nullptr);

	static std::deque<std::string> SplitIntoLines(const std::u8string&);

	const std::string& GetFilePath() const { return fontPath; }

	static constexpr char8_t ColorCodeIndicator  = 0xFF;
	static constexpr char8_t ColorResetIndicator = 0x08; // =: '\\b'
	static bool threadSafety;

	// typedef void (*ColorCodeCallBack)(float4);
	typedef std::function<void(float4)> ColorCodeCallBack;

private:
	static const float4* ChooseOutlineColor(const float4& textColor);
	static const float4 GetTexScaleMatrix(float w, float h) { return {1.0f / w, 0.0f, 0.0f, 1.0f / h}; } // 2x2

	void RenderString(float x, float y, float scaleX, float scaleY, const std::string& str, const ColorCodeCallBack& cccb);
	void RenderStringShadow(float x, float y, float scaleX, float scaleY, const std::string& str, const ColorCodeCallBack& cccb);
	void RenderStringOutlined(float x, float y, float scaleX, float scaleY, const std::string& str, const ColorCodeCallBack& cccb);

private:
	float GetTextWidth_(const std::u8string& text);
	float GetTextHeight_(const std::u8string& text, float* descender = nullptr, int* numLines = nullptr);

private:
	unsigned int GetBufferIdx(bool outline) const { return (currBufferIndxGL4 * 2 + outline); }

	enum {
		PRIMARY_BUFFER = 0,
		OUTLINE_BUFFER = 1,
	};

private:
	std::string fontPath;
	spring::recursive_mutex bufferMutex;


	// used by {Begin,End}GL4; each double-buffered
	GL::RenderDataBuffer primaryBuffer[2];
	GL::RenderDataBuffer outlineBuffer[2];

	GL::RenderDataBufferTC primaryBufferTC[2];
	GL::RenderDataBufferTC outlineBufferTC[2];

	Shader::IProgramObject* defShader = nullptr;
	Shader::IProgramObject* curShader = nullptr;

	VA_TYPE_TC* mapBufferPtr[2 * 2] = {nullptr, nullptr, nullptr, nullptr}; // {primary,outline} start-pos
	VA_TYPE_TC* prvBufferPos[2 * 2] = {nullptr, nullptr, nullptr, nullptr}; // previous {primary,outline} write-pos
	VA_TYPE_TC* curBufferPos[2 * 2] = {nullptr, nullptr, nullptr, nullptr}; // current {primary,outline} write-pos


	unsigned int currBufferIndxGL4 = 0;
	unsigned int lastPrintFrameGL4 = 0;

	bool inBeginEndGL4 = false; // implies bufferMutex is locked
	bool autoOutlineColor = false; // auto-select outline color for in-text-colorcodes
	bool setColor = true; // used for backward compability (so you can call glPrint (w/o BeginEnd and no shadow/outline!) and set the color yourself via glColor)

	float4 textColor;
	float4 outlineColor;

	// colors set when glPrint was called; ColorResetIndicator will reset to these
	float4 baseTextColor;
	float4 baseOutlineColor;

	float2 textDepth;
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

#endif /* _GLFONT_H */

