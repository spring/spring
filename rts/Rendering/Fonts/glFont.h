/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <string>
#include <deque>

#include "TextWrap.h"
#include "ustring.h"

#include "Rendering/GL/RenderBuffers.h"
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

static constexpr int FONT_BUFFERED  = 1 << 14; // make glFormat append to buffer outside a {Begin,End} pair


namespace Shader {
	struct IProgramObject;
};

class CglFont : public CTextWrap
{
public:
	using ColorCodeCallBack = std::function<void(float4)>;

	static bool LoadConfigFonts();
	static bool LoadCustomFonts(const std::string& smallFontFile, const std::string& largeFontFile);
	static CglFont* LoadFont(const std::string& fontFile, bool small);
	static CglFont* LoadFont(const std::string& fontFile, int size, int outlinewidth = 2, float outlineweight = 5.0f);
	static void ReallocAtlases(bool pre);
	static void SwapRenderBuffers();

	CglFont(const std::string& fontFile, int size, int outlinewidth, float outlineweight);
	~CglFont();

	void Begin(Shader::IProgramObject* shader);
	void Begin() { Begin(defShader.get()); };
	void End();

	void DrawBufferedGL4() { DrawBuffered(); }
	void DrawBufferedGL4(Shader::IProgramObject* shader) { DrawBuffered(shader); }

	void DrawBuffered() { DrawBuffered(defShader.get()); }
	void DrawBuffered(Shader::IProgramObject* shader);

	void SwapBuffers();

	void glWorldPrint(const float3& p, const float size, const std::string& str);

	void SetViewMatrix(const CMatrix44f& mat) { viewMatrix = mat; }
	void SetProjMatrix(const CMatrix44f& mat) { projMatrix = mat; }

	static CMatrix44f DefViewMatrix();
	static CMatrix44f DefProjMatrix();

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

	const TypedRenderBuffer<VA_TYPE_TC>& GetPrimaryBuffer() const { return primaryBufferTC; };
	const TypedRenderBuffer<VA_TYPE_TC>& GetOutlineBuffer() const { return outlineBufferTC; };

	static constexpr char8_t ColorCodeIndicator  = 0xFF;
	static constexpr char8_t ColorResetIndicator = 0x08; // =: '\\b'
	static bool threadSafety;
private:
	static void CreateDefaultShader();

	static const float4* ChooseOutlineColor(const float4& textColor);

	template<int shiftXC, int shiftYC, bool outline>
	void RenderStringImpl(float x, float y, float scaleX, float scaleY, const std::string& str, const ColorCodeCallBack& cccb);

	void RenderString(float x, float y, float scaleX, float scaleY, const std::string& str, const ColorCodeCallBack& cccb) {
		RenderStringImpl<0 , 0 , false>(x, y, scaleX, scaleY, str, cccb);
	}
	void RenderStringOutlined(float x, float y, float scaleX, float scaleY, const std::string& str, const ColorCodeCallBack& cccb) {
		RenderStringImpl<0 , 0 , true >(x, y, scaleX, scaleY, str, cccb);
	}
	void RenderStringShadow(float x, float y, float scaleX, float scaleY, const std::string& str, const ColorCodeCallBack& cccb) {
		RenderStringImpl<10, 10, true >(x, y, scaleX, scaleY, str, cccb);
	}
private:
	float GetTextWidth_(const std::u8string& text);
	float GetTextHeight_(const std::u8string& text, float* descender = nullptr, int* numLines = nullptr);
private:
	inline static spring::unsynced_set<CglFont*> loadedFonts = {};
public:
	static auto GetLoadedFonts() -> const decltype(loadedFonts)& {
		return loadedFonts;
	}
private:
	inline static std::unique_ptr<Shader::IProgramObject> defShader = nullptr;

	std::string fontPath;
	spring::recursive_mutex bufferMutex;

	TypedRenderBuffer<VA_TYPE_TC> primaryBufferTC;
	TypedRenderBuffer<VA_TYPE_TC> outlineBufferTC;

	Shader::IProgramObject* curShader = nullptr;

	bool inBeginEndBlock = false; // implies bufferMutex is locked
	bool autoOutlineColor = false; // auto-select outline color for in-text-colorcodes

	float4 textColor;
	float4 outlineColor;

	// colors set when glPrint was called; ColorResetIndicator will reset to these
	float4 baseTextColor;
	float4 baseOutlineColor;

	float2 textDepth;

	CMatrix44f viewMatrix;
	CMatrix44f projMatrix;
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
