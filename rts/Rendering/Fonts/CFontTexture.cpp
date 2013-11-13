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
  struct ErrorString
  {
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

class texture_size_exception : public std::exception
{
};

#ifndef   HEADLESS
class FtLibraryHandler
{

    FT_Library lib;

public:
    FtLibraryHandler()
    {
        FT_Error error = FT_Init_FreeType(&lib);
        if (error)
        {
            std::string msg = "FT_Init_FreeType failed:";
            msg += GetFTError(error);
            throw std::runtime_error(msg);
        }
    };

    ~FtLibraryHandler()
    {
        FT_Done_FreeType(lib);
    };

    FT_Library GetLibrary()
    {
        return lib;
    };
};

FtLibraryHandler libraryHandler;///it will be automaticly createated and deleted
#endif


CFontTexture::CFontTexture(const std::string& fontfile, int size, int _outlinesize, float  _outlineweight):
    outlineSize(_outlinesize),
    outlineWeight(_outlineweight),
    #ifndef   HEADLESS
    library(libraryHandler.GetLibrary()),
    #else
    library(0),
    #endif
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
    if (!f->FileExists())
    {
		//! check in 'fonts/', too
		if (fontPath.substr(0,6) != "fonts/")
		{
			delete f;
			fontPath = "fonts/" + fontPath;
			f = new CFileHandler(fontPath);
		}

		if (!f->FileExists())
		{
			delete f;
			throw content_error("Couldn't find font '" + fontPath + "'.");
		}
	}

	int filesize = f->FileSize();
	faceDataBuffer = new FT_Byte[filesize];
	f->Read(faceDataBuffer,filesize);
	delete f;

	FT_Error error;
	FT_Face _face;
	error = FT_New_Memory_Face((FT_Library)library, faceDataBuffer, filesize, 0, &_face);
	face=(void*)_face;

	if (error)
	{
		delete[] faceDataBuffer;
		std::string msg = fontfile + ": FT_New_Face failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
	}

    //! set render size
	error = FT_Set_Pixel_Sizes(_face, 0, size);
	if (error)
	{
		FT_Done_Face(_face);
		delete[] faceDataBuffer;
		std::string msg = fontfile + ": FT_Set_Pixel_Sizes failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
	}

	//! select unicode charmap
    error = FT_Select_Charmap(_face, FT_ENCODING_UNICODE);
	if(error) //Support unicode or GTFO
    {
		FT_Done_Face(_face);
		delete[] faceDataBuffer;
		std::string msg = fontfile + ": FT_Select_Charmap failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
    }
    fontDescender = FT_MulFix(_face->descender, _face->size->metrics.y_scale);
    lineHeight = _face->size->metrics.height; //! Originaly it was _face->size->metrics.height >> 6

    //! Create initial small texture
    CreateTexture(32,32);
    #endif
}

CFontTexture::~CFontTexture()
{
    #ifndef   HEADLESS
    FT_Face _face=(FT_Face)face;
    FT_Done_Face(_face);
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

int CFontTexture::GetOutlineSize() const
{
    return outlineSize;
}

float CFontTexture::GetOutilneWeight() const
{
    return outlineWeight;
}

int CFontTexture::GetLineHeight() const
{
    return lineHeight;
}

int CFontTexture::GetFontDescender() const
{
    return fontDescender;
}

void* CFontTexture::GetLibrary() const
{
    return (void*)library;
}

void* CFontTexture::GetFace() const
{
    return (void*)face;
}

int CFontTexture::GetTexture() const
{
    return texture;
}

const CFontTexture::GlyphInfo& CFontTexture::GetGlyph(unsigned int ch)
{
    #ifndef   HEADLESS
    std::map<unsigned int,GlyphInfo>::iterator it;
    it=glyphs.find(ch);
    if(it==glyphs.end())
        return LoadGlyph(ch);
    return it->second;
    #else
    return GlyphInfo();
    #endif
}

int CFontTexture::GetKerning(unsigned int lchar,unsigned int rchar)
{
    #ifndef   HEADLESS
    FT_Face _face=(FT_FaceRec_*)face;
    const GlyphInfo& left=GetGlyph(lchar);
    if(FT_HAS_KERNING(_face))
    {
        //!get or load requred glyphs
        const GlyphInfo& right=GetGlyph(rchar);

        FT_Vector kerning;
        FT_Get_Kerning(_face, left.index, right.index, FT_KERNING_DEFAULT, &kerning);

        return left.advance + kerning.x;
    }
    else
        return left.advance;//!This font does not contain kerning
    #else
    return 0;
    #endif
}

int CFontTexture::GetKerning(const GlyphInfo& lgl,const GlyphInfo& rgl)
{
    #ifndef   HEADLESS
    FT_Face _face=(FT_FaceRec_*)face;
    if(FT_HAS_KERNING(_face))
    {
        FT_Vector kerning;
        FT_Get_Kerning(_face, lgl.index, rgl.index, FT_KERNING_DEFAULT, &kerning);
        return lgl.advance + kerning.x;
    }
    else
        return lgl.advance;

    #else
    return 0;
    #endif
}

CFontTexture::GlyphInfo& CFontTexture::LoadGlyph(unsigned int ch)
{
    #ifndef   HEADLESS
    //! This code mainly base on SFML code
    //! I actually do not have a clear idea how it works
    // I just pray
  //  LOG("Try to get glyph: %i", ch);

    FT_Error error;
    FT_Face _face=(FT_FaceRec_*)face;
    CFontTexture::GlyphInfo& glyph=glyphs[ch];

    FT_UInt index = FT_Get_Char_Index(_face, ch);
    glyph.index=index;

    error = FT_Load_Glyph(_face,index,FT_LOAD_RENDER);
    if(error)
    {
        return glyph;
    }

    FT_GlyphSlot slot = _face->glyph;

    const float xbearing = slot->metrics.horiBearingX;
    const float ybearing = slot->metrics.horiBearingY;

    glyph.advance = slot->advance.x;
    glyph.height = slot->metrics.height;
    glyph.descender = ybearing - glyph.height;

    //! Get glyph bound box
    glyph.size = IGlyphRect(xbearing,                           // x0
                            ybearing,                           // y0
                            slot->metrics.width,                // w
                            slot->metrics.height);              // h


    int width  = slot->bitmap.width;
    int height = slot->bitmap.rows;
    static const int padding=1;
    if(width>0 && height>0)
    {
        unsigned char* pixels_buffer=new unsigned char[width * height];
        glyph.texCord=AllocateGlyphRect(width+2*padding,height+2*padding);
        const unsigned char* normal_pixels = slot->bitmap.buffer;

      /*  if(slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
        {
            // Pixels are 1 bit monochrome values
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    // The color channels remain white, just fill the alpha channel
                    int index = x + y * width;
                    pixels_buffer[index] = ((normal_pixels[x / 8]) & (1 << (7 - (x % 8)))) ? 255 : 0;
                }
                normal_pixels += slot->bitmap.pitch;
            }
        }
        else
        {*/
            // Pixels are 8 bits gray levels
            for(int y = 0; y < height; y++)
            {
                //Flip y cords
                const unsigned char* src = normal_pixels + (height - 1 - y) * slot->bitmap.pitch;
                //const unsigned char* src = normal_pixels + y * slot->bitmap.pitch;
                unsigned char* dst = pixels_buffer + y * width;
                memcpy(dst,src,width);
            }
      //  }

        Update(pixels_buffer, glyph.texCord.x+padding, glyph.texCord.y+padding,
               width, height);

        glyph.shadowTexCord=glyph.texCord;
        /*int shadowW=width+2*outlineSize;
        int shadowH=height+2*outlineSize;
        glyph.shadowTexCord=AllocateGlyphRect(shadowW+padding*2,shadowH+padding*2);

        CBitmap bmp;
        bmp.channels=1;
        bmp.mem=(unsigned char*)pixels_buffer;
        bmp.xsize=width;
        bmp.ysize=height;

        CBitmap blurbmp;
        blurbmp.channels = 1;
        blurbmp.Alloc(shadowW,shadowH);
        blurbmp.CopySubImage(bmp,outlineSize,outlineSize);
        blurbmp.Blur(outlineSize,outlineWeight);
        bmp.mem=0;

        Update((const unsigned char*)blurbmp.mem,glyph.shadowTexCord.x+padding, glyph.shadowTexCord.y+padding,
               glyph.shadowTexCord.w-padding*2, glyph.shadowTexCord.h-padding*2);
        */
        delete[] pixels_buffer;
    }

    LOG("Glyph is loaded: %i", ch);
    return glyph;
    #else
    return glyphs[0];
    #endif
}

CFontTexture::Row* CFontTexture::FindRow(unsigned int glyphWidth,unsigned int glyphHeight)
{
    std::list<Row>::iterator it;
    for(it=imageRows.begin();it!=imageRows.end();it++)
    {
        float ratio=(float)it->height/(float)glyphHeight;
        //! Ignore too small or too big raws
        if(ratio < 1.0f || ratio > 1.3f)
            continue;

        //! Check if there is enought space in this row
        if(texWidth - it->wight < glyphWidth)
            continue;

        return &(*it);
    }
    return 0;
}

CFontTexture::Row* CFontTexture::AddRow(unsigned int glyphWidth,unsigned int glyphHeight)
{
    int rowHeight = glyphHeight + (2*glyphHeight)/10;
    LOG("Try to add row %i", rowHeight);
    while(nextRowPos+rowHeight>=texHeight)
    {
        LOG("Row pos %i tex height %i", nextRowPos+rowHeight,texHeight);
        //! Resize texture
        CreateTexture(texWidth*2,texHeight*2); //! Make texture twice bigger
    }
    Row newrow(nextRowPos,rowHeight);
    nextRowPos+=rowHeight;
    imageRows.push_back(newrow);
    return &imageRows.back();
}

CFontTexture::IGlyphRect CFontTexture::AllocateGlyphRect(unsigned int glyphWidth,unsigned int glyphHeight)
{
    #ifndef   HEADLESS
    LOG("Look for place: %i x %i", glyphWidth,glyphHeight);
    Row* row=FindRow(glyphWidth,glyphHeight);
    if(!row)
        row=AddRow(glyphWidth,glyphHeight);

    IGlyphRect rect=IGlyphRect(row->wight,row->position,glyphWidth,glyphHeight);
    row->wight+=glyphWidth;
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

    glEnable(GL_TEXTURE_2D);
    GLuint ntex;
    glGenTextures(1, &ntex);
    glBindTexture(GL_TEXTURE_2D, ntex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    static const GLfloat borderColor[4] = {1.0f,1.0f,1.0f,0.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);


    if(texture)
    {
        unsigned char* pixels=new unsigned char[texWidth * texHeight ];
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
    }
    else
    {
        texture=ntex;
        texWidth=w;
        texHeight=h;
    }

    LOG("Font texture (re)created %i x %i", texWidth,texHeight);
    #endif
}

void CFontTexture::Update(const unsigned char* pixels,int x,int y,int w,int h)
{
    #ifndef   HEADLESS
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
    glPopAttrib();
    #endif
}
