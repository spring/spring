#ifndef _CFONTTEXTURE_H
#define _CFONTTEXTURE_H
#include <map>
#include <list>
#include <string>
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
        int GetTextureWidth() const ;
        int GetTextureHeight() const;
        int GetOutlineSize() const;
        float GetOutilneWeight() const;
        int GetLineHeightA() const;
        int GetFontDescender() const;

        int GetTexture() const;

        struct IGlyphRect
        {
            IGlyphRect():
                x(0),y(0),
                w(0),h(0)
            {
            };

            IGlyphRect(int _x,int _y,int _w,int _h):
                x(_x),
                y(_y),
                w(_w),
                h(_h)
            {
            };

            int x0() const{return x;};
            int x1() const{return x+w;};
            int y0() const{return y;};
            int y1() const{return y+h;};

            int x,y;
            int w,h;
        };

        struct GlyphInfo
        {
            GlyphInfo():
                size(),
                texCord(0,0,1,1),
                advance(1),
                height(1),
                descender(0),
                index(0)
            {
            };

            IGlyphRect size;
            IGlyphRect texCord;
            int advance, height, descender;
            unsigned int index;
        };
        //! Get or load a glyph
        const CFontTexture::GlyphInfo& GetGlyph(unsigned int ch);

    protected:
        void* GetLibrary() const;
        void* GetFace() const;

    protected:
        int GetKerning(unsigned int lchar,unsigned int rchar);
        int GetKerning(const CFontTexture::GlyphInfo& lgl,const CFontTexture::GlyphInfo& rgl);

    private:
        void* library;
        void* face;
        unsigned char* faceDataBuffer;
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
        //! Copy glyph pixels on the texture
        void Update(const unsigned char* pixels,int x,int y,int w,int h);

        void Clear(int x,int y,int w,int h);
    private:
        std::map<unsigned int,GlyphInfo> glyphs;    //!UTF32 -> GlyphInfo
        CFontTexture::GlyphInfo& LoadGlyph(unsigned int ch);

    private:
        struct Row
        {
            Row(int _ypos,int _height):
                position(_ypos),
                height(_height),
                wight(0)
            {
            };

            int position;
            int height;
            int wight;
        };
        std::list<Row> imageRows;
        int nextRowPos;

        Row* FindRow(unsigned int glyphWidth,unsigned int glyphHeight);
        Row* AddRow(unsigned int glyphWidth,unsigned int glyphHeight);
        //! Try to find a place where can be placed a glyph with given size
        //! If it is imposible, resize the texture and try it again
        CFontTexture::IGlyphRect AllocateGlyphRect(unsigned int glyphWidth,unsigned int glyphHeight);
};

#endif // CFONTTEXTURE_H
