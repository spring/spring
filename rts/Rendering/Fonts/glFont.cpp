/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "glFont.h"
#include "glFontRenderer.h"
#include "FontLogSection.h"

#include <cstdarg>
#include <stdexcept>

#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "System/Color.h"
#include "System/Exceptions.h"
#include "System/SpringMath.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

#undef GetCharWidth // winapi.h

CONFIG(std::string,      FontFile).defaultValue("fonts/FreeSansBold.otf").description("Sets the font of Spring engine text.");
CONFIG(std::string, SmallFontFile).defaultValue("fonts/FreeSansBold.otf").description("Sets the font of Spring engine small text.");

CONFIG(int,      FontSize).defaultValue(23).description("Sets the font size (in pixels) of the MainMenu and more.");
CONFIG(int, SmallFontSize).defaultValue(14).description("Sets the font size (in pixels) of the engine GUIs and more.");
CONFIG(int,      FontOutlineWidth).defaultValue(3).description("Sets the width of the black outline around Spring engine text, such as the title screen version number, clock, and basic UI. Does not affect LuaUI elements.");
CONFIG(int, SmallFontOutlineWidth).defaultValue(2).description("see FontOutlineWidth");
CONFIG(float,      FontOutlineWeight).defaultValue(25.0f).description("Sets the opacity of Spring engine text, such as the title screen version number, clock, and basic UI. Does not affect LuaUI elements.");
CONFIG(float, SmallFontOutlineWeight).defaultValue(10.0f).description("see FontOutlineWeight");

std::shared_ptr<CglFont> font = nullptr;
std::shared_ptr<CglFont> smallFont = nullptr;

static constexpr float4        white(1.00f, 1.00f, 1.00f, 0.95f);
static constexpr float4  darkOutline(0.05f, 0.05f, 0.05f, 0.95f);
static constexpr float4 lightOutline(0.95f, 0.95f, 0.95f, 0.80f);

static const float darkLuminosity = 0.05f +
	0.2126f * std::pow(darkOutline[0], 2.2f) +
	0.7152f * std::pow(darkOutline[1], 2.2f) +
	0.0722f * std::pow(darkOutline[2], 2.2f);



bool CglFont::LoadConfigFonts()
{
	font      = CglFont::LoadFont("", false);
	smallFont = CglFont::LoadFont("", true);

	if (font      == nullptr)
		throw content_error("Failed to load FontFile \"" + configHandler->GetString("FontFile") + "\", did you forget to run make install?");

	if (smallFont == nullptr)
		throw content_error("Failed to load SmallFontFile \"" + configHandler->GetString("SmallFontFile") + "\", did you forget to run make install?");

	return true;
}

bool CglFont::LoadCustomFonts(const std::string& smallFontFile, const std::string& largeFontFile)
{
	auto newLargeFont = CglFont::LoadFont(largeFontFile, false);
	auto newSmallFont = CglFont::LoadFont(smallFontFile,  true);

	if (newLargeFont != nullptr && newSmallFont != nullptr) {
		font = newLargeFont;
		smallFont = newSmallFont;

		LOG("[%s] loaded fonts \"%s\" and \"%s\"", __func__, smallFontFile.c_str(), largeFontFile.c_str());
		configHandler->SetString(     "FontFile", largeFontFile);
		configHandler->SetString("SmallFontFile", smallFontFile);
	}

	return true;
}

std::shared_ptr<CglFont> CglFont::LoadFont(const std::string& fontFileOverride, bool smallFont)
{
	const std::string fontFiles[] = {configHandler->GetString("FontFile"), configHandler->GetString("SmallFontFile")};
	const std::string& fontFile = (fontFileOverride.empty())? fontFiles[smallFont]: fontFileOverride;

	const   int fontSizes[] = {configHandler->GetInt("FontSize"), configHandler->GetInt("SmallFontSize")};
	const   int fontWidths[] = {configHandler->GetInt("FontOutlineWidth"), configHandler->GetInt("SmallFontOutlineWidth")};
	const float fontWeights[] = {configHandler->GetFloat("FontOutlineWeight"), configHandler->GetFloat("SmallFontOutlineWeight")};

	return std::move(CglFont::LoadFont(fontFile, fontSizes[smallFont], fontWidths[smallFont], fontWeights[smallFont]));
}


std::shared_ptr<CglFont> CglFont::LoadFont(const std::string& fontFile, int size, int outlinewidth, float outlineweight)
{
	try {
		//return (new CglFont(fontFile, size, outlinewidth, outlineweight));
		auto fnt = FindFont(fontFile, size, outlinewidth, outlineweight);
		if (fnt)
			return fnt;

		return std::static_pointer_cast<CglFont>(
			allFonts.emplace_back(
				std::make_shared<CglFont>(fontFile, size, outlinewidth, outlineweight)
			)
			.lock()
		);

	} catch (const content_error& ex) {
		LOG_L(L_ERROR, "Failed creating font: %s", ex.what());
		return nullptr;
	}
}

std::shared_ptr<CglFont> CglFont::FindFont(const std::string& fontFile, int size, int outlinewidth, float outlineweight)
{
	const auto cmpFunc = [&fontFile, size, outlinewidth, outlineweight](std::weak_ptr<CFontTexture> item) {
		std::shared_ptr<CglFont> font = std::static_pointer_cast<CglFont>(item.lock());
		return
			size == font->GetSize() &&
			outlinewidth == font->GetOutlineWidth() &&
			outlineweight == font->GetOutlineWeight() &&
			fontFile == font->GetFilePath();
	};

	// check for unused fonts and search in the same time
	size_t fontIndex = size_t(-1);
	for (size_t i = 0; i < allFonts.size(); /*NOOP*/) {
		if (allFonts[i].expired()) {
			allFonts[i] = std::move(allFonts.back());
			allFonts.pop_back();
		}
		else {
			if (fontIndex == size_t(-1) && cmpFunc(allFonts[i]))
				fontIndex = i;

			++i;
		}
	}

	if (fontIndex == size_t(-1))
		return nullptr;

	return std::static_pointer_cast<CglFont>(allFonts[fontIndex].lock());
}


void CglFont::ReallocSystemFontAtlases(bool pre)
{
#ifdef _DEBUG
	size_t fontsCounter = 0;
	for (const auto& f : allFonts) {
		if (!f.expired())
			++fontsCounter;
	}

	assert(fontsCounter <= 2);
#endif

	if (font != nullptr)		
		font->ReallocAtlases(pre);
	if (smallFont != nullptr && smallFont != font)
		smallFont->ReallocAtlases(pre);
}

CglFont::CglFont(const std::string& fontFile, int size, int _outlineWidth, float _outlineWeight)
	: CTextWrap(fontFile, size, _outlineWidth, _outlineWeight)
	, fontPath(fontFile)
	, stringWidth {
		1 << 12,
		[f = this](const std::string& str) { return f->GetTextWidth_(toustring(str)); },
		[](const std::string& str, const auto& cache) {}, //don't save anything
	}
	, stringHeight {
		1 << 10,
		[f = this](const std::string& str) { HeightCache hc; hc.height = f->GetTextHeight_(toustring(str), &hc.descender, &hc.numLines); return hc; },
		[](const std::string& str, const auto& cache) {}, //don't save anything
	}
{
	textColor    = white;
	outlineColor = darkOutline;

	viewMatrix = DefViewMatrix();
	projMatrix = DefProjMatrix();

	fontMutexes = {
		std::make_unique<spring::mutex_wrapper<spring::noop_mutex     >>(),
		std::make_unique<spring::mutex_wrapper<spring::recursive_mutex>>()
	};

	fontRenderer = CglFontRenderer::CreateInstance();
}

CglFont::~CglFont() {
#ifndef HEADLESS
	CglFontRenderer::DeleteInstance(fontRenderer);
#endif
}

#ifdef HEADLESS
void CglFont::Begin() {}
void CglFont::End() {}
void CglFont::DrawBuffered() {}
void CglFont::DrawWorldBuffered() {}

void CglFont::glWorldPrint(const float3& p, const float size, const std::string& str, int options) {}

CMatrix44f CglFont::DefViewMatrix() { return CMatrix44f::Identity(); }
CMatrix44f CglFont::DefProjMatrix() { return CMatrix44f::Identity(); }

void CglFont::glPrint(float x, float y, float s, const int options, const std::string& str) {}
void CglFont::glPrintTable(float x, float y, float s, const int options, const std::string& str) {}

void CglFont::SetAutoOutlineColor(bool enable) {}
void CglFont::SetTextColor(const float4* color) {}
void CglFont::SetOutlineColor(const float4* color) {}
void CglFont::SetColors(const float4* textColor, const float4* outlineColor) {}

float CglFont::GetCharacterWidth(const char32_t c) { return 1.0f; }
void CglFont::ScanForWantedGlyphs(const std::u8string& str) {}
float CglFont::GetTextWidth_(const std::u8string& text) { return (text.size() * 1.0f); }
float CglFont::GetTextHeight_(const std::u8string& text, float* descender, int* numLines) { return 1.0f; }

std::deque<std::string> CglFont::SplitIntoLines(const std::u8string& text) { return {}; }

void CglFont::GetStats(std::array<size_t, 8>& stats) const {}

#else



// helper for GetText{Width,Height}
template <typename T>
static inline T SkipColorCodes(const std::u8string& text, T idx)
{
	while (idx < text.size() && text[idx] == CglFont::ColorCodeIndicator) {
		idx += 4;
	}

	return (std::min(T(text.size()), idx));
}

// helper for RenderString*
template <typename T>
static inline bool SkipColorCodesAndNewLines(
	const std::u8string& text,
	const CglFont::ColorCodeCallBack& cccb,
	T* curIndex,
	T* numLines,
	float4* color,
	float4* colorReset
) {
	T idx = *curIndex;
	T nls = 0;

	char32_t nextChar = 0;
	for (T end = T(text.length()); idx < end; ) {
		switch (nextChar = utf8::GetNextChar(text, idx, false/*do not advance*/)) {
			case CglFont::ColorCodeIndicator: {
				if ((idx += 4) < end)
					cccb(*color = {text[idx - 3] / 255.0f, text[idx - 2] / 255.0f, text[idx - 1] / 255.0f, 1.0f});

			} break;

			case CglFont::ColorResetIndicator: {
				idx += 1;
				cccb(*color = *colorReset);
			} break;

			case 0x0D: {
				idx += (idx < end && text[idx + 1] == 0x0A);
				[[fallthrough]]; // CR; fall-through
			}
			case 0x0A: {
				// LF
				idx += 1;
				nls += 1;
			} break;

			default: {
				// skip any non-printable ASCII chars which can only occur with
				// malformed color-codes (e.g. when the ColorCodeIndicator byte
				// is missing)
				// idx += (text[idx] >= 127 && text[idx] <= 255);
				*curIndex = idx;
				*numLines = nls;
				return false;
			} break;
		}
	}

	*curIndex = idx;
	*numLines = nls;
	return true;
}




float CglFont::GetCharacterWidth(const char32_t c)
{
	const auto& glyph = GetGlyph(c);
	assert(&glyph != &CFontTexture::dummyGlyph);
	return glyph.advance;
}

float CglFont::GetTextWidth_(const std::u8string& text)
{
	if (text.empty())
		return 0.0f;

	std::lock_guard lk(*GetFontMutex());

	ScanForWantedGlyphs(text);

	float curw = 0.0f;
	float maxw = 0.0f;

	char32_t prvGlyphIdx = 0;
	char32_t curGlyphIdx = 0;

	const GlyphInfo* prvGlyphPtr = nullptr;
	const GlyphInfo* curGlyphPtr = nullptr;

	for (int idx = 0, end = int(text.length()); idx < end; ) {
		#if 0
		// see SkipColorCodesAndNewLines
		if (text[idx] >= 127 && text[idx] <= 255) {
			idx++;
			continue;
		}
		#endif

		switch (curGlyphIdx = utf8::GetNextChar(text, idx)) {
			// inlined colorcode; subtract 1 since GetNextChar increments idx
			case ColorCodeIndicator: {
				idx = SkipColorCodes(text, idx - 1);
			} break;

			// reset color; no-op since GetNextChar increments idx
			case ColorResetIndicator: {
			} break;

			case 0x0D: {
				idx += (idx < end && text[idx] == 0x0A);
				[[fallthrough]]; // CR; fall-through
			}
			case 0x0A: {
				// LF
				if (prvGlyphPtr != nullptr)
					curw += GetCharacterWidth(prvGlyphIdx);

				maxw = std::max(curw, maxw);
				curw = 0.0f;

				prvGlyphPtr = nullptr;
			} break;

			// printable char
			default: {
				curGlyphPtr = &GetGlyph(curGlyphIdx);
				assert(curGlyphPtr != &CFontTexture::dummyGlyph);

				if (prvGlyphPtr != nullptr) {
					prvGlyphPtr = &GetGlyph(prvGlyphIdx);
					assert(prvGlyphPtr != &CFontTexture::dummyGlyph);
					curw += GetKerning(*prvGlyphPtr, *curGlyphPtr);
				}

				prvGlyphPtr = curGlyphPtr;
				prvGlyphIdx = curGlyphIdx;
			} break;
		}
	}

	if (prvGlyphPtr != nullptr)
		curw += GetCharacterWidth(prvGlyphIdx);

	return (std::max(curw, maxw));
}


float CglFont::GetTextHeight_(const std::u8string& text, float* descender, int* numLines)
{
	if (text.empty()) {
		if (descender != nullptr) *descender = 0.0f;
		if (numLines != nullptr) *numLines = 0;
		return 0.0f;
	}

	std::lock_guard lk(*GetFontMutex());

	ScanForWantedGlyphs(text);

	float h = 0.0f;
	float d = GetLineHeight() + GetDescender();

	unsigned int multiLine = 1;

	for (int idx = 0, end = int(text.length()); idx < end; ) {
		#if 0
		// see SkipColorCodesAndNewLines
		if (text[idx] >= 127 && text[idx] <= 255) {
			idx++;
			continue;
		}
		#endif

		const char32_t u = utf8::GetNextChar(text, idx);

		switch (u) {
			// inlined colorcode; subtract 1 since GetNextChar increments idx
			case ColorCodeIndicator: {
				idx = SkipColorCodes(text, idx - 1);
			} break;

			// reset color; no-op since GetNextChar increments idx
			case ColorResetIndicator: {
			} break;

			case 0x0D: {
				idx += (idx < end && text[idx] == 0x0A);
				[[fallthrough]]; // CR; fall-through
			}
			case 0x0A: {
				// LF
				multiLine++;
				d = GetLineHeight() + GetDescender();
			} break;

			// printable char
			default: {
				const GlyphInfo& g = GetGlyph(u);
				assert(&g != &CFontTexture::dummyGlyph);

				d = std::min(d, g.descender);
				h = std::max(h, g.height * (multiLine < 2)); // only calculate height for the first line
			} break;
		}
	}

	d -= ((multiLine - 1) * GetLineHeight() * (multiLine > 1));

	if (descender != nullptr) *descender = d;
	if (numLines != nullptr) *numLines = multiLine;

	return h;
}

void CglFont::ScanForWantedGlyphs(const std::u8string& ustr)
{
	static std::vector<char32_t> missingGlyphs;
	missingGlyphs.clear();

	char32_t nextChar = 0;

	for (int idx = 0, end = int(ustr.length()); idx < end; ) {
		switch (nextChar = utf8::GetNextChar(ustr, idx)) {
			// inlined colorcode; subtract 1 since GetNextChar increments idx
		case ColorCodeIndicator: {
			idx = SkipColorCodes(ustr, idx - 1);
		} break;

			// reset color; no-op since GetNextChar increments idx
		case ColorResetIndicator: {
		} break;

		case 0x0D: {
			idx += (idx < end&& ustr[idx] == 0x0A);
			[[fallthrough]]; // CR; fall-through
		}
		case 0x0A: {
			// LF
		} break;

			// printable char
		default: {
			const GlyphInfo& curGlyph = GetGlyph(nextChar);
			if (&curGlyph == &CFontTexture::dummyGlyph)
				missingGlyphs.emplace_back(nextChar);
		} break;
		}
	}

	LoadWantedGlyphs(missingGlyphs);
}


std::deque<std::string> CglFont::SplitIntoLines(const std::u8string& text)
{
	std::deque<std::string> lines;
	std::deque<std::string> colorCodeStack;

	if (text.empty())
		return lines;

	lines.emplace_back("");

	for (int idx = 0, end = text.length(); idx < end; idx++) {
		const char8_t& c = text[idx];

		switch (c) {
			// inlined colorcode; push to stack if [I,R,G,B] is followed by more text
			case ColorCodeIndicator: {
				if ((idx + 4) < end) {
					colorCodeStack.emplace_back(std::move(text.substr(idx, 4)));
					lines.back() += colorCodeStack.back();

					// compensate for loop-incr
					idx -= 1;
					idx += 4;
				}
			} break;

			// reset color
			case ColorResetIndicator: {
				if (!colorCodeStack.empty())
					colorCodeStack.pop_back();
				lines.back() += c;
			} break;

			case 0x0D: {
				idx += ((idx + 1) < end && text[idx + 1] == 0x0A);
				[[fallthrough]]; // CR; fall-through
			}
			case 0x0A: {
				lines.emplace_back("");

				#if 0
				for (auto& color: colorCodeStack)
					lines.back() = color;
				#else
				if (!colorCodeStack.empty())
					lines.back() = colorCodeStack.back();
				#endif
			} break;

			default: {
				// printable char or orphaned (c >= 127 && c <= 255) color-code
				lines.back() += c;
			} break;
		}
	}

	return lines;
}




void CglFont::SetAutoOutlineColor(bool enable)
{
	std::lock_guard lk(*GetFontMutex());

	autoOutlineColor = enable;
}

void CglFont::SetTextColor(const float4* color)
{
	if (color == nullptr)
		color = &white;

	std::lock_guard lk(*GetFontMutex());

	textColor = *color;
}

void CglFont::SetOutlineColor(const float4* color)
{
	if (color == nullptr)
		color = ChooseOutlineColor(textColor);

	std::lock_guard lk(*GetFontMutex());

	outlineColor = *color;
}


void CglFont::SetColors(const float4* _textColor, const float4* _outlineColor)
{
	SetTextColor(_textColor);
	SetOutlineColor(_outlineColor);
}

const float4* CglFont::ChooseOutlineColor(const float4& textColor)
{
	const float luminosity =
		0.2126f * std::pow(textColor[0], 2.2f) +
		0.7152f * std::pow(textColor[1], 2.2f) +
		0.0722f * std::pow(textColor[2], 2.2f);

	const float maxLum = std::max(luminosity + 0.05f, darkLuminosity);
	const float minLum = std::min(luminosity + 0.05f, darkLuminosity);

	if ((maxLum / minLum) > 5.0f)
		return &darkOutline;

	return &lightOutline;
}

void CglFont::Begin() {
	GetFontMutex()->lock();

	if (inBeginEndBlock) {
		GetFontMutex()->unlock();
		return;
	}

	inBeginEndBlock = true;
}

void CglFont::End() {
	if (!inBeginEndBlock) {
		LOG_L(L_ERROR, "called End() without Begin()");
		return;
	}
	inBeginEndBlock = false;

	UpdateGlyphAtlasTexture();
	//without this, fonts textures are empty in display lists (probably GL commands in UploadGlyphAtlasTexture are get recorded as part of the list)
	GLint dl = 0;
	glGetIntegerv(GL_LIST_INDEX, &dl);
	if (dl == 0) {
		UploadGlyphAtlasTexture();
	}
	fontRenderer->PushGLState(this);
	fontRenderer->DrawTraingleElements();
	fontRenderer->PopGLState();

	inBeginEndBlock = false;
	GetFontMutex()->unlock();
}


void CglFont::DrawBuffered()
{
	std::lock_guard lk(*GetFontMutex());

	UpdateGlyphAtlasTexture();
	UploadGlyphAtlasTexture();

	fontRenderer->PushGLState(this);
	fontRenderer->DrawTraingleElements();
	fontRenderer->PopGLState();
}

void CglFont::DrawWorldBuffered()
{
	glPushMatrix();
	glMultMatrixf(camera->GetBillBoardMatrix());

	DrawBuffered();

	glPopMatrix();
}

template<int shiftXC, int shiftYC, bool outline>
void CglFont::RenderStringImpl(float x, float y, float scaleX, float scaleY, const std::string& str, const ColorCodeCallBack& cccb)
{
	const std::u8string& ustr = toustring(str);

	ScanForWantedGlyphs(ustr);

	const float startx = x;
	const float lineHeight_ = scaleY * GetLineHeight();

	char32_t curGlyphIdx = 0;
	char32_t prvGlyphIdx = 0;

	int currentPos = 0;
	int skippedLines = 0;

	float4 newColor = textColor;

	constexpr float texScaleX = 1.0f;
	constexpr float texScaleY = 1.0f;

	// check for end-of-string
	while (!SkipColorCodesAndNewLines(ustr, cccb, &currentPos, &skippedLines, &newColor, &baseTextColor)) {
		curGlyphIdx = utf8::GetNextChar(ustr, currentPos);

		const GlyphInfo* curGlyphPtr = &GetGlyph(curGlyphIdx);
		assert(curGlyphPtr != &CFontTexture::dummyGlyph);
		const GlyphInfo* prvGlyphPtr = nullptr;

		if (skippedLines > 0) {
			x = startx;
			y -= (skippedLines * lineHeight_);
		}
		else if (prvGlyphIdx != 0) {
			prvGlyphPtr = &GetGlyph(prvGlyphIdx);
			assert(prvGlyphPtr != &CFontTexture::dummyGlyph);
			x += (scaleX * GetKerning(*prvGlyphPtr, *curGlyphPtr));
		}

		prvGlyphPtr = curGlyphPtr;
		prvGlyphIdx = curGlyphIdx;


		const auto& tc = prvGlyphPtr->texCord;
		const float dx0 = (scaleX * prvGlyphPtr->size.x0()) + x;
		const float dy0 = (scaleY * prvGlyphPtr->size.y0()) + y;
		const float dx1 = (scaleX * prvGlyphPtr->size.x1()) + x;
		const float dy1 = (scaleY * prvGlyphPtr->size.y1()) + y;
		const float tx0 = tc.x0() * texScaleX;
		const float ty0 = tc.y0() * texScaleY;
		const float tx1 = tc.x1() * texScaleX;
		const float ty1 = tc.y1() * texScaleY;


		if constexpr (shiftXC > 0 || shiftYC > 0 || outline) {
			const auto& stc = prvGlyphPtr->shadowTexCord;
			const float stx0 = stc.x0() * texScaleX;
			const float sty0 = stc.y0() * texScaleY;
			const float stx1 = stc.x1() * texScaleX;
			const float sty1 = stc.y1() * texScaleY;

			float shiftX = 0.0f;
			float shiftY = 0.0f;
			if constexpr (shiftXC > 0 || shiftYC > 0) {
				shiftX = scaleX * static_cast<float>(shiftXC) / 100.0f;
				shiftY = scaleY * static_cast<float>(shiftYC) / 100.0f;
			}

			float ssX = 0.0f;
			float ssY = 0.0f;
			if constexpr (outline) {
				ssX = (scaleX / fontSize) * GetOutlineWidth();
				ssY = (scaleY / fontSize) * GetOutlineWidth();
			}

			fontRenderer->AddQuadTrianglesOB(
				{ {dx0 + shiftX - ssX, dy0 - shiftY + ssY, textDepth.y},  stx0, sty0,  (&outlineColor.x) },
				{ {dx1 + shiftX + ssX, dy0 - shiftY + ssY, textDepth.y},  stx1, sty0,  (&outlineColor.x) },
				{ {dx1 + shiftX + ssX, dy1 - shiftY - ssY, textDepth.y},  stx1, sty1,  (&outlineColor.x) },
				{ {dx0 + shiftX - ssX, dy1 - shiftY - ssY, textDepth.y},  stx0, sty1,  (&outlineColor.x) }
			);
		}

		fontRenderer->AddQuadTrianglesPB(
			{ {dx0, dy0, textDepth.x},  tx0, ty0,  (&newColor.x) },
			{ {dx1, dy0, textDepth.x},  tx1, ty0,  (&newColor.x) },
			{ {dx1, dy1, textDepth.x},  tx1, ty1,  (&newColor.x) },
			{ {dx0, dy1, textDepth.x},  tx0, ty1,  (&newColor.x) }
		);
	}
}

void CglFont::glWorldPrint(const float3& p, const float size, const std::string& str, int options)
{
	const bool buffered = (options & FONT_BUFFERED) == FONT_BUFFERED;
	if (!buffered) {
		glPushMatrix();

		CMatrix44f tbM = camera->GetBillBoardMatrix();
		//tbM.SetPos(p); // (Tr * Bm)
		glMultMatrixf(tbM);

		const float3 pos = tbM.Transpose() * p;

		Begin();
		SetTextDepth(pos.z); SetOutlineDepth(pos.z);
		glPrint(pos.x, pos.y, size, options, str);
		SetTextDepth(     ); SetOutlineDepth(     );
		End();

		glPopMatrix();
	}
	else {
		CMatrix44f bm = camera->GetBillBoardMatrix();
		const float3 drawPos = bm.Transpose() * p;

		// drawPos negates the effect of multiplication by camera->GetBillBoardMatrix() in DrawWorldBuffered

		SetTextDepth(drawPos.z); SetOutlineDepth(drawPos.z);
		glPrint(drawPos.x, drawPos.y, size, options | FONT_BUFFERED, str);
		SetTextDepth(         ); SetOutlineDepth(         );
	}
}

CMatrix44f CglFont::DefViewMatrix() { return CMatrix44f::Identity(); }
CMatrix44f CglFont::DefProjMatrix() { return CMatrix44f::ClipOrthoProj01(); }



void CglFont::glPrint(float x, float y, float s, const int options, const std::string& text)
{
	// s := scale or absolute size?
	if (options & FONT_SCALE)
		s *= fontSize;

	float sizeX = s;
	float sizeY = s;
	float textDescender = 0.0f;

	// render in normalized coords (0..1) instead of screencoords (0..~1024)
	if (options & FONT_NORM) {
		sizeX *= globalRendering->pixelX;
		sizeY *= globalRendering->pixelY;
	}

	// horizontal alignment (FONT_LEFT is default)
	if (options & FONT_CENTER) {
		x -= (sizeX * 0.5f * GetTextWidth(text));
	} else if (options & FONT_RIGHT) {
		x -= (sizeX * GetTextWidth(text));
	}


	// vertical alignment
	y += (sizeY * GetDescender()); // move to baseline (note: descender is negative)

	if (options & FONT_BASELINE) {
		// nothing
	} else if (options & FONT_DESCENDER) {
		y -= (sizeY * GetDescender());
	} else if (options & FONT_VCENTER) {
		y -= (sizeY * 0.5f * GetTextHeight(text, &textDescender));
		y -= (sizeY * 0.5f * textDescender);
	} else if (options & FONT_TOP) {
		y -= sizeY * GetTextHeight(text);
	} else if (options & FONT_ASCENDER) {
		y -= (sizeY * (GetDescender() + 1.0f));
	} else if (options & FONT_BOTTOM) {
		GetTextHeight(text, &textDescender);
		y -= (sizeY * textDescender);
	}

	if (options & FONT_NEAREST) {
		x = (int)x;
		y = (int)y;
	}

	// backup text & outline colors, ::ColorResetIndicator will reset them
	baseTextColor = textColor;
	baseOutlineColor = outlineColor;

	const ColorCodeCallBack cccb = [this](float4 newColor) {
		if (autoOutlineColor) {
			SetColors(&newColor, nullptr);
		} else {
			SetTextColor(&newColor);
		}
	};

	const bool buffered = ((options & FONT_BUFFERED) != 0);
	const bool immediate = (!inBeginEndBlock && !buffered);

	if (immediate) {
		// no buffering
		Begin();
	} else if (buffered) {
		if (!inBeginEndBlock)
			GetFontMutex()->lock();
	}

	// select correct decoration RenderString function
	if ((options & FONT_OUTLINE) != 0) {
		RenderStringOutlined(x, y, sizeX, sizeY, text, cccb);
	} else if ((options & FONT_SHADOW) != 0) {
		RenderStringShadow(x, y, sizeX, sizeY, text, cccb);
	} else {
		RenderString(x, y, sizeX, sizeY, text, cccb);
	}

	if (immediate) {
		End();
	} else if (buffered) {
		if (!inBeginEndBlock)
			GetFontMutex()->unlock();
	}

	// reset text & outline colors (if changed via in-text colorcodes)
	SetColors(&baseTextColor, &baseOutlineColor);
}

// TODO: remove, only used by PlayerRosterDrawer
void CglFont::glPrintTable(float x, float y, float s, const int options, const std::string& text)
{
	std::vector<std::string> colLines;
	std::vector<float> colWidths;
	std::vector<SColor> colColor;

	SColor defColor(int(ColorCodeIndicator), 0, 0);
	SColor curColor(int(ColorCodeIndicator), 0, 0);

	for (int i = 0; i < 3; ++i) {
		defColor[i + 1] = uint8_t(textColor[i] * 255.0f);
		curColor[i + 1] = defColor[i + 1];
	}

	colLines.emplace_back("");
	colColor.push_back(defColor);

	int col = 0;
	int row = 0;

	for (int pos = 0; pos < text.length(); pos++) {
		const unsigned char& c = text[pos];

		switch (c) {
			// inline colorcodes
			case ColorCodeIndicator: {
				for (int i = 0; i < 4 && pos < text.length(); ++i, ++pos) {
					colLines[col] += text[pos];
					curColor[i] = text[pos];
				}
				colColor[col] = curColor;
				pos -= 1;
			} break;

			// column separator is horizontal tab
			case '\t': {
				if ((col += 1) >= colLines.size()) {
					colLines.emplace_back("");
					for (int i = 0; i < row; ++i)
						colLines[col] += 0x0A;
					colColor.push_back(defColor);
				}
				if (colColor[col] != curColor) {
					for (int i = 0; i < 4; ++i)
						colLines[col] += curColor[i];
					colColor[col] = curColor;
				}
			} break;

			case 0x0D: {
				pos += ((pos + 1) < text.length() && text[pos + 1] == 0x0A);
				[[fallthrough]]; // CR; fall-through
			}
			case 0x0A: {
				// LF
				for (auto& colLine: colLines)
					colLine += 0x0A;

				if (colColor[0] != curColor) {
					for (int i = 0; i < 4; ++i)
						colLines[0] += curColor[i];
					colColor[0] = curColor;
				}

				col = 0;
				row += 1;
			} break;

			// printable char or orphaned (c >= 127 && c <= 255) color-code
			default: {
				colLines[col] += c;
			} break;
		}
	}

	float totalWidth = 0.0f;
	float maxHeight = 0.0f;
	float minDescender = 0.0f;

	colWidths.resize(colLines.size(), 0.0f);

	for (size_t i = 0; i < colLines.size(); ++i) {
		colWidths[i] = GetTextWidth(colLines[i]);
		totalWidth += colWidths[i];

		float textDescender;
		float textHeight = GetTextHeight(colLines[i], &textDescender);

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
		x -= (sizeX * 0.5f * totalWidth);
	} else if (options & FONT_RIGHT) {
		x -= (sizeX * totalWidth);
	}

	// vertical alignment
	if (options & FONT_BASELINE) {
		// nothing
	} else if (options & FONT_DESCENDER) {
		y -= (sizeY * GetDescender());
	} else if (options & FONT_VCENTER) {
		y -= (sizeY * 0.5f * maxHeight);
		y -= (sizeY * 0.5f * minDescender);
	} else if (options & FONT_TOP) {
		y -= (sizeY * maxHeight);
	} else if (options & FONT_ASCENDER) {
		y -= (sizeY * (GetDescender() + 1.0f));
	} else if (options & FONT_BOTTOM) {
		y -= (sizeY * minDescender);
	}

	for (size_t i = 0; i < colLines.size(); ++i) {
		glPrint(x, y, s, (options | FONT_BASELINE) & ~(FONT_RIGHT | FONT_CENTER), colLines[i]);
		x += (sizeX * colWidths[i]);
	}
}

void CglFont::GetStats(std::array<size_t, 8>& stats) const
{
	fontRenderer->GetStats(stats);
}

#endif

