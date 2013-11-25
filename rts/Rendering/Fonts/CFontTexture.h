/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CFONTTEXTURE_H
#define _CFONTTEXTURE_H
#include <unordered_map>
#include <list>
#include <string>


struct FT_FaceRec_;
struct FT_LibraryRec_;
typedef struct FT_FaceRec_* FT_Face;
typedef struct FT_LibraryRec_* FT_Library;

class CBitmap;


struct IGlyphRect { //FIXME use SRect
	IGlyphRect():
		x(0),y(0),
		w(0),h(0) {
	};

	IGlyphRect(float _x,float _y,float _w,float _h):
		x(_x),y(_y),
		w(_w),h(_h) {
	};

	float x0() const {
		return x;
	};
	float x1() const {
		return x+w;
	};
	float y0() const {
		return y;
	};
	float y1() const {
		return y+h;
	};

	float x,y;
	float w,h;
};

struct GlyphInfo {
	GlyphInfo()
	: advance(0)
	, height(0)
	, descender(0)
	, index(0)
	, utf16(0)
	{ };

	IGlyphRect size;
	IGlyphRect texCord;
	IGlyphRect shadowTexCord;
	float advance, height, descender;
	char32_t index;
	char32_t utf16;
};



/**
This class just store glyphs and load new glyphs if requred
It works with image and don't care about rendering these glyphs
It works only and only with UTF32 chars
**/
class CFontTexture
{
public:
	static void Update();

public:
	CFontTexture(const std::string& fontfile, int size, int outlinesize, float  outlineweight);
	virtual ~CFontTexture();

public:
	int GetTextureWidth() const { return texWidth; }
	int GetTextureHeight() const { return texHeight; }
	int   GetOutlineWidth() const { return outlineSize; }
	float GetOutlineWeight() const { return outlineWeight; }
	float GetLineHeight() const { return lineHeight; }
	float GetDescender() const { return fontDescender; }

	int GetTexture() const { return texture; }

	const GlyphInfo& GetGlyph(char32_t ch); //< Get or load a glyph

protected:
	const FT_Face& GetFace() const { return face; }

	float GetKerning(const GlyphInfo& lgl,const GlyphInfo& rgl);

private:
	struct Row {
		Row(int _ypos,int _height):
			position(_ypos),
			height(_height),
			width(0) {
		};

		int position;
		int height;
		int width;
	};

private:
	// Create a new texture and copy all data from the old one
	void ResizeTexture();
	void CreateTexture(const int width, const int height);

	// Load all chars in block's range
	void LoadBlock(char32_t start, char32_t end);
	void LoadGlyph(char32_t ch);

	// Throw texture_size_exception if image's width or height is bigger than 2048
	Row* FindRow(int glyphWidth, int glyphHeight);
	Row* AddRow(int glyphWidth, int glyphHeight);
	// Try to find a place where can be placed a glyph with given size
	// If it is imposible, resize the texture and try it again
	IGlyphRect AllocateGlyphRect(int glyphWidth, int glyphHeight);

protected:
	std::unordered_map<char32_t, GlyphInfo> glyphs; // UTF16 -> GlyphInfo

	float kerningPrecached[128 * 128]; // contains ASCII kerning
	std::unordered_map<uint32_t, float> kerningDynamic; // contains unicode kerning

	int outlineSize;
	float outlineWeight;
	float lineHeight;
	float fontDescender;
	float normScale;
	int texWidth, texHeight;
	int wantedTexWidth, wantedTexHeight;
	unsigned int texture;

public:
	unsigned int textureSpaceMatrix;

private:
	CBitmap* atlasUpdate;

	FT_Library library;
	FT_Face face;
	unsigned char* faceDataBuffer;

	std::list<Row> imageRows;
	int nextRowPos;
};

#endif // CFONTTEXTURE_H
