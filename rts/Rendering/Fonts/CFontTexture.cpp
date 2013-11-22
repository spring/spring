/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CFontTexture.h"
#include "Rendering/GlobalRendering.h"

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
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Util.h"
#include "System/Exceptions.h"
#include "System/float4.h"
#include "System/bitops.h"

#include "LanguageBlocksDefs.h"

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


CFontTexture::CFontTexture(const std::string& fontfile, int size, int _outlinesize, float  _outlineweight):
#ifndef HEADLESS
	library(libraryHandler.GetLibrary()),
#else
	library(NULL),
#endif
	face(NULL),
	faceDataBuffer(NULL),
	outlineSize(_outlinesize),
	outlineWeight(_outlineweight),
	lineHeight(0),
	fontDescender(0),
	texWidth(0),
	texHeight(0),
	texture(0),
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

	//! Create initial small texture
	CreateTexture(32,32); //FIXME
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
#endif
}

CFontTexture::~CFontTexture()
{
#ifndef   HEADLESS
	for(auto it=blocks.begin();it!=blocks.end();++it)
		delete it->second;

	FT_Done_Face(face);
	delete[] faceDataBuffer;
	glDeleteTextures(1, (const GLuint*)&texture);
#endif
}

int CFontTexture::GetTextureWidth() const
{
	return texWidth;
}

int CFontTexture::GetTextureHeight() const
{
	return texHeight;
}

int CFontTexture::GetOutlineWidth() const
{
	return outlineSize;
}

float CFontTexture::GetOutlineWeight() const
{
	return outlineWeight;
}

float CFontTexture::GetLineHeight() const
{
	return lineHeight;
}

float CFontTexture::GetDescender() const
{
	return fontDescender;
}

const FT_Face& CFontTexture::GetFace() const
{
	return face;
}

int CFontTexture::GetTexture() const
{
	return texture;
}

const GlyphInfo& CFontTexture::GetGlyph(char32_t ch)
{
#ifndef HEADLESS
	char32_t end;
	//Get block start pos
	char32_t start=GetLanguageBlock(ch,end);
	auto it = blocks.find(start);
	if (it ==  blocks.end())
	{
		//Load an entire block
		return LoadBlock(start,end)->Get(ch); //FIXME can throw texture_size_exception
	}
	return it->second->Get(ch);
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


LanguageBlock* CFontTexture::LoadBlock(char32_t start,char32_t end)
{
#ifndef   HEADLESS
	LanguageBlock* block=blocks.insert(std::pair<char32_t,LanguageBlock*>(start,new LanguageBlock(start,end))).first->second;
	for(char32_t i=start;i<end;i++)
		LoadGlyph(block,i);

	return block;
#endif
}

void CFontTexture::LoadGlyph(LanguageBlock* block,char32_t ch)
{
#ifndef   HEADLESS
	FT_Error error;
	auto& glyph = block->Get(ch);
	glyph = GlyphInfo();// Make empty glyph in case of error
	FT_UInt index = FT_Get_Char_Index(face, ch);
	glyph.index = index;
	glyph.utf16 = ch;

	error = FT_Load_Glyph(face, index, FT_LOAD_RENDER|FT_LOAD_FORCE_AUTOHINT);
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

	      unsigned char* dst_pixels = new unsigned char[width * height]; // alpha-only 8bit texture
	const unsigned char* src_pixels = slot->bitmap.buffer;

	// Pixels are 8 bits gray levels
	for(int y = 0; y < height; y++) {
		const unsigned char* src = src_pixels + y * slot->bitmap.pitch;
		      unsigned char* dst = dst_pixels + y * width;
		memcpy(dst, src, width);
	}

	glyph.texCord = AllocateGlyphRect(width, height);
	Update(dst_pixels, glyph.texCord.x, glyph.texCord.y, width, height);

	delete[] dst_pixels;
#endif
}

CFontTexture::Row* CFontTexture::FindRow(int glyphWidth, int glyphHeight)
{
	for(auto& row: imageRows) {
		float ratio = (float)row.height/(float)glyphHeight;
		// Ignore too small or too big raws
		if (ratio < 1.0f || ratio > 1.3f)
			continue;

		// Check if there is enough space in this row
		if (texWidth - row.width < glyphWidth)
			continue;

		return &row;
	}
	return 0;
}

CFontTexture::Row* CFontTexture::AddRow(int glyphWidth, int glyphHeight)
{
	int rowHeight = glyphHeight + (2*glyphHeight)/10;
	while (nextRowPos+rowHeight >= texHeight) {
		// Resize texture
		ResizeTexture(texWidth*2,texHeight*2); //FIXME
	}
	Row newrow(nextRowPos, rowHeight);
	nextRowPos += rowHeight;
	imageRows.push_back(newrow);
	return &imageRows.back();
}

IGlyphRect CFontTexture::AllocateGlyphRect(int glyphWidth,int glyphHeight)
{
#ifndef   HEADLESS
	//FIXME add padding
	Row* row = FindRow(glyphWidth, glyphHeight);
	if (!row)
		row = AddRow(glyphWidth, glyphHeight);

	IGlyphRect rect = IGlyphRect(row->width, row->position, glyphWidth, glyphHeight);
	row->width += glyphWidth;
	return rect;
#else
	return IGlyphRect();
#endif
}

void CFontTexture::CreateTexture(int w,int h)
{
#ifndef   HEADLESS
	if(w>2048 || h>2048)
		throw texture_size_exception();

	if(!texture)
		glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	if (GLEW_ARB_texture_border_clamp) {
		static const GLfloat borderColor[4] = {1.0f,1.0f,1.0f,0.0f};
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);

	texWidth=w;
	texHeight=h;
#endif
}

void CFontTexture::ResizeTexture(int w,int h)
{
#ifndef   HEADLESS
	glPushAttrib(GL_PIXEL_MODE_BIT);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	unsigned char* pixels = new unsigned char[texWidth * texHeight];
	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);

	int oldsw=texWidth;
	int oldsh=texHeight;
	CreateTexture(w,h);

	Update(pixels,0,0,oldsw,oldsh);
	delete[] pixels;
#endif
}

void CFontTexture::Update(const unsigned char* pixels,int x,int y,int w,int h)
{
#ifndef HEADLESS
	//FIXME readd shadow blur
	glPushAttrib(GL_PIXEL_MODE_BIT);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, (unsigned int)w, (unsigned int)h, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
	glPopAttrib();
#endif
}

void CFontTexture::Clear(int x,int y,int w,int h)
{
	unsigned char* wipe_buf=new unsigned char[w*h];
	memset(wipe_buf,0,w*h);
	Update(wipe_buf,x,y,w,h);
	delete[] wipe_buf;
}
