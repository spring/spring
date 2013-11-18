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


	static const char* GetFTError(FT_Error e)
{
	for (int a = 0; errorTable[a].err_msg; ++a) {
		if (errorTable[a].err_code == e)
			return errorTable[a].err_msg;
	}
	return "Unknown error";
}
#endif // HEADLESS



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

	FT_Library* GetLibrary() {
		return &lib;
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
	//Why not exception? It is caller problems if arguments are invalide
	if (size<=0)
		size = 14;
#ifndef   HEADLESS
	std::string fontPath(fontfile);
	CFileHandler* f = new CFileHandler(fontPath);
	if (!f->FileExists()) {
		//! check in 'fonts/', too
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
	error = FT_New_Memory_Face(*library, faceDataBuffer, filesize, 0, face);

	if (error) {
		delete[] faceDataBuffer;
		std::string msg = fontfile + ": FT_New_Face failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
	}

	//! set render size
	error = FT_Set_Pixel_Sizes(*face, 0, size);
	if (error) {
		FT_Done_Face(*face);
		delete[] faceDataBuffer;
		std::string msg = fontfile + ": FT_Set_Pixel_Sizes failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
	}

	//! select unicode charmap
	error = FT_Select_Charmap(*face, FT_ENCODING_UNICODE);
	if(error) { //Support unicode or GTFO
		FT_Done_Face(*face);
		delete[] faceDataBuffer;
		std::string msg = fontfile + ": FT_Select_Charmap failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
	}
	fontDescender = FT_MulFix((*face)->descender, (*face)->size->metrics.y_scale);
	lineHeight = FT_MulFix((*face)->height, (*face)->size->metrics.y_scale);

	if(lineHeight<=0)
		lineHeight = 1.25 * ((*face)->bbox.yMax - (*face)->bbox.yMin);

	//! Create initial small texture
	CreateTexture(32,32); //FIXME
#endif
}

CFontTexture::~CFontTexture()
{
#ifndef   HEADLESS
	FT_Done_Face(*face);
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

int CFontTexture::GetLineHeightA() const
{
	return lineHeight;
}

int CFontTexture::GetFontDescender() const
{
	return fontDescender;
}

const FT_Face& CFontTexture::GetFace() const
{
	return *face;
}

int CFontTexture::GetTexture() const
{
	return texture;
}

const CFontTexture::GlyphInfo& CFontTexture::GetGlyph(char32_t ch)
{
#ifndef HEADLESS
	auto it = glyphs.find(ch);
	if (it == glyphs.end())
		return LoadGlyph(ch); //FIXME can throw texture_size_exception
	return it->second;
#else
	static CFontTexture::GlyphInfo g = GlyphInfo();
	return g;
#endif
}

int CFontTexture::GetKerning(char32_t lchar, char32_t rchar)
{
#ifndef HEADLESS
	//FIXME cache!
	const GlyphInfo& left = GetGlyph(lchar);
	// if(FT_HAS_KERNING(face))
	{
		// get or load required glyphs
		const GlyphInfo& right = GetGlyph(rchar);
		FT_Vector kerning;
		FT_Get_Kerning(*face, left.index, right.index, FT_KERNING_DEFAULT, &kerning);
		return left.advance + kerning.x;
	}
//   else
//       return left.advance; // This font does not contain kerning
#else
	return 0;
#endif
}

int CFontTexture::GetKerning(const GlyphInfo& lgl, const GlyphInfo& rgl)
{
#ifndef HEADLESS
	//FIXME cache!
	// if(FT_HAS_KERNING(face))
	{
		FT_Vector kerning;
		FT_Get_Kerning(*face, lgl.index, rgl.index, FT_KERNING_DEFAULT, &kerning);
		return lgl.advance + kerning.x;
	}
	//  else
	//      return lgl.advance;
#else
	return 0;
#endif
}

CFontTexture::GlyphInfo& CFontTexture::LoadGlyph(char32_t ch)
{
#ifndef   HEADLESS
	// This code mainly base on SFML code
	// I actually do not have a clear idea how it works
	// I just pray

	FT_Error error;
	auto& glyph = glyphs[ch];

	FT_UInt index = FT_Get_Char_Index(*face, ch);
	glyph.index = index;

	error = FT_Load_Glyph(*face, index, FT_LOAD_RENDER|FT_LOAD_FORCE_AUTOHINT);
	if (error) {
		LOG_L(L_ERROR, "Couldn't load glyph %d", ch);
		return glyph;
	}

	FT_GlyphSlot slot = (*face)->glyph;

	const float xbearing = slot->metrics.horiBearingX;
	const float ybearing = slot->metrics.horiBearingY;

	glyph.advance   = slot->advance.x;
	glyph.height    = slot->metrics.height;
	glyph.descender = ybearing - glyph.height;

	//! Get glyph bound box
	glyph.size = IGlyphRect(xbearing,                    // x0
				ybearing - fontDescender,    // y0
				 slot->metrics.width,        // w
				-slot->metrics.height);      // h

	int width  = slot->bitmap.width;
	int height = slot->bitmap.rows;
	static const int padding = 1;
	if (width>0 && height>0) {
		//FIXME
		/*glyph.size.x-=padding;
		glyph.size.y-=padding;
		glyph.size.w+=2*padding;
		glyph.size.h+=2*padding;
		*/
		unsigned char* pixels_buffer = new unsigned char[width * height];
		glyph.texCord = AllocateGlyphRect(width+2*padding,height+2*padding);

		const unsigned char* source_pixels = slot->bitmap.buffer;

		/*  if(slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
		  {
		      // Pixels are 1 bit monochrome values
		      for (int y = 0; y < height; ++y)
		      {
		          for (int x = 0; x < width; ++x)
		          {
		              // The color channels remain white, just fill the alpha channel
		              int index = x + y * width;
		              pixels_buffer[index] = ((source_pixels[x / 8]) & (1 << (7 - (x % 8)))) ? 255 : 0;
		          }
		          source_pixels += slot->bitmap.pitch;
		      }
		  }
		  else
		  {*/
		// Pixels are 8 bits gray levels
		for(int y = 0; y < height; y++) {
			//!Flip y cords
			//const unsigned char* src = source_pixels + (height - 1 - y) * slot->bitmap.pitch;
			const unsigned char* src = source_pixels + y * slot->bitmap.pitch;
			unsigned char* dst = pixels_buffer + y * width;
			memcpy(dst,src,width);
		}
		//  }
		Clear(glyph.texCord.x,glyph.texCord.y,glyph.texCord.w,glyph.texCord.h);
		glyph.texCord.x += padding;
		glyph.texCord.y += padding;
		glyph.texCord.w -= 2 * padding;
		glyph.texCord.h -= 2 * padding;

		Update(pixels_buffer, glyph.texCord.x, glyph.texCord.y,
		       width, height);

		delete[] pixels_buffer;
	} else {
		LOG_L(L_ERROR, "invalid glyph size");
	}
	return glyph;
#else
	return glyphs[0];
#endif
}

CFontTexture::Row* CFontTexture::FindRow(int glyphWidth, int glyphHeight)
{
	for(auto& row: imageRows) {
		float ratio=(float)row.height/(float)glyphHeight;
		//! Ignore too small or too big raws
		if(ratio < 1.0f || ratio > 1.3f)
			continue;

		//! Check if there is enought space in this row
		if(texWidth - row.wight < glyphWidth)
			continue;

		return &row;
	}
	return 0;
}

CFontTexture::Row* CFontTexture::AddRow(int glyphWidth, int glyphHeight)
{
	int rowHeight = glyphHeight + (2*glyphHeight)/10;
	while(nextRowPos+rowHeight>=texHeight) {
		//! Resize texture
		CreateTexture(texWidth*2,texHeight*2); //! Make texture twice bigger //FIXME
	}
	Row newrow(nextRowPos,rowHeight);
	nextRowPos += rowHeight;
	imageRows.push_back(newrow);
	return &imageRows.back();
}

CFontTexture::IGlyphRect CFontTexture::AllocateGlyphRect(int glyphWidth,int glyphHeight)
{
#ifndef   HEADLESS
	Row* row = FindRow(glyphWidth,glyphHeight);
	if(!row)
		row = AddRow(glyphWidth,glyphHeight);

	IGlyphRect rect = IGlyphRect(row->wight,row->position,glyphWidth,glyphHeight);
	row->wight += glyphWidth;
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

	glPushAttrib(GL_PIXEL_MODE_BIT);
	glEnable(GL_TEXTURE_2D);
	GLuint ntex;
	glGenTextures(1, &ntex);
	glBindTexture(GL_TEXTURE_2D, ntex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	static const GLfloat borderColor[4] = {1.0f,1.0f,1.0f,0.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
	glPopAttrib();

	if(texture) {
		unsigned char* pixels = new unsigned char[texWidth * texHeight ];
		glBindTexture(GL_TEXTURE_2D, texture);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
		glDeleteTextures(1, (const GLuint*)&texture);

		texture=ntex;
		int oldsw=texWidth;
		int oldsh=texHeight;
		texWidth=w;
		texHeight=h;

		Update(pixels,0,0,oldsw,oldsh);
		delete[] pixels;
	} else {
		texture=ntex;
		texWidth=w;
		texHeight=h;
	}
#endif
}

void CFontTexture::Update(const unsigned char* pixels,int x,int y,int w,int h)
{
#ifndef HEADLESS
	//FIXME readd shadow blur
	glEnable(GL_TEXTURE_2D);
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
