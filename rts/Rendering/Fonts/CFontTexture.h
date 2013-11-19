#ifndef _CFONTTEXTURE_H
#define _CFONTTEXTURE_H
#include <map>
#include <list>
#include <string>


struct FT_FaceRec_;
struct FT_LibraryRec_;
typedef struct FT_FaceRec_* FT_Face;
typedef struct FT_LibraryRec_* FT_Library;


class texture_size_exception : public std::exception
{
};


/**
This class just store glyphs and load new glyphs if requred
It works with image and don't care about rendering these glyphs
It works only and only with UTF32 chars
**/
class CFontTexture
{
public:
	CFontTexture(const std::string& fontfile, int size, int outlinesize, float  outlineweight);
	virtual ~CFontTexture();
public:
	int GetTextureWidth() const;
	int GetTextureHeight() const;
	int   GetOutlineWidth() const;
	float GetOutlineWeight() const;
	int GetLineHeightA() const;
	int GetFontDescender() const;

	int GetTexture() const;

	struct IGlyphRect { //FIXME use SRect
		IGlyphRect():
			x(0),y(0),
			w(0),h(0) {
		};

		IGlyphRect(int _x,int _y,int _w,int _h):
			x(_x),
			y(_y),
			w(_w),
			h(_h) {
		};

		int x0() const {
			return x;
		};
		int x1() const {
			return x+w;
		};
		int y0() const {
			return y;
		};
		int y1() const {
			return y+h;
		};

		int x,y;
		int w,h;
	};

	struct GlyphInfo {
		GlyphInfo():
			size(),
			texCord(0,0,1,1),
			advance(1),
			height(1),
			descender(0),
			index(0) {
		};

		IGlyphRect size;
		IGlyphRect texCord;
		int advance, height, descender;
		char32_t index;
	};
	// Get or load a glyph
	const CFontTexture::GlyphInfo& GetGlyph(char32_t ch);

protected:
	const FT_Face& GetFace() const;

protected:
	int GetKerning(char32_t lchar, char32_t rchar);
	int GetKerning(const CFontTexture::GlyphInfo& lgl,const CFontTexture::GlyphInfo& rgl);

private:
	int outlineSize;
	float outlineWeight;
	int lineHeight;
	int fontDescender;

private:
	unsigned int texWidth,texHeight;
	unsigned int texture;
	//! Create a new texture and copy all data from the old one
	//! Throw texture_size_exception if image's width or height is bigger than 2048
	void CreateTexture(int w,int h);
	//! Create a new texture
	void ResizeTexture(int w,int h);
	//! Copy glyph pixels on the texture
	void Update(const unsigned char* pixels,int x,int y,int w,int h);

	void Clear(int x,int y,int w,int h);

private:
	FT_Library library;
	FT_Face face;
	unsigned char* faceDataBuffer;

private:
	std::map<char32_t,GlyphInfo> glyphs;    //!UTF32 -> GlyphInfo
	CFontTexture::GlyphInfo& LoadGlyph(char32_t ch);

private:
	struct Row {
		Row(int _ypos,int _height):
			position(_ypos),
			height(_height),
			wight(0) {
		};

		int position;
		int height;
		int wight;
	};
	std::list<Row> imageRows;
	int nextRowPos;

	Row* FindRow(int glyphWidth, int glyphHeight);
	Row* AddRow(int glyphWidth, int glyphHeight);
	//! Try to find a place where can be placed a glyph with given size
	//! If it is imposible, resize the texture and try it again
	CFontTexture::IGlyphRect AllocateGlyphRect(int glyphWidth, int glyphHeight);
};

#endif // CFONTTEXTURE_H
