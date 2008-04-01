#ifndef __OUTLINE_FONT_H__
#define __OUTLINE_FONT_H__

#include "Rendering/glFont.h"

// OutlineFont.h: interface for the COutlineFont class.
//
//////////////////////////////////////////////////////////////////////

class COutlineFont {
	public:
		COutlineFont();
		~COutlineFont();

		void Enable(bool value) { enabled = value; }
		bool IsEnabled() const { return enabled; }

	private:
		bool enabled;
};


extern COutlineFont outlineFont;


#endif // __OUTLINE_FONT_H__
