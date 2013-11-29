/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CFontTexture.h"
#include "Rendering/GlobalRendering.h"

#include <string>
#include <cstring> // for memset, memcpy
#include <stdio.h>
#include <stdarg.h>
#include <stdexcept>
#include <unordered_set>
#ifndef   HEADLESS
#include <ft2build.h>
#include FT_FREETYPE_H
#endif // HEADLESS

#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Util.h"
#include "System/Exceptions.h"
#include "System/float4.h"
#include "System/bitops.h"

#include "LanguageBlocksDefs.h"

static const int ATLAS_PADDING = 2;


#ifndef   HEADLESS
#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, 0 } };
struct ErrorString {
	int          err_code;
	const char*  err_msg;
} static errorTable[] =
#include FT_ERRORS_H


static const char* GetFTError(FT_Error e) {
	for (int a = 0; errorTable[a].err_msg; ++a) {
		if (errorTable[a].err_code == e)
			return errorTable[a].err_msg;
	}
	return "Unknown error";
}
#endif // HEADLESS


class texture_size_exception : public std::exception
{
};


static inline uint32_t GetKerningHash(char32_t lchar, char32_t rchar)
{
	if (lchar < 128 && rchar < 128) {
		return (lchar << 7) | rchar; // 14bit used
	}
	return (lchar << 16) | rchar; // 32bit used
}

#ifndef   HEADLESS
class FtLibraryHandler
{
public:
	FtLibraryHandler() {
		FT_Error error = FT_Init_FreeType(&lib);
		if (error) {
			std::string msg = "FT_Init_FreeType failed:";
			msg += GetFTError(error);
			throw std::runtime_error(msg);
		}
	};

	~FtLibraryHandler() {
		FT_Done_FreeType(lib);
	};

	FT_Library& GetLibrary() {
		return lib;
	};

private:
	FT_Library lib;
};

FtLibraryHandler libraryHandler; ///it will be automaticly createated and deleted
#endif

static std::unordered_set<CFontTexture*> allFonts;


CFontTexture::CFontTexture(const std::string& fontfile, int size, int _outlinesize, float  _outlineweight):
	outlineSize(_outlinesize),
	outlineWeight(_outlineweight),
	lineHeight(0),
	fontDescender(0),
	texWidth(0),
	texHeight(0),
	wantedTexWidth(0),
	wantedTexHeight(0),
	texture(0),
	textureSpaceMatrix(0),
	atlasUpdate(NULL),
#ifndef HEADLESS
	library(libraryHandler.GetLibrary()),
#else
	library(NULL),
#endif
	face(NULL),
	faceDataBuffer(NULL),
	nextRowPos(0)
{
	if (size <= 0)
		size = 14;

	static const int FT_INTERNAL_DPI = 64;
	normScale = 1.0f / (size * FT_INTERNAL_DPI);

#ifndef   HEADLESS
	std::string fontPath(fontfile);
	CFileHandler* f = new CFileHandler(fontPath);
	if (!f->FileExists()) {
		// check in 'fonts/', too
		if (fontPath.substr(0,6) != "fonts/") {
			delete f;
			fontPath = "fonts/" + fontPath;
			f = new CFileHandler(fontPath);
		}

		if (!f->FileExists()) {
			delete f;
			throw content_error("Couldn't find font '" + fontPath + "'.");
		}
	}

	int filesize = f->FileSize();
	faceDataBuffer = new FT_Byte[filesize];
	f->Read(faceDataBuffer,filesize);
	delete f;

	FT_Error error;
	error = FT_New_Memory_Face(library, faceDataBuffer, filesize, 0, &face);

	if (error) {
		delete[] faceDataBuffer;
		std::string msg = fontfile + ": FT_New_Face failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
	}

	// set render size
	error = FT_Set_Pixel_Sizes(face, 0, size);
	if (error) {
		FT_Done_Face(face);
		delete[] faceDataBuffer;
		std::string msg = fontfile + ": FT_Set_Pixel_Sizes failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
	}

	// select unicode charmap
	error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
	if (error) {
		FT_Done_Face(face);
		delete[] faceDataBuffer;
		std::string msg = fontfile + ": FT_Select_Charmap failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
	}

	fontDescender = normScale * FT_MulFix(face->descender, face->size->metrics.y_scale);
	//lineHeight = FT_MulFix(face->height, face->size->metrics.y_scale); // bad results
	lineHeight = face->height / face->units_per_EM;
	if (lineHeight <= 0)
		lineHeight = 1.25 * (face->bbox.yMax - face->bbox.yMin);

	// has to be done before first GetGlyph() call!
	CreateTexture(128, 128);

	// precache ASCII kernings
	memset(kerningPrecached, 0, 128*128*sizeof(float));
	for (char32_t i=32; i<128; ++i) {
		const auto& lgl = GetGlyph(i);
		const float advance = lgl.advance;
		for (char32_t j=32; j<128; ++j) {
			const auto& rgl = GetGlyph(j);
			const auto hash = GetKerningHash(i, j);
			FT_Vector kerning;
			FT_Get_Kerning(face, lgl.index, rgl.index, FT_KERNING_DEFAULT, &kerning);
			kerningPrecached[hash] = advance + normScale * kerning.x;
		}
	}

	allFonts.insert(this);
#endif
}

CFontTexture::~CFontTexture()
{
#ifndef   HEADLESS
	allFonts.erase(this);
	FT_Done_Face(face);
	delete[] faceDataBuffer;
	glDeleteTextures(1, (const GLuint*)&texture);
	glDeleteLists(textureSpaceMatrix, 1);
#endif
}


void CFontTexture::Update() {
	for (auto& font: allFonts) {
		if (font->texWidth != font->wantedTexWidth || font->texHeight != font->wantedTexHeight) {
			font->ResizeTexture();
		}
	}
}


const GlyphInfo& CFontTexture::GetGlyph(char32_t ch)
{
#ifndef HEADLESS
	const auto it = glyphs.find(ch);
	if (it != glyphs.end())
		return it->second;

	// Get block start pos
	char32_t start, end;
	start = GetLanguageBlock(ch, end);

	// Load an entire block
	LoadBlock(start, end);
	return GetGlyph(ch);
#else
	static GlyphInfo g = GlyphInfo();
	return g;
#endif
}


float CFontTexture::GetKerning(const GlyphInfo& lgl, const GlyphInfo& rgl)
{
#ifndef HEADLESS
	// first check caches
	uint32_t hash = GetKerningHash(lgl.utf16, rgl.utf16);
	if (hash < 128*128) {
		return kerningPrecached[hash];
	}
	const auto it = kerningDynamic.find(hash);
	if (it != kerningDynamic.end()) {
		return it->second;
	}

	// load & cache
	FT_Vector kerning;
	FT_Get_Kerning(face, lgl.index, rgl.index, FT_KERNING_DEFAULT, &kerning);
	kerningDynamic[hash] = lgl.advance + normScale * kerning.x;
	return kerningDynamic[hash];
#else
	return 0;
#endif
}


void CFontTexture::LoadBlock(char32_t start, char32_t end)
{
	for(char32_t i=start; i<end; ++i)
		LoadGlyph(i);

	GLboolean inListCompile;
	glGetBooleanv(GL_LIST_INDEX, &inListCompile);
	if (!inListCompile) {
		ResizeTexture();
	}
}

void CFontTexture::LoadGlyph(char32_t ch)
{
#ifndef HEADLESS
	if (glyphs.find(ch) != glyphs.end())
		return;

	// map unicode to font internal index
	FT_UInt index = FT_Get_Char_Index(face, ch);

	// check for duplicated glyphs
	for (auto& it: glyphs) {
		if (it.second.index == index) {
			auto& glyph = glyphs[ch];
			glyph = it.second;
			glyph.utf16 = ch;
			return;
		}
	}

	auto& glyph = glyphs[ch];
	glyph.index = index;
	glyph.utf16 = ch;

	// load glyph
	FT_Error error = FT_Load_Glyph(face, index, FT_LOAD_RENDER|FT_LOAD_FORCE_AUTOHINT);
	if (error) {
		LOG_L(L_ERROR, "Couldn't load glyph %d", ch);
	}

	FT_GlyphSlot slot = face->glyph;

	const float xbearing = slot->metrics.horiBearingX * normScale;
	const float ybearing = slot->metrics.horiBearingY * normScale;

	glyph.size.x = xbearing;
	glyph.size.y = ybearing - fontDescender;
	glyph.size.w =  slot->metrics.width * normScale;
	glyph.size.h = -slot->metrics.height * normScale;

	glyph.advance   = slot->advance.x * normScale;
	glyph.height    = slot->metrics.height * normScale;
	glyph.descender = ybearing - glyph.height;

	const int width  = slot->bitmap.width;
	const int height = slot->bitmap.rows;

	if (width<=0 || height<=0) {
		return;
	}

	if (slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
		LOG_L(L_ERROR, "invalid pixeldata mode");
		return;
	}

	try {
		glyph.texCord = AllocateGlyphRect(width, height);
		glyph.shadowTexCord = AllocateGlyphRect(width + 2 * outlineSize, height + 2 * outlineSize);
	} catch(const texture_size_exception& e) {
		LOG_L(L_WARNING, "Texture limit reached! (try to reduce the font size/outline-width)");
	}

	if ((atlasUpdate->xsize != wantedTexWidth) || (atlasUpdate->ysize != wantedTexHeight)) {
		(*atlasUpdate) = atlasUpdate->CanvasResize(wantedTexWidth, wantedTexHeight, false);
	}

	assert(slot->bitmap.pitch == 1);
	CBitmap gbm(slot->bitmap.buffer, width, height, 1);
	if (glyph.texCord.w != 0)
		atlasUpdate->CopySubImage(gbm, int(glyph.texCord.x), int(glyph.texCord.y));
	gbm = gbm.CanvasResize(width + 2 * outlineSize, height + 2 * outlineSize, true);
	gbm.Blur(outlineSize, outlineWeight);
	if (glyph.shadowTexCord.w != 0)
		atlasUpdate->CopySubImage(gbm, int(glyph.shadowTexCord.x), int(glyph.shadowTexCord.y));
#endif
}

CFontTexture::Row* CFontTexture::FindRow(int glyphWidth, int glyphHeight)
{
	float best_ratio = 10000.0f;
	Row*  best_row   = nullptr;

	// first try to find a row with similar height
	for(auto& row: imageRows) {
		// Check if there is enough space in this row
		if (wantedTexWidth - row.width < glyphWidth)
			continue;

		// cache suboptimum solution
		const float ratio = float(row.height)/glyphHeight;
		if (
			   (best_ratio > ratio)
			&& (ratio >= 1.0f)
			&& (float(row.width) / wantedTexWidth < 0.85f)
		) {
			best_ratio = ratio;
			best_row   = &row;
		}

		// Ignore too small or too big rows
		if (ratio < 1.0f || ratio > 1.3f)
			continue;

		// found a good enough row (not optimal solution for that we would need to check all lines -> slower)
		return &row;
	}

	// none found, take any row that isn't full yet and is high enough to fit the glyph
	if (best_row)
		return best_row;

	// try to resize last row when possible
	if (!imageRows.empty()) {
		const int freeHeightSpace = wantedTexHeight - nextRowPos;
		if (freeHeightSpace > 0) {
			Row& lastRow = imageRows.back();
			const int moreHeightNeeded = glyphHeight - lastRow.height;
			if (moreHeightNeeded > 0 && moreHeightNeeded < 4) {
				if ((freeHeightSpace > moreHeightNeeded) && (wantedTexWidth - lastRow.width > glyphWidth)) {
					lastRow.height += moreHeightNeeded;
					nextRowPos     += moreHeightNeeded;
					return &lastRow;
				}
			}
		}
	}

	// still no row found create a new one
	return AddRow(glyphWidth, glyphHeight);
}

CFontTexture::Row* CFontTexture::AddRow(int glyphWidth, int glyphHeight)
{
	const int wantedRowHeight = glyphHeight * 1.2f;
	while (nextRowPos+wantedRowHeight >= wantedTexHeight) {
		if (wantedTexWidth>=2048 || wantedTexHeight>=2048)
			throw texture_size_exception();

		// Resize texture
		wantedTexWidth  <<= 1;
		wantedTexHeight <<= 1;
	}
	Row newrow(nextRowPos, wantedRowHeight);
	nextRowPos += wantedRowHeight;
	imageRows.push_back(newrow);
	return &imageRows.back();
}

IGlyphRect CFontTexture::AllocateGlyphRect(int glyphWidth,int glyphHeight)
{
#ifndef   HEADLESS
	//FIXME add padding
	Row* row = FindRow(glyphWidth + ATLAS_PADDING, glyphHeight + ATLAS_PADDING);

	IGlyphRect rect = IGlyphRect(row->width, row->position, glyphWidth, glyphHeight);
	row->width += glyphWidth + ATLAS_PADDING;
	return rect;
#else
	return IGlyphRect();
#endif
}


void CFontTexture::CreateTexture(const int width, const int height)
{
#ifndef   HEADLESS
	glPushAttrib(GL_TEXTURE_BIT);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	if (GLEW_ARB_texture_border_clamp) {
		const GLfloat borderColor[4] = { 1.0f,1.0f,1.0f,0.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 1, 1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);

	texWidth  = wantedTexWidth  = width;
	texHeight = wantedTexHeight = height;
	glPopAttrib();

	atlasUpdate = new CBitmap();
	atlasUpdate->channels = 1;
	atlasUpdate->Alloc(width, height);

	textureSpaceMatrix = glGenLists(1);
	glNewList(textureSpaceMatrix, GL_COMPILE);
	glEndList();
#endif
}


void CFontTexture::ResizeTexture()
{
#ifndef   HEADLESS
	glPushAttrib(GL_PIXEL_MODE_BIT | GL_TEXTURE_BIT);

	const int width  = wantedTexWidth;
	const int height = (globalRendering->supportNPOTs) ? nextRowPos : wantedTexHeight;

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, atlasUpdate->mem);

	texWidth  = width;
	texHeight = height;

	glNewList(textureSpaceMatrix, GL_COMPILE);
	glScalef(1.f/width, 1.f/height, 1.f);
	glEndList();

	glPopAttrib();
#endif
}
