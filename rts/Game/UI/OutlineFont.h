#ifndef __OUTLINE_FONT_H__
#define __OUTLINE_FONT_H__
// OutlineFont.h: interface for the COutlineFont class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)


class COutlineFont {
	public:
		COutlineFont();
		~COutlineFont();
		
		void print(float xPixelSize, float yPixelSize,
		           const float color[4], const char* text) const;
		           
		void Enable(bool value) { enabled = value; }
		bool IsEnabled() const { return enabled; }
		
	private:
		bool enabled;
};


extern COutlineFont outlineFont;


#endif // __OUTLINE_FONT_H__
