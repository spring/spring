/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "glFont.h"
#include "FontLogSection.h"
#include <stdarg.h>
#include <stdexcept>

#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VertexArray.h"
#include "System/Color.h"
#include "System/Exceptions.h"
#include "System/myMath.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

#undef GetCharWidth // winapi.h

// should be enough to hold all data for a given frame
#define NUM_BUFFER_ELEMS (1 << 17)
#define NUM_BUFFER_QUADS (NUM_BUFFER_ELEMS / 4)
#define ELEM_BUFFER_SIZE (sizeof(VA_TYPE_TC))
#define QUAD_BUFFER_SIZE (4 * ELEM_BUFFER_SIZE)

CONFIG(std::string,      FontFile).defaultValue("fonts/FreeSansBold.otf").description("Sets the font of Spring engine text.");
CONFIG(std::string, SmallFontFile).defaultValue("fonts/FreeSansBold.otf").description("Sets the font of Spring engine small text.");

CONFIG(int,      FontSize).defaultValue(23).description("Sets the font size (in pixels) of the MainMenu and more.");
CONFIG(int, SmallFontSize).defaultValue(14).description("Sets the font size (in pixels) of the engine GUIs and more.");
CONFIG(int,      FontOutlineWidth).defaultValue(3).description("Sets the width of the black outline around Spring engine text, such as the title screen version number, clock, and basic UI. Does not affect LuaUI elements.");
CONFIG(int, SmallFontOutlineWidth).defaultValue(2).description("see FontOutlineWidth");
CONFIG(float,      FontOutlineWeight).defaultValue(25.0f).description("Sets the opacity of Spring engine text, such as the title screen version number, clock, and basic UI. Does not affect LuaUI elements.");
CONFIG(float, SmallFontOutlineWeight).defaultValue(10.0f).description("see FontOutlineWeight");


bool CglFont::threadSafety = false;

/*******************************************************************************/
/*******************************************************************************/

CglFont* font = nullptr;
CglFont* smallFont = nullptr;

static const float4        white(1.00f, 1.00f, 1.00f, 0.95f);
static const float4  darkOutline(0.05f, 0.05f, 0.05f, 0.95f);
static const float4 lightOutline(0.95f, 0.95f, 0.95f, 0.8f);

static const float darkLuminosity = 0.05 +
	0.2126f * std::pow(darkOutline[0], 2.2) +
	0.7152f * std::pow(darkOutline[1], 2.2) +
	0.0722f * std::pow(darkOutline[2], 2.2);

/*******************************************************************************/
/*******************************************************************************/

bool CglFont::LoadConfigFonts()
{
	spring::SafeDelete(font);
	spring::SafeDelete(smallFont);

	font = CglFont::LoadFont("", false);
	smallFont = CglFont::LoadFont("", true);

	if (font == nullptr)
		throw content_error("Failed to load FontFile \"" + configHandler->GetString("FontFile") + "\", did you forget to run make install?");

	if (smallFont == nullptr)
		throw content_error("Failed to load SmallFontFile \"" + configHandler->GetString("SmallFontFile") + "\", did you forget to run make install?");

	return true;
}

bool CglFont::LoadCustomFonts(const std::string& smallFontFile, const std::string& largeFontFile)
{
	CglFont* newLargeFont = CglFont::LoadFont(largeFontFile, false);
	CglFont* newSmallFont = CglFont::LoadFont(smallFontFile, true);

	if (newLargeFont != nullptr && newSmallFont != nullptr) {
		spring::SafeDelete(font);
		spring::SafeDelete(smallFont);
		font = newLargeFont;
		smallFont = newSmallFont;

		LOG("[%s] loaded fonts \"%s\" and \"%s\"", __func__, smallFontFile.c_str(), largeFontFile.c_str());
		configHandler->SetString(     "FontFile", largeFontFile);
		configHandler->SetString("SmallFontFile", smallFontFile);
	}

	return true;
}

CglFont* CglFont::LoadFont(const std::string& fontFileOverride, bool smallFont)
{
	const std::string fontFiles[] = {configHandler->GetString("FontFile"), configHandler->GetString("SmallFontFile")};
	const std::string& fontFile = (fontFileOverride.empty())? fontFiles[smallFont]: fontFileOverride;

	const   int fontSizes[] = {configHandler->GetInt("FontSize"), configHandler->GetInt("SmallFontSize")};
	const   int fontWidths[] = {configHandler->GetInt("FontOutlineWidth"), configHandler->GetInt("SmallFontOutlineWidth")};
	const float fontWeights[] = {configHandler->GetFloat("FontOutlineWeight"), configHandler->GetFloat("SmallFontOutlineWeight")};

	return (CglFont::LoadFont(fontFile, fontSizes[smallFont], fontWidths[smallFont], fontWeights[smallFont]));
}


CglFont* CglFont::LoadFont(const std::string& fontFile, int size, int outlinewidth, float outlineweight)
{
	try {
		return (new CglFont(fontFile, size, outlinewidth, outlineweight));
	} catch (const content_error& ex) {
		LOG_L(L_ERROR, "Failed creating font: %s", ex.what());
		return nullptr;
	}
}


CglFont::CglFont(const std::string& fontFile, int size, int _outlineWidth, float _outlineWeight)
: CTextWrap(fontFile, size, _outlineWidth, _outlineWeight)
, fontPath(fontFile)
, colorCallback(nullptr)
{
	textColor    = white;
	outlineColor = darkOutline;

	for (unsigned int i = 0; i < 2; i++) {
		char vsBuf[65536];
		char fsBuf[65536];
		// color attribs are not normalized
		const char* fsVars = "const float N = 1.0 / 255.0;\n";
		const char* fsCode = "\tf_color_rgba = (v_color_rgba * N) * vec4(1.0, 1.0, 1.0, texture(u_tex0, v_texcoor_st).a);\n";

		primaryBuffer[i].Init(true);
		outlineBuffer[i].Init(true);
		primaryBuffer[i].UploadTC((NUM_BUFFER_ELEMS * QUAD_BUFFER_SIZE) / sizeof(VA_TYPE_TC), 0,  nullptr, nullptr); // no indices
		outlineBuffer[i].UploadTC((NUM_BUFFER_ELEMS * QUAD_BUFFER_SIZE) / sizeof(VA_TYPE_TC), 0,  nullptr, nullptr);
		primaryBuffer[i].FormatShaderTC(vsBuf, vsBuf + sizeof(vsBuf), "", "", "", "VS");
		primaryBuffer[i].FormatShaderTC(fsBuf, fsBuf + sizeof(fsBuf), "", fsVars, fsCode, "FS");

		Shader::GLSLShaderObject shaderObjs[2] = {{GL_VERTEX_SHADER, &vsBuf[0]}, {GL_FRAGMENT_SHADER, &fsBuf[0]}};
		Shader::IProgramObject* shaderProg = primaryBuffer[i].CreateShader((sizeof(shaderObjs) / sizeof(shaderObjs[0])), 0, &shaderObjs[0], nullptr);

		shaderProg->Enable();
		shaderProg->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, CMatrix44f::Identity());
		shaderProg->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, CMatrix44f::OrthoProj(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f));
		shaderProg->SetUniform("u_tex0", 0);
		shaderProg->Disable();

		mapBufferPtr[i * 2 + PRIMARY_BUFFER] = primaryBuffer[i].MapElems<VA_TYPE_TC>(true, true);
		prvBufferPos[i * 2 + PRIMARY_BUFFER] = mapBufferPtr[i * 2 + PRIMARY_BUFFER];
		curBufferPos[i * 2 + PRIMARY_BUFFER] = mapBufferPtr[i * 2 + PRIMARY_BUFFER];

		mapBufferPtr[i * 2 + OUTLINE_BUFFER] = outlineBuffer[i].MapElems<VA_TYPE_TC>(true, true);
		prvBufferPos[i * 2 + OUTLINE_BUFFER] = mapBufferPtr[i * 2 + OUTLINE_BUFFER];
		curBufferPos[i * 2 + OUTLINE_BUFFER] = mapBufferPtr[i * 2 + OUTLINE_BUFFER];

		assert(mapBufferPtr[i * 2 + PRIMARY_BUFFER] != nullptr);
		assert(mapBufferPtr[i * 2 + OUTLINE_BUFFER] != nullptr);
	}
}

CglFont::~CglFont()
{
	for (unsigned int i = 0; i < 2; i++) {
		primaryBuffer[i].Kill();
		outlineBuffer[i].Kill();
	}
}


/*******************************************************************************/
/*******************************************************************************/

template <typename T>
static inline int SkipColorCodes(const std::u8string& text, T pos)
{
	while (text[pos] == CglFont::ColorCodeIndicator) {
		if ((pos += 4) >= text.size())
			return -1;
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
	CglFont* f = reinterpret_cast<CglFont*>(data);
	f->SetNextColor();
}

static inline void CallColorCallback(CglFont::ColorCallback colorCallback, float* v)
{
	if (colorCallback == nullptr) {
		glColor4fv(v);
	} else {
		colorCallback(v);
	}
}

void CglFont::SetNextColor()
{
	CallColorCallback(colorCallback, *colorIterator++);
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
	if (text.empty())
		return 0.0f;

	float w = 0.0f;
	float maxw = 0.0f;

	const GlyphInfo* prv_g = nullptr;
	const GlyphInfo* cur_g = nullptr;

	int pos = 0;
	while (pos < text.length()) {
		const char32_t u = utf8::GetNextChar(text, pos);

		switch (u) {
			// inlined colorcode
			case ColorCodeIndicator:
				pos = SkipColorCodes(text, pos - 1);
				if (pos < 0)
					pos = text.length();

				break;

			// reset color
			case ColorResetIndicator:
				break;

			// newline
			case 0x0d: // CR+LF
				pos += (pos < text.length() && text[pos] == 0x0a);
			case 0x0a: // LF
				if (prv_g != nullptr)
					w += prv_g->advance;
				if (w > maxw)
					maxw = w;
				w = 0.0f;
				prv_g = nullptr;
				break;

			// printable char
			default: {
				cur_g = &GetGlyph(u);
				if (prv_g != nullptr)
					w += GetKerning(*prv_g, *cur_g);
				prv_g = cur_g;
			}
		}
	}

	if (prv_g != nullptr)
		w += prv_g->advance;
	if (w > maxw)
		maxw = w;

	return maxw;
}


float CglFont::GetTextHeight_(const std::u8string& text, float* descender, int* numLines)
{
	if (text.empty()) {
		if (descender != nullptr) *descender = 0.0f;
		if (numLines != nullptr) *numLines = 0;
		return 0.0f;
	}

	float h = 0.0f, d = GetLineHeight() + GetDescender();
	unsigned int multiLine = 1;

	int pos = 0;
	while (pos < text.length()) {
		const char32_t u = utf8::GetNextChar(text, pos);

		switch (u) {
			// inlined colorcode
			case ColorCodeIndicator:
				pos = SkipColorCodes(text, pos - 1);
				if (pos < 0)
					pos = text.length();

				break;

			// reset color
			case ColorResetIndicator:
				break;

			// newline
			case 0x0d: // CR+LF
				pos += (pos < text.length() && text[pos] == 0x0a);
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

	if (multiLine > 1) d -= ((multiLine - 1) * GetLineHeight());
	if (descender != nullptr) *descender = d;
	if (numLines != nullptr) *numLines = multiLine;

	return h;
}


int CglFont::GetTextNumLines_(const std::u8string& text)
{
	if (text.empty())
		return 0;

	int lines = 1;

	for (int pos = 0; pos < text.length(); pos++) {
		const char8_t c = text[pos];

		switch (c) {
			// inlined colorcode
			case ColorCodeIndicator:
				pos = SkipColorCodes(text, pos);
				if (pos < 0) {
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
				pos += (pos + 1 < text.length() && text[pos + 1] == 0x0a);
			case 0x0a:
				lines++;
				break;

			//default:
		}
	}

	return lines;
}


std::deque<std::string> CglFont::SplitIntoLines(const std::u8string& text)
{
	std::deque<std::string> lines;
	std::deque<std::string> colorCodeStack;

	if (text.empty())
		return lines;

	lines.push_back("");

	for (int pos = 0; pos < text.length(); pos++) {
		const char8_t& c = text[pos];
		switch (c) {
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
				pos += (pos + 1 < text.length() && text[pos + 1] == 0x0a);
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
		bufferMutex.lock();
	autoOutlineColor = enable;
	if (threadSafety)
		bufferMutex.unlock();
}


void CglFont::SetTextColor(const float4* color)
{
	if (color == nullptr)
		color = &white;

	if (threadSafety)
		bufferMutex.lock();


	// if between Begin and End, replace current primary color
	if (inBeginEnd && !(*color == textColor)) {
		if (va.drawIndex() == 0 && !stripTextColors.empty()) {
			stripTextColors.back() = *color;
		} else {
			stripTextColors.push_back(*color);
			va.EndStrip();
		}
	}


	textColor = *color;

	if (threadSafety)
		bufferMutex.unlock();
}

void CglFont::SetOutlineColor(const float4* color)
{
	if (color == nullptr)
		color = ChooseOutlineColor(textColor);

	if (threadSafety)
		bufferMutex.lock();


	// if between Begin and End, replace current outline color
	if (inBeginEnd && !(*color == outlineColor)) {
		if (va2.drawIndex() == 0 && !stripOutlineColors.empty()) {
			stripOutlineColors.back() = *color;
		} else {
			stripOutlineColors.push_back(*color);
			va2.EndStrip();
		}
	}


	outlineColor = *color;

	if (threadSafety)
		bufferMutex.unlock();
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

	const float maxLum = std::max(luminosity, darkLuminosity);
	const float minLum = std::min(luminosity, darkLuminosity);

	if ((maxLum / minLum) > 5.0f)
		return &darkOutline;

	return &lightOutline;
}




/*******************************************************************************/
/*******************************************************************************/

void CglFont::BeginGL4(Shader::IProgramObject* shader) {
	if (threadSafety)
		bufferMutex.lock();

	if (inBeginEndGL4 || inBeginEnd) {
		bufferMutex.unlock();
		return;
	}

	inBeginEndGL4 = true;

	{
		if (shader != nullptr) {
			curShader = shader;
		} else {
			curShader = GetPrimaryShader();
		}

		if (curShader == GetPrimaryShader())
			curShader->Enable();

		// NOTE: FFP
		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

void CglFont::EndGL4(Shader::IProgramObject* shader) {
	if (!inBeginEndGL4 || inBeginEnd)
		return;

	{
		UpdateTexture();

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, GetTexture());

		const unsigned int pbi = GetBufferIdx(PRIMARY_BUFFER);
		const unsigned int obi = GetBufferIdx(OUTLINE_BUFFER);

		if (curBufferPos[obi] > prvBufferPos[obi])
			outlineBuffer[currBufferIdxGL4].Submit(GL_QUADS, (prvBufferPos[obi] - mapBufferPtr[obi]), (curBufferPos[obi] - prvBufferPos[obi]));
		if (curBufferPos[pbi] > prvBufferPos[pbi])
			primaryBuffer[currBufferIdxGL4].Submit(GL_QUADS, (prvBufferPos[pbi] - mapBufferPtr[pbi]), (curBufferPos[pbi] - prvBufferPos[pbi]));

		// skip past the submitted data chunk; buffer should never fill up
		// within a single frame so handling wrap-around here is pointless
		prvBufferPos[pbi] = curBufferPos[pbi];
		prvBufferPos[obi] = curBufferPos[obi];

		// NOTE: each {Begin,End}GL4 pair currently requires an (implicit or explicit) glFinish after Submit
		if (pendingFinishGL4)
			glFinish();

		if (curShader == GetPrimaryShader())
			curShader->Disable();

		glBindTexture(GL_TEXTURE_2D, 0);
		glPopAttrib();
	}

	curShader = nullptr;

	inBeginEndGL4 = false;
	pendingFinishGL4 = false;

	if (threadSafety)
		bufferMutex.unlock();
}


void CglFont::DrawBufferedGL4(Shader::IProgramObject* shader)
{
	if (threadSafety)
		bufferMutex.lock();

	if (shader != nullptr) {
		curShader = shader;
	} else {
		curShader = GetPrimaryShader();
	}

	{
		UpdateTexture();

		// assume external shaders are already bound
		if (curShader == GetPrimaryShader())
			curShader->Enable();

		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, GetTexture());

		const unsigned int pbi = GetBufferIdx(PRIMARY_BUFFER);
		const unsigned int obi = GetBufferIdx(OUTLINE_BUFFER);

		if (curBufferPos[obi] > prvBufferPos[obi])
			outlineBuffer[currBufferIdxGL4].Submit(GL_QUADS, (prvBufferPos[obi] - mapBufferPtr[obi]), (curBufferPos[obi] - prvBufferPos[obi]));
		if (curBufferPos[pbi] > prvBufferPos[pbi])
			primaryBuffer[currBufferIdxGL4].Submit(GL_QUADS, (prvBufferPos[pbi] - mapBufferPtr[pbi]), (curBufferPos[pbi] - prvBufferPos[pbi]));

		prvBufferPos[pbi] = curBufferPos[pbi];
		prvBufferPos[obi] = curBufferPos[obi];

		if (pendingFinishGL4)
			glFinish();

		if (curShader == GetPrimaryShader())
			curShader->Disable();

		glBindTexture(GL_TEXTURE_2D, 0);
		glPopAttrib();
	}

	curShader = nullptr;

	pendingFinishGL4 = false;

	if (threadSafety)
		bufferMutex.unlock();
}




void CglFont::Begin(const bool immediate, const bool resetColors)
{
	if (threadSafety)
		bufferMutex.lock();

	if (inBeginEnd || inBeginEndGL4) {
		LOG_L(L_ERROR, "[glFont::%s] called Begin() multiple times (inBeginEnd{GL4}={%d,%d}", __func__, inBeginEnd, inBeginEndGL4);
		if (threadSafety)
			bufferMutex.unlock();
		return;
	}


	autoOutlineColor = true;
	setColor = !immediate;

	if (resetColors)
		SetColors(); // reset colors

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
	if (!inBeginEnd || inBeginEndGL4) {
		LOG_L(L_ERROR, "[glFont::%s] called End() without Begin() (inBeginEnd{GL4}={%d,%d}", __func__, inBeginEnd, inBeginEndGL4);
		return;
	}

	inBeginEnd = false;

	if (va.drawIndex() == 0) {
		glPopAttrib();
		if (threadSafety)
			bufferMutex.unlock();
		return;
	}

	GLboolean inListCompile;
	glGetBooleanv(GL_LIST_INDEX, &inListCompile);
	if (!inListCompile)
		UpdateTexture();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, GetTexture());


	// Because texture size can change, texture coordinates are absolute in texels.
	// We could use also just use GL_TEXTURE_RECTANGLE but then all shaders would
	// need to detect so and use different funcs & types if supported -> more work
	//
	if (va2.drawIndex() > 0) {
		if (stripOutlineColors.size() > 1) {
			colorIterator = stripOutlineColors.begin();
			va2.DrawArray2dT(GL_QUADS, TextStripCallback, this);
		} else {
			CallColorCallback(colorCallback, outlineColor);
			va2.DrawArray2dT(GL_QUADS);
		}
	}

	{
		if (stripTextColors.size() > 1) {
			colorIterator = stripTextColors.begin();
			va.DrawArray2dT(GL_QUADS, TextStripCallback, this);//FIXME calls a 0 length strip!
		} else {
			// single color over entire text
			if (setColor)
				CallColorCallback(colorCallback, textColor);
			va.DrawArray2dT(GL_QUADS);
		}
	}


	glPopAttrib();

	if (threadSafety)
		bufferMutex.unlock();
}




/*******************************************************************************/
/*******************************************************************************/

void CglFont::RenderString(float x, float y, float scaleX, float scaleY, const std::string& str)
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

	const std::u8string& ustr = toustring(str);

	const float startx = x;
	const float lineHeight_ = scaleY * GetLineHeight();
	const size_t length = str.length();

	if (!inBeginEndGL4)
		va.EnlargeArrays(length * 4, 0, VA_SIZE_2DT);

	const unsigned int pbi = GetBufferIdx(PRIMARY_BUFFER);
	// const unsigned int obi = GetBufferIdx(OUTLINE_BUFFER);

	int i = 0;
	int skippedLines = 0;
	bool colorChanged = false;

	// NOTE:
	//   we need to keep track of the current and previous *characters*
	//   rather than glyph *pointers*, because the previous-pointer can
	//   become dangling as a result of GetGlyph calls
	char32_t cc = 0;
	char32_t pc = 0;

	float4 newColor = textColor;

	const float texScaleX = 1.0f / texWidth;
	const float texScaleY = 1.0f / texHeight;

	do {
		// check for end-of-string
		if (SkipColorCodesAndNewLines(ustr, &i, &newColor, &colorChanged, &skippedLines, &baseTextColor))
			return;

		cc = utf8::GetNextChar(str, i);

		if (colorChanged) {
			if (autoOutlineColor) {
				SetColors(&newColor, nullptr);
			} else {
				SetTextColor(&newColor);
			}
		}


		const GlyphInfo* cg = &GetGlyph(cc);
		const GlyphInfo* pg = nullptr;

		if (skippedLines > 0) {
			x  = startx;
			y -= (skippedLines * lineHeight_);
		} else if (pc != 0) {
			pg = &GetGlyph(pc);
			x += (scaleX * GetKerning(*pg, *cg));
		}

		pg = cg;
		pc = cc;

		const auto&  tc = pg->texCord;
		const float dx0 = (scaleX * pg->size.x0()) + x;
		const float dy0 = (scaleY * pg->size.y0()) + y;
		const float dx1 = (scaleX * pg->size.x1()) + x;
		const float dy1 = (scaleY * pg->size.y1()) + y;
		const float tx0 = tc.x0() * texScaleX;
		const float ty0 = tc.y0() * texScaleY;
		const float tx1 = tc.x1() * texScaleX;
		const float ty1 = tc.y1() * texScaleY;

		if (!inBeginEndGL4 && !bufferedWriteGL4) {
			va.AddVertexQ2dT(dx0, dy1, tx0, ty1);
			va.AddVertexQ2dT(dx0, dy0, tx0, ty0);
			va.AddVertexQ2dT(dx1, dy0, tx1, ty0);
			va.AddVertexQ2dT(dx1, dy1, tx1, ty1);
			continue;
		}

		if (((curBufferPos[pbi] - mapBufferPtr[pbi]) / 4) < NUM_BUFFER_QUADS) {
			*(curBufferPos[pbi]++) = {{dx0, dy1, textDepth.x},  tx0, ty1,  (&newColor.x)};
			*(curBufferPos[pbi]++) = {{dx0, dy0, textDepth.x},  tx0, ty0,  (&newColor.x)};
			*(curBufferPos[pbi]++) = {{dx1, dy0, textDepth.x},  tx1, ty0,  (&newColor.x)};
			*(curBufferPos[pbi]++) = {{dx1, dy1, textDepth.x},  tx1, ty1,  (&newColor.x)};
		}
	} while (true);
}


void CglFont::RenderStringShadow(float x, float y, float scaleX, float scaleY, const std::string& str)
{
	#if 0
	RenderString(x, y, scaleX, scaleY, str);
	return;
	#endif

	const std::u8string& ustr = toustring(str);

	const float startx = x;
	const float shiftX = scaleX * 0.1;
	const float shiftY = scaleY * 0.1;
	const float ssX = (scaleX / fontSize) * GetOutlineWidth();
	const float ssY = (scaleY / fontSize) * GetOutlineWidth();
	const float lineHeight_ = scaleY * GetLineHeight();
	const size_t length = str.length();

	if (!inBeginEndGL4) {
		va.EnlargeArrays(length * 4, 0, VA_SIZE_2DT);
		va2.EnlargeArrays(length * 4, 0, VA_SIZE_2DT);
	}

	const unsigned int pbi = GetBufferIdx(PRIMARY_BUFFER);
	const unsigned int obi = GetBufferIdx(OUTLINE_BUFFER);

	int i = 0;
	int skippedLines = 0;
	bool colorChanged = false;

	char32_t cc = 0;
	char32_t pc = 0;

	float4 newColor = textColor;

	const float texScaleX = 1.0f / texWidth;
	const float texScaleY = 1.0f / texHeight;

	do {
		// check for end-of-string
		if (SkipColorCodesAndNewLines(ustr, &i, &newColor, &colorChanged, &skippedLines, &baseTextColor))
			return;

		cc = utf8::GetNextChar(str, i);

		if (colorChanged) {
			if (autoOutlineColor) {
				SetColors(&newColor, nullptr);
			} else {
				SetTextColor(&newColor);
			}
		}


		const GlyphInfo* cg = &GetGlyph(cc);
		const GlyphInfo* pg = nullptr;

		if (skippedLines > 0) {
			x  = startx;
			y -= (skippedLines * lineHeight_);
		} else if (pc != 0) {
			pg = &GetGlyph(pc);
			x += (scaleX * GetKerning(*pg, *cg));
		}

		pg = cg;
		pc = cc;


		const auto&  tc = pg->texCord;
		const auto& stc = pg->shadowTexCord;
		const float dx0 = (scaleX * pg->size.x0()) + x;
		const float dy0 = (scaleY * pg->size.y0()) + y;
		const float dx1 = (scaleX * pg->size.x1()) + x;
		const float dy1 = (scaleY * pg->size.y1()) + y;
		const float tx0 = tc.x0() * texScaleX;
		const float ty0 = tc.y0() * texScaleY;
		const float tx1 = tc.x1() * texScaleX;
		const float ty1 = tc.y1() * texScaleY;
		const float stx0 = stc.x0() * texScaleX;
		const float sty0 = stc.y0() * texScaleY;
		const float stx1 = stc.x1() * texScaleX;
		const float sty1 = stc.y1() * texScaleY;

		if (!inBeginEndGL4 && !bufferedWriteGL4) {
			// draw shadow
			va2.AddVertexQ2dT(dx0 + shiftX - ssX, dy1 - shiftY - ssY, stx0, sty1);
			va2.AddVertexQ2dT(dx0 + shiftX - ssX, dy0 - shiftY + ssY, stx0, sty0);
			va2.AddVertexQ2dT(dx1 + shiftX + ssX, dy0 - shiftY + ssY, stx1, sty0);
			va2.AddVertexQ2dT(dx1 + shiftX + ssX, dy1 - shiftY - ssY, stx1, sty1);

			// draw the actual character
			va.AddVertexQ2dT(dx0, dy1, tx0, ty1);
			va.AddVertexQ2dT(dx0, dy0, tx0, ty0);
			va.AddVertexQ2dT(dx1, dy0, tx1, ty0);
			va.AddVertexQ2dT(dx1, dy1, tx1, ty1);
			continue;
		}

		if (((curBufferPos[obi] - mapBufferPtr[obi]) / 4) < NUM_BUFFER_QUADS) {
			*(curBufferPos[obi]++) = {{dx0 + shiftX - ssX, dy1 - shiftY - ssY, textDepth.y},  stx0, sty1,  (&outlineColor.x)};
			*(curBufferPos[obi]++) = {{dx0 + shiftX - ssX, dy0 - shiftY + ssY, textDepth.y},  stx0, sty0,  (&outlineColor.x)};
			*(curBufferPos[obi]++) = {{dx1 + shiftX + ssX, dy0 - shiftY + ssY, textDepth.y},  stx1, sty0,  (&outlineColor.x)};
			*(curBufferPos[obi]++) = {{dx1 + shiftX + ssX, dy1 - shiftY - ssY, textDepth.y},  stx1, sty1,  (&outlineColor.x)};
		}
		if (((curBufferPos[pbi] - mapBufferPtr[pbi]) / 4) < NUM_BUFFER_QUADS) {
			*(curBufferPos[pbi]++) = {{dx0, dy1, textDepth.x},  tx0, ty1,  (&newColor.x)};
			*(curBufferPos[pbi]++) = {{dx0, dy0, textDepth.x},  tx0, ty0,  (&newColor.x)};
			*(curBufferPos[pbi]++) = {{dx1, dy0, textDepth.x},  tx1, ty0,  (&newColor.x)};
			*(curBufferPos[pbi]++) = {{dx1, dy1, textDepth.x},  tx1, ty1,  (&newColor.x)};
		}
	} while (true);
}

void CglFont::RenderStringOutlined(float x, float y, float scaleX, float scaleY, const std::string& str)
{
	#if 0
	RenderString(x, y, scaleX, scaleY, str);
	return;
	#endif

	const std::u8string& ustr = toustring(str);

	const float startx = x;
	const float shiftX = (scaleX / fontSize) * GetOutlineWidth();
	const float shiftY = (scaleY / fontSize) * GetOutlineWidth();
	const float lineHeight_ = scaleY * GetLineHeight();
	const size_t length = str.length();

	if (!inBeginEndGL4) {
		va.EnlargeArrays(length * 4, 0, VA_SIZE_2DT);
		va2.EnlargeArrays(length * 4, 0, VA_SIZE_2DT);
	}

	const unsigned int pbi = GetBufferIdx(PRIMARY_BUFFER);
	const unsigned int obi = GetBufferIdx(OUTLINE_BUFFER);

	int i = 0;
	int skippedLines = 0;
	bool colorChanged = false;

	char32_t cc = 0;
	char32_t pc = 0;

	float4 newColor = textColor;

	const float texScaleX = 1.0f / texWidth;
	const float texScaleY = 1.0f / texHeight;

	do {
		// check for end-of-string
		if (SkipColorCodesAndNewLines(ustr, &i, &newColor, &colorChanged, &skippedLines, &baseTextColor))
			return;

		cc = utf8::GetNextChar(str, i);

		if (colorChanged) {
			if (autoOutlineColor) {
				SetColors(&newColor, nullptr);
			} else {
				SetTextColor(&newColor);
			}
		}


		const GlyphInfo* cg = &GetGlyph(cc);
		const GlyphInfo* pg = nullptr;

		if (skippedLines > 0) {
			x  = startx;
			y -= (skippedLines * lineHeight_);
		} else if (pc != 0) {
			pg = &GetGlyph(pc);
			x += (scaleX * GetKerning(*pg, *cg));
		}

		pg = cg;
		pc = cc;


		const auto&  tc = pg->texCord;
		const auto& stc = pg->shadowTexCord;
		const float dx0 = (scaleX * pg->size.x0()) + x;
		const float dy0 = (scaleY * pg->size.y0()) + y;
		const float dx1 = (scaleX * pg->size.x1()) + x;
		const float dy1 = (scaleY * pg->size.y1()) + y;
		const float tx0 = tc.x0() * texScaleX;
		const float ty0 = tc.y0() * texScaleY;
		const float tx1 = tc.x1() * texScaleX;
		const float ty1 = tc.y1() * texScaleY;
		const float stx0 = stc.x0() * texScaleX;
		const float sty0 = stc.y0() * texScaleY;
		const float stx1 = stc.x1() * texScaleX;
		const float sty1 = stc.y1() * texScaleY;

		if (!inBeginEndGL4 && !bufferedWriteGL4) {
			// draw outline
			va2.AddVertexQ2dT(dx0 - shiftX, dy1 - shiftY, stx0, sty1);
			va2.AddVertexQ2dT(dx0 - shiftX, dy0 + shiftY, stx0, sty0);
			va2.AddVertexQ2dT(dx1 + shiftX, dy0 + shiftY, stx1, sty0);
			va2.AddVertexQ2dT(dx1 + shiftX, dy1 - shiftY, stx1, sty1);

			// draw the actual character
			va.AddVertexQ2dT(dx0, dy1, tx0, ty1);
			va.AddVertexQ2dT(dx0, dy0, tx0, ty0);
			va.AddVertexQ2dT(dx1, dy0, tx1, ty0);
			va.AddVertexQ2dT(dx1, dy1, tx1, ty1);
			continue;
		}

		if (((curBufferPos[obi] - mapBufferPtr[obi]) / 4) < NUM_BUFFER_QUADS) {
			*(curBufferPos[obi]++) = {{dx0 - shiftX, dy1 - shiftY, textDepth.y},  stx0, sty1,  (&outlineColor.x)};
			*(curBufferPos[obi]++) = {{dx0 - shiftX, dy0 + shiftY, textDepth.y},  stx0, sty0,  (&outlineColor.x)};
			*(curBufferPos[obi]++) = {{dx1 + shiftX, dy0 + shiftY, textDepth.y},  stx1, sty0,  (&outlineColor.x)};
			*(curBufferPos[obi]++) = {{dx1 + shiftX, dy1 - shiftY, textDepth.y},  stx1, sty1,  (&outlineColor.x)};
		}
		if (((curBufferPos[pbi] - mapBufferPtr[pbi]) / 4) < NUM_BUFFER_QUADS) {
			*(curBufferPos[pbi]++) = {{dx0, dy1, textDepth.x},  tx0, ty1,  (&newColor.x)};
			*(curBufferPos[pbi]++) = {{dx0, dy0, textDepth.x},  tx0, ty0,  (&newColor.x)};
			*(curBufferPos[pbi]++) = {{dx1, dy0, textDepth.x},  tx1, ty0,  (&newColor.x)};
			*(curBufferPos[pbi]++) = {{dx1, dy1, textDepth.x},  tx1, ty1,  (&newColor.x)};
		}
	} while (true);
}




void CglFont::glWorldBegin()
{
	if (threadSafety)
		bufferMutex.lock();

	curShader = GetPrimaryShader();
	curShader->Enable();
	curShader->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, camera->GetProjectionMatrix());

	UpdateTexture();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, GetTexture());

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void CglFont::glWorldPrint(const float3& p, const float size, const std::string& str)
{
	// must be preceded by glWorldBegin
	if (curShader == nullptr)
		return;

	const CMatrix44f& vm = camera->GetViewMatrix();
	const CMatrix44f& bm = camera->GetBillBoardMatrix();

	// TODO: simplify
	curShader->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, vm * CMatrix44f(p, RgtVector, UpVector, FwdVector) * bm);
	glPrint(0.0f, 0.0f, size, FONT_DESCENDER | FONT_CENTER | FONT_OUTLINE | FONT_BUFFERED, str);

	const unsigned int pbi = GetBufferIdx(PRIMARY_BUFFER);
	const unsigned int obi = GetBufferIdx(OUTLINE_BUFFER);

	outlineBuffer[currBufferIdxGL4].Submit(GL_QUADS, (prvBufferPos[obi] - mapBufferPtr[obi]), (curBufferPos[obi] - prvBufferPos[obi]));
	primaryBuffer[currBufferIdxGL4].Submit(GL_QUADS, (prvBufferPos[pbi] - mapBufferPtr[pbi]), (curBufferPos[pbi] - prvBufferPos[pbi]));

	prvBufferPos[pbi] = curBufferPos[pbi];
	prvBufferPos[obi] = curBufferPos[obi];
}

void CglFont::glWorldEnd()
{
	curShader->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, CMatrix44f::Identity());
	curShader->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, CMatrix44f::OrthoProj(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f));

	glPopAttrib();
	glBindTexture(GL_TEXTURE_2D, 0);

	curShader = nullptr;

	if (threadSafety)
		bufferMutex.unlock();
}




void CglFont::glPrint(float x, float y, float s, const int options, const std::string& text)
{
	// s := scale or absolute size?
	if (options & FONT_SCALE)
		s *= fontSize;

	float sizeX = s;
	float sizeY = s;

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
		y -= sizeY * 0.5f * GetTextHeight(text, &textDescender);
		y -= sizeY * 0.5f * textDescender;
	} else if (options & FONT_TOP) {
		y -= sizeY * GetTextHeight(text);
	} else if (options & FONT_ASCENDER) {
		y -= sizeY * GetDescender();
		y -= sizeY;
	} else if (options & FONT_BOTTOM) {
		float textDescender;
		GetTextHeight(text, &textDescender);
		y -= sizeY * textDescender;
	}

	if (options & FONT_NEAREST) {
		x = (int)x;
		y = (int)y;
	}

	// backup text & outline colors (also ::ColorResetIndicator will reset to those)
	baseTextColor = textColor;
	baseOutlineColor = outlineColor;

	const bool buffered = ((options & FONT_BUFFERED) != 0);
	// immediate mode?
	const bool immediate = (!inBeginEnd && !inBeginEndGL4 && !buffered);

	if (immediate) {
		Begin(!(options & (FONT_OUTLINE | FONT_SHADOW)));
	} else if (buffered) {
		if (threadSafety && !inBeginEndGL4)
			bufferMutex.lock();

		bufferedWriteGL4 = !inBeginEndGL4;
		pendingFinishGL4 |= ((options & FONT_FINISHED) != 0);
	}

	// any data that was buffered but not submitted last frame gets discarded, user error
	if (lastPrintFrameGL4 != globalRendering->drawFrame) {
		lastPrintFrameGL4 = globalRendering->drawFrame;
		currBufferIdxGL4 = (currBufferIdxGL4 + 1) & 1;

		ResetBufferGL4(false);
		ResetBufferGL4( true);
	}

	// select correct decoration RenderString function
	if (options & FONT_OUTLINE) {
		RenderStringOutlined(x, y, sizeX, sizeY, text);
	} else if (options & FONT_SHADOW) {
		RenderStringShadow(x, y, sizeX, sizeY, text);
	} else {
		RenderString(x, y, sizeX, sizeY, text);
	}

	if (immediate) {
		End();
	} else if (buffered) {
		bufferedWriteGL4 = false;

		if (threadSafety && !inBeginEndGL4)
			bufferMutex.unlock();
	}

	// reset text & outline colors (if changed via in-text colorcodes)
	SetColors(&baseTextColor, &baseOutlineColor);
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

		maxHeight = std::max(maxHeight, textHeight);
		minDescender = std::min(minDescender, textDescender);
	}

	// s := scale or absolute size?
	float ss = s;
	if (options & FONT_SCALE)
		ss *= fontSize;

	float sizeX = ss;
	float sizeY = ss;

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

	if (fmt == nullptr)
		return;

	va_list ap;
	va_start(ap, fmt);
	VSNPRINTF(out, sizeof(out), fmt, ap);
	va_end(ap);
	glPrint(x, y, s, options, std::string(out));
}


