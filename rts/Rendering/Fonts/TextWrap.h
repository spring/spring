/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TEXTWRAP_H
#define _TEXTWRAP_H

#include <string>
#include <list>

#include "CFontTexture.h"
#include "ustring.h"
#include "System/Color.h"


class CTextWrap : public CFontTexture
{
public:
	CTextWrap(const std::string& fontfile, int size, int outlinesize, float  outlineweight);
	virtual ~CTextWrap() {}

	//! Adds \n's (and '...' if it would be too high) until the text fits into maxWidth/maxHeight
	inline int WrapInPlace(std::string& text, float fontSize,  float maxWidth, float maxHeight = 1e9);
	inline std::string Wrap(const std::string& text, float fontSize, float maxWidth, float maxHeight = 1e9);

	static const char8_t ColorCodeIndicator  = 0xFF;
	static const char8_t ColorResetIndicator = 0x08; //! =: '\\b'

public:
	virtual float GetTextWidth(const std::string& text) = 0;

private:
	struct colorcode {
		colorcode() : resetColor(false),color(1.f,1.f,1.f,1.f),pos(0) {};
		bool resetColor;
		SColor color;
		unsigned int pos;

		std::string tostring() const {
			std::string out;
			out.reserve(4);
			if (resetColor) {
				out = ColorResetIndicator;
			} else {
				out = ColorCodeIndicator;
				out += color.r;
				out += color.g;
				out += color.b;
			}
			return out;
		}
	};
	struct word {
		word() : width(0.0f), text(""), isSpace(false), isLineBreak(false), isColorCode(false), numSpaces(0), pos(0) {};

		float width;
		std::u8string text;
		bool isSpace;
		bool isLineBreak;
		bool isColorCode;
		unsigned int numSpaces;
		unsigned int pos; //! position in the original text (needed for remerging colorcodes after wrapping; in printable chars (linebreaks and space don't count))
	};
	struct line {
		line() : width(0.0f), cost(0.0f), forceLineBreak(false) {};

		std::list<word>::iterator start, end;
		float width;
		float cost;
		bool forceLineBreak;
	};

	word SplitWord(word& w, float wantedWidth, bool smart = true);

	void SplitTextInWords(const std::u8string& text, std::list<word>* words, std::list<colorcode>* colorcodes);
	void RemergeColorCodes(std::list<word>* words, std::list<colorcode>& colorcodes) const;

	void AddEllipsis(std::list<line>& lines, std::list<word>& words, float maxWidth);

	void WrapTextConsole(std::list<word>& words, float maxWidth, float maxHeight);

	int WrapInPlace(std::u8string& text, float fontSize,  float maxWidth, float maxHeight = 1e9);
	std::u8string Wrap(const std::u8string& text, float fontSize, float maxWidth, float maxHeight = 1e9);
};

// wrappers
int CTextWrap::WrapInPlace(std::string& text, float fontSize,  float maxWidth, float maxHeight)
{
	return WrapInPlace(toustring(text), fontSize, maxWidth, maxHeight);
}
std::string CTextWrap::Wrap(const std::string& text, float fontSize, float maxWidth, float maxHeight)
{
	return Wrap(toustring(text), fontSize, maxWidth, maxHeight);
}

#endif /* _TEXTWRAP_H */
