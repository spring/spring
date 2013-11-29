/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "glFont.h"
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

#undef GetCharWidth // winapi.h

using std::string;


#define LOG_SECTION_FONT "Font"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_FONT)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_FONT

/*******************************************************************************/
/*******************************************************************************/

CglFont* font;
CglFont* smallFont;

static const unsigned char nullChar = 0;
static const float4        white(1.00f, 1.00f, 1.00f, 0.95f);
static const float4  darkOutline(0.05f, 0.05f, 0.05f, 0.95f);
static const float4 lightOutline(0.95f, 0.95f, 0.95f, 0.8f);

static const float darkLuminosity = 0.05 +
	0.2126f * math::powf(darkOutline[0], 2.2) +
	0.7152f * math::powf(darkOutline[1], 2.2) +
	0.0722f * math::powf(darkOutline[2], 2.2);


/*******************************************************************************/
/*******************************************************************************/

static inline unsigned count_leading_ones(uint8_t x)
{
	uint32_t i = ~x;
	return __builtin_clz((i<<24) | 0x00FFFFFF);
}


static char32_t GetUnicodeChar(const std::string& text, int& pos)
{
	// UTF8 looks like this
	// 1Byte == ASCII:      0xxxxxxxxx
	// 2Bytes encoded char: 110xxxxxxx 10xxxxxx
	// 3Bytes encoded char: 1110xxxxxx 10xxxxxx 10xxxxxx
	// 4Bytes encoded char: 11110xxxxx 10xxxxxx 10xxxxxx 10xxxxxx
	// Originaly there were 5&6 byte versions too, but they were dropped in RFC 3629.
	// So UTF8 maps to UTF16 range only.

	static const auto UTF8_CONT_MASK = 0xC0; // 11xxxxxx
	static const auto UTF8_CONT_OKAY = 0x80; // 10xxxxxx

	union UTF8_4Byte {
		uint32_t i;
		uint8_t  c[4];
	};

	// read next 4bytes and check if it is an utf8 sequence
	UTF8_4Byte utf8 = { 0 };
	const auto remainingChars = text.length() - pos;
	if (remainingChars >= 3) {
		utf8.i = *(uint32_t*)(&text[pos]);
	} else {
		// end of string reached only read till end
		switch (remainingChars) {
			case 3: utf8.c[2] = ((uint32_t)(text[pos + 2]));
			case 2: utf8.c[1] = ((uint32_t)(text[pos + 1]));
			case 1: utf8.c[0] = ((uint32_t)(text[pos    ]));
				break;
			default:
				assert(false);
		};
	}

	// how many bytes are requested for our multi-byte utf8 sequence
	unsigned clo = count_leading_ones(utf8.c[0]);
	if (clo>4) clo = 0; // ignore >=5 byte ones cause of RFC 3629

	// how many healthy utf bytes are following
	unsigned numValidUtf8Bytes = 1; // first char is always valid
	numValidUtf8Bytes += int((utf8.c[1] & UTF8_CONT_MASK) == UTF8_CONT_OKAY);
	numValidUtf8Bytes += int((utf8.c[2] & UTF8_CONT_MASK) == UTF8_CONT_OKAY);
	numValidUtf8Bytes += int((utf8.c[3] & UTF8_CONT_MASK) == UTF8_CONT_OKAY);

	// check if enough trailing utf8 bytes are healthy
	// else ignore utf8 and parse it as 8bit Latin-1 char (extended ASCII)
	// this adds backwardcompatibility with the old renderer
	// which supported extended ASCII with umlauts etc.
	const auto usedUtf8Bytes = (clo <= numValidUtf8Bytes) ? clo : 0;

	char32_t u = 0;
	switch (usedUtf8Bytes) {
		case 0:
		case 1: {
			u  = utf8.c[0];
		} break;
		case 2: {
			u  = ((char32_t)(utf8.c[0] & 0x1F)) << 6;
			u |= ((char32_t)(utf8.c[1] & 0x3F));
			pos += 1;
		} break;
		case 3: {
			u  = ((char32_t)(utf8.c[0] & 0x0F)) << 12;
			u |= ((char32_t)(utf8.c[1] & 0x3F)) << 6;
			u |= ((char32_t)(utf8.c[2] & 0x3F));
			pos += 2;
		} break;
		case 4: {
			u  = ((char32_t)(utf8.c[0] & 0x07)) << 18;
			u |= ((char32_t)(utf8.c[1] & 0x3F)) << 12;
			u |= ((char32_t)(utf8.c[2] & 0x3F)) << 6;
			u |= ((char32_t)(utf8.c[3] & 0x3F));
			//TODO limit range to UTF16!
			pos += 3;
		} break;
	}
	return u;
}


/*******************************************************************************/
/*******************************************************************************/

CglFont::CglFont(const std::string& fontfile, int size, int _outlinewidth, float _outlineweight)
: CFontTexture(fontfile,size,_outlinewidth,_outlineweight)
, fontPath(fontfile)
, fontSize(size)
, inBeginEnd(false)
, autoOutlineColor(true)
, setColor(false)
{
	va  = new CVertexArray();
	va2 = new CVertexArray();

	fontFamily = "unknown";
	fontStyle  = "unknown";
#ifndef HEADLESS
	fontFamily = GetFace()->family_name;
	fontStyle  = GetFace()->style_name;
#endif

	textColor    = white;
	outlineColor = darkOutline;
}

CglFont* CglFont::LoadFont(const std::string& fontFile, int size, int outlinewidth, float outlineweight)
{
	try {
		CglFont* newFont = new CglFont(fontFile, size, outlinewidth, outlineweight);
		return newFont;
	} catch (const content_error& ex) {
		LOG_L(L_ERROR, "Failed creating font: %s", ex.what());
		return NULL;
	}
}


CglFont::~CglFont()
{
	delete va;
	delete va2;
}


/*******************************************************************************/
/*******************************************************************************/

template <typename T>
static inline int SkipColorCodesOld(const std::string& text, T pos)
{
	while (text[pos] == CglFont::ColorCodeIndicator) {
		pos += 4;
		if (pos >= text.size()) { return -1; }
	}
	return pos;
}


template <typename T>
static inline int SkipColorCodes(const std::string& text, T* pos, float4* color)
{
	int colorFound = 0;
	while (text[(*pos)] == CglFont::ColorCodeIndicator) {
		(*pos) += 4;
		if ((*pos) >= text.size()) {
			return -(1 + colorFound);
		} else {
			(*color)[0] = ((unsigned char) text[(*pos)-3]) / 255.0f;
			(*color)[1] = ((unsigned char) text[(*pos)-2]) / 255.0f;
			(*color)[2] = ((unsigned char) text[(*pos)-1]) / 255.0f;
			colorFound = 1;
		}
	}
	return colorFound;
}


template <typename T>
static inline bool SkipColorCodesAndNewLines(const std::string& text, T* pos, float4* color, bool* colorChanged, int* skippedLines, float4* colorReset)
{
	const size_t length = text.length();
	(*colorChanged) = false;
	(*skippedLines) = 0;
	while (*pos < length) {
		const char& chr = text[*pos];
		switch(chr) {
			case CglFont::ColorCodeIndicator:
				*pos += 4;
				if ((*pos) < length) {
					(*color)[0] = ((unsigned char) text[(*pos) - 3]) / 255.0f;
					(*color)[1] = ((unsigned char) text[(*pos) - 2]) / 255.0f;
					(*color)[2] = ((unsigned char) text[(*pos) - 1]) / 255.0f;
					*colorChanged = true;
				}
				break;

			case CglFont::ColorResetIndicator:
				(*pos)++;
				(*color) = *colorReset;
				*colorChanged = true;
				break;

			case '\x0d': //! CR
				(*skippedLines)++;
				(*pos)++;
				if (*pos < length && text[*pos] == '\x0a') { //! CR+LF
					(*pos)++;
				}
				break;

			case '\x0a': //! LF
				(*skippedLines)++;
				(*pos)++;
				break;

			default:
				return false;
		}
	}
	return true;
}


static inline void TextStripCallback(void* data)
{
	CglFont::ColorMap::iterator& sci = *reinterpret_cast<CglFont::ColorMap::iterator*>(data);
	glColor4fv(*sci++);
}


/*******************************************************************************/
/*******************************************************************************/

std::string CglFont::StripColorCodes(const std::string& text)
{
	const size_t len = text.size();

	std::string nocolor;
	nocolor.reserve(len);
	for (int i = 0; i < len; i++) {
		if (text[i] == ColorCodeIndicator) {
			i += 3;
		} else {
			nocolor += text[i];
		}
	}
	return nocolor;
}


float CglFont::GetCharacterWidth(const char32_t c)
{
	return GetGlyph(c).advance;
}


float CglFont::GetTextWidth(const std::string& text)
{
	if (text.size()==0) return 0.0f;

	float w = 0.0f;
	float maxw = 0.0f;

	const GlyphInfo* prv_g=NULL;
	const GlyphInfo* cur_g;

	for (int pos = 0; pos < text.length(); pos++) {
		const char& c = text[pos];
		switch(c) {
			// inlined colorcode
			case ColorCodeIndicator:
				pos = SkipColorCodesOld(text, pos);
				if (pos<0) {
					pos = text.length();
				} else {
					pos--;
				}
				break;

			// reset color
			case ColorResetIndicator:
				break;

			// newline
			case '\x0d': //! CR+LF
				if (pos+1 < text.length() && text[pos+1] == '\x0a')
					pos++;
			case '\x0a': //! LF
				if (prv_g) w += prv_g->advance;
				if (w > maxw)
					maxw = w;
				w = 0.0f;
				prv_g = NULL;
				break;

			// printable char
			default:
				char32_t u = GetUnicodeChar(text, pos);
				cur_g = &GetGlyph(u);
				if (prv_g) w += GetKerning(*prv_g, *cur_g);
				prv_g = cur_g;
		}
	}

	if (prv_g)
		w += prv_g->advance;
	if (w > maxw)
		maxw = w;

	return maxw;
}


float CglFont::GetTextHeight(const std::string& text, float* descender, int* numLines)
{
	if (text.empty()) {
		if (descender) *descender = 0.0f;
		if (numLines) *numLines = 0;
		return 0.0f;
	}

	float h = 0.0f, d = GetLineHeight() + GetDescender();
	unsigned int multiLine = 1;

	for (int pos = 0 ; pos < text.length(); pos++) {
		const char& c = text[pos];
		switch(c) {
			//! inlined colorcode
			case ColorCodeIndicator:
				pos = SkipColorCodesOld(text, pos);
				if (pos<0) {
					pos = text.length();
				} else {
					pos--;
				}
				break;

			//! reset color
			case ColorResetIndicator:
				break;

			//! newline
			case '\x0d': //! CR+LF
				if (pos+1 < text.length() && text[pos+1] == '\x0a')
					pos++;
			case '\x0a': //! LF
				multiLine++;
				d = GetLineHeight() + GetDescender();
				break;

			//! printable char
			default:
				char32_t u = GetUnicodeChar(text,pos);
				const GlyphInfo& g = GetGlyph(u);
				if (g.descender < d) d = g.descender;
				if (multiLine < 2 && g.height > h) h = g.height; //! only calc height for the first line
		}
	}

	if (multiLine>1) d -= (multiLine-1) * GetLineHeight();
	if (descender) *descender = d;
	if (numLines) *numLines = multiLine;

	return h;
}


int CglFont::GetTextNumLines(const std::string& text) const
{
	if (text.empty())
		return 0;

	int lines = 1;

	for (int pos = 0 ; pos < text.length(); pos++) {
		const char& c = text[pos];
		switch(c) {
			//! inlined colorcode
			case ColorCodeIndicator:
				pos = SkipColorCodesOld(text, pos);
				if (pos<0) {
					pos = text.length();
				} else {
					pos--;
				}
				break;

			//! reset color
			case ColorResetIndicator:
				break;

			//! newline
			case '\x0d':
				if (pos+1 < text.length() && text[pos+1] == '\x0a')
					pos++;
			case '\x0a':
				lines++;
				break;

			//default:
		}
	}

	return lines;
}

/*******************************************************************************/
/*******************************************************************************/

/**
 * @brief IsUpperCase
 * @return true if the given uchar is an uppercase character (WinLatin charmap)
 */
static inline bool IsUpperCase(const char32_t& c)
{
	// overkill to add unicode
	return
		(c >= 0x41 && c <= 0x5A) ||
		(c >= 0xC0 && c <= 0xD6) ||
		(c >= 0xD8 && c <= 0xDE) ||
		(c == 0x8A) ||
		(c == 0x8C) ||
		(c == 0x8E) ||
		(c == 0x9F);
}

static inline bool IsLowerCase(const char32_t& c)
{
	// overkill to add unicode
	return c >= 0x61 && c <= 0x7A; // only ascii (no latin-1!)
}


/**
 * @brief GetPenalty
 * @param c character at %strpos% in the word
 * @param strpos position of c in the word
 * @param strlen total length of the word
 * @return penalty (smaller is better) to split a word at that position
 */
static inline float GetPenalty(const char32_t& c, unsigned int strpos, unsigned int strlen)
{
	const float dist = strlen - strpos;

	if (dist > (strlen / 2) && dist < 4) {
		return 1e9;
	} else if (IsLowerCase(c)) {
		//! lowercase char
		return 1.0f + (strlen - strpos);
	} else if (c >= 0x30 && c <= 0x39) {
		//! is number
		return 1.0f + (strlen - strpos)*0.9;
	} else if (IsUpperCase(c)) {
		//! uppercase char
		return 1.0f + (strlen - strpos)*0.75;
	} else {
		//! any special chars
		return Square(dist / 4);
	}
	return Square(dist / 4);
}


CglFont::word CglFont::SplitWord(CglFont::word& w, float wantedWidth, bool smart)
{
	//! returns two pieces 'L'eft and 'R'ight of the split word (returns L, *wi becomes R)

	word w2;
	w2.pos = w.pos;

	const float spaceAdvance = GetGlyph(0x20).advance;
	if (w.isLineBreak) {
		//! shouldn't happen
		w2 = w;
		w.isSpace = true;
	} else if (w.isSpace) {
		int split = (int)math::floor(wantedWidth / spaceAdvance);
		w2.isSpace   = true;
		w2.numSpaces = split;
		w2.width     = spaceAdvance * w2.numSpaces;
		w.numSpaces -= split;
		w.width      = spaceAdvance * w.numSpaces;
		w.pos       += split;
	} else {
		if (!smart) {
			float width = 0.0f;
			int i = 0;
			char32_t c = GetUnicodeChar(w.text,i);
			do {
				const GlyphInfo& g = GetGlyph(c);
				++i;

				if (i < w.text.length()) {
					c = GetUnicodeChar(w.text,i);
					width += GetKerning(g, GetGlyph(c));
				} else {
					width += GetKerning(g, GetGlyph(0x20));
				}

				if (width > wantedWidth) {
					w2.text  = w.text.substr(0,i - 1);
					w2.width = GetTextWidth(w2.text);
					w.text.erase(0,i - 1);
					w.width  = GetTextWidth(w.text);
					w.pos   += i - 1;
					return w2;
				}
			} while(i < w.text.length());
		}

		if(
			(wantedWidth < 8 * spaceAdvance) ||
			(w.text.length() < 1)
		) {
			w2.isSpace = true;
			return w2;
		}

		float width = 0.0f;
		int i = 0;
		float min_penalty = 1e9;
		unsigned int goodbreak = 0;
		char32_t c = GetUnicodeChar(w.text,i);
		do {
			char32_t co = c;
			unsigned int io = i;
			const GlyphInfo& g = GetGlyph(c);
			++i;

			if (i < w.text.length()) {
				c = GetUnicodeChar(w.text,i);
				width += GetKerning(g,GetGlyph(c));
			} else {
				width += GetKerning(g,GetGlyph(0x20));
			}

			if (width > wantedWidth) {
				w2.text  = w.text.substr(0,goodbreak);
				w2.width = GetTextWidth(w2.text);
				w.text.erase(0,goodbreak);
				w.width  = GetTextWidth(w.text);
				w.pos   += goodbreak;
				break;
			}

			float penalty = GetPenalty(co, io, w.text.length());
			if (penalty < min_penalty) {
				min_penalty = penalty;
				goodbreak   = i;
			}

		} while(i < w.text.length());
	}
	return w2;
}


void CglFont::AddEllipsis(std::list<line>& lines, std::list<word>& words, float maxWidth)
{
	const float ellipsisAdvance = GetGlyph(0x85).advance;
	const float spaceAdvance = GetGlyph(0x20).advance;

	if (ellipsisAdvance > maxWidth)
		return;

	line* l = &(lines.back());

	//! If the last line ends with a linebreak, remove it
	std::list<word>::iterator wi_end = l->end;
	if (wi_end->isLineBreak) {
		if (l->start == l->end || l->end == words.begin()) {
			//! there is just the linebreak in that line, so replace linebreak with just a null space
			word w;
			w.pos       = wi_end->pos;
			w.isSpace   = true;
			w.numSpaces = 0;
			l->start = words.insert(wi_end,w);
			l->end = l->start;

			words.erase(wi_end);
		} else {
			wi_end = words.erase(wi_end);
			l->end = --wi_end;
		}
	}

	//! remove as many words until we have enough free space for the ellipsis
	while (l->end != l->start) {
		word& w = *l->end;

		//! we have enough free space
		if (l->width + ellipsisAdvance < maxWidth)
			break;

		//! we can cut the last word to get enough freespace (so show as many as possible characters of that word)
		if (
			((l->width - w.width + ellipsisAdvance) < maxWidth) &&
			(w.width > ellipsisAdvance)
		) {
			break;
		}

		l->width -= w.width;
		--(l->end);
	}

	//! we don't even have enough space for the ellipsis
	word& w = *l->end;
	if ((l->width - w.width) + ellipsisAdvance > maxWidth)
		return;

	//! sometimes words aren't hyphenated for visual aspects
	//! but if we put an ellipsis in there, it is better to show as many as possible characters of those words
	std::list<word>::iterator nextwi(l->end);
	++nextwi;
	if (
		(!l->forceLineBreak) &&
		(nextwi != words.end()) &&
		(w.isSpace || w.isLineBreak) &&
		(l->width + ellipsisAdvance < maxWidth) &&
		!(nextwi->isSpace || nextwi->isLineBreak)
	) {
		float spaceLeft = maxWidth - (l->width + ellipsisAdvance);
		l->end = words.insert( nextwi, SplitWord(*nextwi, spaceLeft, false) );
		l->width += l->end->width;
	}

	//! the last word in the line needs to be cut
	if (l->width + ellipsisAdvance > maxWidth) {
		word& w = *l->end;
		l->width -= w.width;
		float spaceLeft = maxWidth - (l->width + ellipsisAdvance);
		l->end = words.insert( l->end, SplitWord(w, spaceLeft, false) );
		l->width += l->end->width;
	}

	//! put in a space between words and the ellipsis (if there is enough space)
	if (l->forceLineBreak && !l->end->isSpace) {
		if (l->width + ellipsisAdvance + spaceAdvance <= maxWidth) {
			word space;
			space.isSpace = true;
			space.numSpaces = 1;
			space.width = spaceAdvance;
			std::list<word>::iterator wi(l->end);
			++l->end;
			if (l->end == words.end()) {
				space.pos = wi->pos + wi->text.length() + 1;
			} else {
				space.pos = l->end->pos;
			}
			l->end = words.insert( l->end, space );
			l->width += l->end->width;
		}
	}

	//! add our ellipsis
	word ellipsis;
	ellipsis.text  = "\x85";
	ellipsis.width = ellipsisAdvance;
	std::list<word>::iterator wi(l->end);
	++l->end;
	if (l->end == words.end()) {
		ellipsis.pos = wi->pos + wi->text.length() + 1;
	} else {
		ellipsis.pos = l->end->pos;
	}
	l->end = words.insert( l->end, ellipsis );
	l->width += l->end->width;
}


void CglFont::WrapTextConsole(std::list<word>& words, float maxWidth, float maxHeight)
{
	if (words.empty() || (GetLineHeight()<=0.0f))
		return;
	const bool splitAllWords = false;
	const unsigned int maxLines = (unsigned int)math::floor(std::max(0.0f, maxHeight / GetLineHeight()));

	line* currLine;
	word linebreak;
	linebreak.isLineBreak = true;

	bool addEllipsis = false;
	bool currLineValid = false; //! true if there was added any data to the current line

	std::list<word>::iterator wi = words.begin();

//	std::list<word> splitWords;
	std::list<line> lines;
	lines.push_back(line());
	currLine = &(lines.back());
	currLine->start = words.begin();

	for (; ;) {
		currLineValid = true;
		if (wi->isLineBreak) {
			currLine->forceLineBreak = true;
			currLine->end = wi;

			//! start a new line after the '\n'
			lines.push_back(line());
			currLineValid = false;
			currLine = &(lines.back());
			currLine->start = wi;
			++currLine->start;
		} else {
			currLine->width += wi->width;
			currLine->end = wi;

			if (currLine->width > maxWidth) {
				currLine->width -= wi->width;

				//! line grew too long by adding the last word, insert a LineBreak
				const bool splitLastWord = (wi->width > (0.5 * maxWidth));
				const float freeWordSpace = (maxWidth - currLine->width);

				if (splitAllWords || splitLastWord) {
					//! last word W is larger than 0.5 * maxLineWidth, split it into
					//! get 'L'eft and 'R'ight parts of the split (wL becomes Left, *wi becomes R)

					bool restart = (currLine->start == wi);
					//! turns *wi into R
					word wL = SplitWord(*wi, freeWordSpace);

					if (splitLastWord && wL.width == 0.0f) {
						//! With smart splitting it can happen that the word isn't split at all,
						//! this can cause a race condition when the word is longer than maxWidth.
						//! In this case we have to force an unaesthetic split.
						wL = SplitWord(*wi, freeWordSpace, false);
					}

					//! increase by the width of the L-part of *wi
					currLine->width += wL.width;

					//! insert the L-part right before R
					wi = words.insert(wi, wL);
					if(restart)
						currLine->start = wi;
					++wi;
				}

				//! insert the forced linebreak (either after W or before R)
				linebreak.pos = wi->pos;
				currLine->end = words.insert(wi, linebreak);

				while (wi != words.end() && wi->isSpace)
					wi = words.erase(wi);

				lines.push_back(line());
				currLineValid = false;
				currLine = &(lines.back());
				currLine->start = wi;
				--wi; //! compensate the wi++ downwards
			}
		}

		++wi;

		if (wi == words.end()) {
			break;
		}

		if (lines.size() > maxLines) {
			addEllipsis = true;
			break;
		}
	}



	//! empty row
	if (!currLineValid || (currLine->start == words.end() && !currLine->forceLineBreak)) {
		lines.pop_back();
		currLine = &(lines.back());
	}

	//! if we had to cut the text because of missing space, add an ellipsis
	if (addEllipsis)
		AddEllipsis(lines, words, maxWidth);

	wi = currLine->end;
	++wi;
	wi = words.erase(wi, words.end());
}


//void CglFont::WrapTextKnuth(std::list<word>& words, float maxWidth, float maxHeight) const
//{
	// TODO FINISH ME!!! (Knuths algorithm would try to share deadspace between lines, with the smallest sum of (deadspace of line)^2)
//}


void CglFont::SplitTextInWords(const std::string& text, std::list<word>* words, std::list<colorcode>* colorcodes)
{
	const unsigned int length = (unsigned int)text.length();
	const float spaceAdvance = GetGlyph(0x20).advance;

	words->push_back(word());
	word* w = &(words->back());

	unsigned int numChar = 0;
	for (int pos = 0; pos < length; pos++) {
		const char& c = text[pos];
		switch(c) {
			//! space
			case '\x20':
				if (!w->isSpace) {
					if (w->isSpace) {
						w->width = spaceAdvance * w->numSpaces;
					} else if (!w->isLineBreak) {
						w->width = GetTextWidth(w->text);
					}
					words->push_back(word());
					w = &(words->back());
					w->isSpace = true;
					w->pos     = numChar;
				}
				w->numSpaces++;
				break;

			//! inlined colorcodes
			case ColorCodeIndicator:
				{
					colorcodes->push_back(colorcode());
					colorcode& cc = colorcodes->back();
					cc.pos = numChar;
					SkipColorCodes(text, &pos, &(cc.color));
					if (pos<0) {
						pos = length;
					} else {
						//! SkipColorCodes jumps 1 too far (it jumps on the first non
						//! colorcode char, but our for-loop will still do "pos++;")
						pos--;
					}
				} break;
			case ColorResetIndicator:
				{
					colorcode* cc = &colorcodes->back();
					if (cc->pos != numChar) {
						colorcodes->push_back(colorcode());
						cc = &colorcodes->back();
						cc->pos = numChar;
					}
					cc->resetColor = true;
				} break;

			//! newlines
			case '\x0d': //! CR+LF
				if (pos+1 < length && text[pos+1] == '\x0a')
					pos++;
			case '\x0a': //! LF
				if (w->isSpace) {
					w->width = spaceAdvance * w->numSpaces;
				} else if (!w->isLineBreak) {
					w->width = GetTextWidth(w->text);
				}
				words->push_back(word());
				w = &(words->back());
				w->isLineBreak = true;
				w->pos = numChar;
				break;

			//! printable chars
			default:
				if (w->isSpace || w->isLineBreak) {
					if (w->isSpace) {
						w->width = spaceAdvance * w->numSpaces;
					} else if (!w->isLineBreak) {
						w->width = GetTextWidth(w->text);
					}
					words->push_back(word());
					w = &(words->back());
					w->pos = numChar;
				}
				w->text += c;
				numChar++;
		}
	}
	if (w->isSpace) {
		w->width = spaceAdvance * w->numSpaces;
	} else if (!w->isLineBreak) {
		w->width = GetTextWidth(w->text);
	}
}


void CglFont::RemergeColorCodes(std::list<word>* words, std::list<colorcode>& colorcodes) const
{
	std::list<word>::iterator wi = words->begin();
	std::list<word>::iterator wi2 = words->begin();
	std::list<colorcode>::iterator ci;
	for (ci = colorcodes.begin(); ci != colorcodes.end(); ++ci) {
		while(wi != words->end() && wi->pos <= ci->pos) {
			wi2 = wi;
			++wi;
		}

		word wc;
		wc.pos = ci->pos;
		wc.isColorCode = true;

		if (ci->resetColor) {
			wc.text = ColorResetIndicator;
		} else {
			wc.text = ColorCodeIndicator;
			wc.text += (unsigned char)(255 * ci->color[0]);
			wc.text += (unsigned char)(255 * ci->color[1]);
			wc.text += (unsigned char)(255 * ci->color[2]);
		}


		if (wi2->isSpace || wi2->isLineBreak) {
			while(wi2 != words->end() && (wi2->isSpace || wi2->isLineBreak))
				++wi2;

			if (wi == words->end() && (wi2->pos + wi2->numSpaces) < ci->pos) {
				return;
			}

			wi2 = words->insert(wi2, wc);
		} else {
			if (wi == words->end() && (wi2->pos + wi2->text.size()) < (ci->pos + 1)) {
				return;
			}

			size_t pos = ci->pos - wi2->pos;
			if (pos < wi2->text.size() && pos > 0) {
				word w2;
				w2.text = wi2->text.substr(0,pos);
				w2.pos = wi2->pos;
				wi2->text.erase(0,pos);
				wi2->pos += pos;
				wi2 = words->insert(wi2, wc);
				wi2 = words->insert(wi2, w2);
			} else {
				wi2 = words->insert(wi2, wc);
			}
		}
		wi = wi2;
		++wi;
	}
}


int CglFont::WrapInPlace(std::string& text, float _fontSize, float maxWidth, float maxHeight)
{
	// TODO make an option to insert '-' for word wrappings (and perhaps try to syllabificate)

	if (_fontSize <= 0.0f)
		_fontSize = fontSize;

	const float maxWidthf  = maxWidth / _fontSize;
	const float maxHeightf = maxHeight / _fontSize;

	std::list<word> words;
	std::list<colorcode> colorcodes;

	SplitTextInWords(text, &words, &colorcodes);
	WrapTextConsole(words, maxWidthf, maxHeightf);
	//WrapTextKnuth(&lines, words, maxWidthf, maxHeightf);
	RemergeColorCodes(&words, colorcodes);

	//! create the wrapped string
	text = "";
	unsigned int numlines = 0;
	if (!words.empty()) {
		numlines++;
		for (std::list<word>::iterator wi = words.begin(); wi != words.end(); ++wi) {
			if (wi->isSpace) {
				for (unsigned int j = 0; j < wi->numSpaces; ++j) {
					text += " ";
				}
			} else if (wi->isLineBreak) {
				text += "\x0d\x0a";
				numlines++;
			} else {
				text += wi->text;
			}
		}
	}

	return numlines;
}


std::list<std::string> CglFont::Wrap(const std::string& text, float _fontSize, float maxWidth, float maxHeight)
{
	// TODO make an option to insert '-' for word wrappings (and perhaps try to syllabificate)

	if (_fontSize <= 0.0f)
		_fontSize = fontSize;

	const float maxWidthf  = maxWidth / _fontSize;
	const float maxHeightf = maxHeight / _fontSize;

	std::list<word> words;
	std::list<colorcode> colorcodes;

	SplitTextInWords(text, &words, &colorcodes);
	WrapTextConsole(words, maxWidthf, maxHeightf);
	//WrapTextKnuth(&lines, words, maxWidthf, maxHeightf);
	RemergeColorCodes(&words, colorcodes);

	//! create the string lines of the wrapped text
	std::list<word>::iterator lastColorCode = words.end();
	std::list<std::string> strlines;
	if (!words.empty()) {
		strlines.push_back("");
		std::string* sl = &strlines.back();
		for (std::list<word>::iterator wi = words.begin(); wi != words.end(); ++wi) {
			if (wi->isSpace) {
				for (unsigned int j = 0; j < wi->numSpaces; ++j) {
					*sl += " ";
				}
			} else if (wi->isLineBreak) {
				strlines.push_back("");
				sl = &strlines.back();
				if (lastColorCode != words.end())
					*sl += lastColorCode->text;
			} else {
				*sl += wi->text;
				if (wi->isColorCode)
					lastColorCode = wi;
			}
		}
	}

	return strlines;
}


/*******************************************************************************/
/*******************************************************************************/

void CglFont::SetAutoOutlineColor(bool enable)
{
	autoOutlineColor = enable;
}


void CglFont::SetTextColor(const float4* color)
{
	if (color == NULL) color = &white;

	if (inBeginEnd && !(*color==textColor)) {
		if ((va->stripArrayPos - va->stripArray) != (va->drawArrayPos - va->drawArray)) {
			stripTextColors.push_back(*color);
			va->EndStrip();
		} else {
			float4& back = stripTextColors.back();
			back = *color;
		}
	}

	textColor = *color;
}


void CglFont::SetOutlineColor(const float4* color)
{
	if (color == NULL) color = ChooseOutlineColor(textColor);

	if (inBeginEnd && !(*color==outlineColor)) {
		if ((va2->stripArrayPos - va2->stripArray) != (va2->drawArrayPos - va2->drawArray)) {
			stripOutlineColors.push_back(*color);
			va2->EndStrip();
		} else {
			float4& back = stripOutlineColors.back();
			back = *color;
		}
	}

	outlineColor = *color;
}


void CglFont::SetColors(const float4* _textColor, const float4* _outlineColor)
{
	if (_textColor == NULL) _textColor = &white;
	if (_outlineColor == NULL) _outlineColor = ChooseOutlineColor(*_textColor);

	if (inBeginEnd) {
		if (!(*_textColor==textColor)) {
			if ((va->stripArrayPos - va->stripArray) != (va->drawArrayPos - va->drawArray)) {
				stripTextColors.push_back(*_textColor);
				va->EndStrip();
			} else {
				float4& back = stripTextColors.back();
				back = *_textColor;
			}
		}
		if (!(*_outlineColor==outlineColor)) {
			if ((va2->stripArrayPos - va2->stripArray) != (va2->drawArrayPos - va2->drawArray)) {
				stripOutlineColors.push_back(*_outlineColor);
				va2->EndStrip();
			} else {
				float4& back = stripOutlineColors.back();
				back = *_outlineColor;
			}
		}
	}

	textColor    = *_textColor;
	outlineColor = *_outlineColor;
}


const float4* CglFont::ChooseOutlineColor(const float4& textColor)
{
	const float luminosity = 0.05 +
				 0.2126f * math::powf(textColor[0], 2.2) +
				 0.7152f * math::powf(textColor[1], 2.2) +
				 0.0722f * math::powf(textColor[2], 2.2);

	const float lumdiff = std::max(luminosity,darkLuminosity) / std::min(luminosity,darkLuminosity);
	if (lumdiff > 5.0f) {
		return &darkOutline;
	} else {
		return &lightOutline;
	}
}


void CglFont::Begin(const bool immediate, const bool resetColors)
{
	if (inBeginEnd) {
		LOG_L(L_ERROR, "called Begin() multiple times");
		return;
	}

	autoOutlineColor = true;

	setColor = !immediate;
	if (resetColors) {
		SetColors(); //! reset colors
	}

	inBeginEnd = true;

	va->Initialize();
	va2->Initialize();
	stripTextColors.clear();
	stripOutlineColors.clear();
	stripTextColors.push_back(textColor);
	stripOutlineColors.push_back(outlineColor);

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void CglFont::End()
{
	if (!inBeginEnd) {
		LOG_L(L_ERROR, "called End() without Begin()");
		return;
	}
	inBeginEnd = false;

	if (va->drawIndex() == 0) {
		glPopAttrib();
		return;
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, GetTexture());

	// Because texture size can change, texture coordinats are absolute in texels.
	// We could use also just use GL_TEXTURE_RECTANGLE
	// but then all shaders would need to detect so and use different funcs & types if supported -> more work
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glCallList(textureSpaceMatrix);
	glMatrixMode(GL_MODELVIEW);

	if (va2->drawIndex() > 0) {
		if (stripOutlineColors.size() > 1) {
			ColorMap::iterator sci = stripOutlineColors.begin();
			va2->DrawArray2dT(GL_QUADS,TextStripCallback,&sci);
		} else {
			glColor4fv(outlineColor);
			va2->DrawArray2dT(GL_QUADS);
		}
	}

	if (stripTextColors.size() > 1) {
		ColorMap::iterator sci = stripTextColors.begin();
		va->DrawArray2dT(GL_QUADS,TextStripCallback,&sci);
	} else {
		if (setColor) glColor4fv(textColor);
		va->DrawArray2dT(GL_QUADS);
	}

	// pop texture matrix
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glPopAttrib();
}


void CglFont::RenderString(float x, float y, const float& scaleX, const float& scaleY, const std::string& str)
{
	/**
	 * NOTE:
	 * Font rendering does not use display lists, but VAs. It's actually faster
	 * (450% faster with a 7600GT!) for these reasons:
	 *
	 * 1. When using DLs, we can not group multiple glyphs into one glBegin/End pair
	 *    because glTranslatef can not go between such a pair.
	 * 2. We can now eliminate all glPushMatrix/PopMatrix pairs related to font rendering
	 *    because the transformations are calculated on the fly. These are just a couple of
	 *    floating point multiplications and shouldn't be too expensive.
	 */

	const float startx = x;
	const float lineHeight_ = scaleY * GetLineHeight();
	unsigned int length = (unsigned int)str.length();

	va->EnlargeArrays(length * 4, 0, VA_SIZE_2DT);

	int skippedLines;
	bool endOfString, colorChanged;
	const GlyphInfo* g = NULL;
	float4 newColor;
	newColor[3] = 1.0f;
	char32_t c;
	int i = 0;

	do {
		endOfString = SkipColorCodesAndNewLines(str, &i, &newColor, &colorChanged, &skippedLines, &baseTextColor);

		if (endOfString)
			return;

		c = GetUnicodeChar(str,i);
		++i;

		if (colorChanged) {
			if (autoOutlineColor) {
				SetColors(&newColor,NULL);
			} else {
				SetTextColor(&newColor);
			}
		}
		const GlyphInfo* c_g = &GetGlyph(c);
		if (skippedLines>0) {
			x  = startx;
			y -= skippedLines * lineHeight_;
		} else if (g) {
			x += scaleX * GetKerning(*g, *c_g);
		}

		g = c_g;

		const auto&  tc = g->texCord;
		const float dx0 = (scaleX * g->size.x0()) + x, dy0 = (scaleY * g->size.y0()) + y;
		const float dx1 = (scaleX * g->size.x1()) + x, dy1 = (scaleY * g->size.y1()) + y;

		va->AddVertex2dQT(dx0, dy1, tc.x0(), tc.y1());
		va->AddVertex2dQT(dx0, dy0, tc.x0(), tc.y0());
		va->AddVertex2dQT(dx1, dy0, tc.x1(), tc.y0());
		va->AddVertex2dQT(dx1, dy1, tc.x1(), tc.y1());
	} while(true);
}


void CglFont::RenderStringShadow(float x, float y, const float& scaleX, const float& scaleY, const std::string& str)
{
	const float shiftX = scaleX*0.1, shiftY = scaleY*0.1;
	const float ssX = (scaleX/fontSize) * GetOutlineWidth(), ssY = (scaleY/fontSize) * GetOutlineWidth();

	const float startx = x;
	const float lineHeight_ = scaleY * GetLineHeight();
	unsigned int length = (unsigned int)str.length();

	va->EnlargeArrays(length * 4, 0, VA_SIZE_2DT);
	va2->EnlargeArrays(length * 4, 0, VA_SIZE_2DT);

	int skippedLines;
	bool endOfString, colorChanged;
	const GlyphInfo* g = NULL;
	float4 newColor;
	newColor[3] = 1.0f;
	char32_t c;
	int i = 0;

	do {
		endOfString = SkipColorCodesAndNewLines(str, &i, &newColor, &colorChanged, &skippedLines, &baseTextColor);

		if (endOfString)
			return;

		c = GetUnicodeChar(str,i);
		++i;

		if (colorChanged) {
			if (autoOutlineColor) {
				SetColors(&newColor,NULL);
			} else {
				SetTextColor(&newColor);
			}
		}

		const GlyphInfo* c_g = &GetGlyph(c);
		if (skippedLines>0) {
			x  = startx;
			y -= skippedLines * lineHeight_;
		} else if (g) {
			x += scaleX * GetKerning(*g, *c_g);
		}

		g = c_g;

		const auto&  tc = g->texCord;
		const auto& stc = g->shadowTexCord;
		const float dx0 = (scaleX * g->size.x0()) + x, dy0 = (scaleY * g->size.y0()) + y;
		const float dx1 = (scaleX * g->size.x1()) + x, dy1 = (scaleY * g->size.y1()) + y;

		//! draw shadow
		va2->AddVertex2dQT(dx0+shiftX-ssX, dy1-shiftY-ssY, stc.x0(), stc.y1());
		va2->AddVertex2dQT(dx0+shiftX-ssX, dy0-shiftY+ssY, stc.x0(), stc.y0());
		va2->AddVertex2dQT(dx1+shiftX+ssX, dy0-shiftY+ssY, stc.x1(), stc.y0());
		va2->AddVertex2dQT(dx1+shiftX+ssX, dy1-shiftY-ssY, stc.x1(), stc.y1());

		//! draw the actual character
		va->AddVertex2dQT(dx0, dy1, tc.x0(), tc.y1());
		va->AddVertex2dQT(dx0, dy0, tc.x0(), tc.y0());
		va->AddVertex2dQT(dx1, dy0, tc.x1(), tc.y0());
		va->AddVertex2dQT(dx1, dy1, tc.x1(), tc.y1());
	} while(true);
}

void CglFont::RenderStringOutlined(float x, float y, const float& scaleX, const float& scaleY, const std::string& str)
{
	const float shiftX = (scaleX/fontSize) * GetOutlineWidth(), shiftY = (scaleY/fontSize) * GetOutlineWidth();

	const float startx = x;
	const float lineHeight_ = scaleY * GetLineHeight();
	unsigned int length = (unsigned int)str.length();

	va->EnlargeArrays(length * 4, 0, VA_SIZE_2DT);
	va2->EnlargeArrays(length * 4, 0, VA_SIZE_2DT);

	int skippedLines;
	bool endOfString, colorChanged;
	const GlyphInfo* g = NULL;
	float4 newColor;
	newColor[3] = 1.0f;
	char32_t c;
	int i = 0;

	do {
		endOfString = SkipColorCodesAndNewLines(str, &i, &newColor, &colorChanged, &skippedLines, &baseTextColor);

		if (endOfString)
			return;

		c = GetUnicodeChar(str,i);
		++i;

		if (colorChanged) {
			if (autoOutlineColor) {
				SetColors(&newColor,NULL);
			} else {
				SetTextColor(&newColor);
			}
		}

		const GlyphInfo* c_g = &GetGlyph(c);
		if (skippedLines>0) {
			x  = startx;
			y -= skippedLines * lineHeight_;
		} else if (g) {
			x += scaleX * GetKerning(*g, *c_g);
		}

		g = c_g;

		const auto&  tc = g->texCord;
		const auto& stc = g->shadowTexCord;
		const float dx0 = (scaleX * g->size.x0()) + x, dy0 = (scaleY * g->size.y0()) + y;
		const float dx1 = (scaleX * g->size.x1()) + x, dy1 = (scaleY * g->size.y1()) + y;

		//! draw outline
		va2->AddVertex2dQT(dx0-shiftX, dy1-shiftY, stc.x0(), stc.y1());
		va2->AddVertex2dQT(dx0-shiftX, dy0+shiftY, stc.x0(), stc.y0());
		va2->AddVertex2dQT(dx1+shiftX, dy0+shiftY, stc.x1(), stc.y0());
		va2->AddVertex2dQT(dx1+shiftX, dy1-shiftY, stc.x1(), stc.y1());

		//! draw the actual character
		va->AddVertex2dQT(dx0, dy1, tc.x0(), tc.y1());
		va->AddVertex2dQT(dx0, dy0, tc.x0(), tc.y0());
		va->AddVertex2dQT(dx1, dy0, tc.x1(), tc.y0());
		va->AddVertex2dQT(dx1, dy1, tc.x1(), tc.y1());
	} while(true);
}


void CglFont::glWorldPrint(const float3& p, const float size, const std::string& str)
{
	glPushMatrix();
	glTranslatef(p.x, p.y, p.z);
	glMultMatrixf(camera->GetBillBoardMatrix());
	Begin(false, false);
	glPrint(0.0f, 0.0f, size, FONT_DESCENDER | FONT_CENTER | FONT_OUTLINE, str);
	End();
	glPopMatrix();
}


void CglFont::glPrint(float x, float y, float s, const int options, const std::string& text)
{
	//! s := scale or absolute size?
	if (options & FONT_SCALE) {
		s *= fontSize;
	}

	float sizeX = s, sizeY = s;

	//! render in normalized coords (0..1) instead of screencoords (0..~1024)
	if (options & FONT_NORM) {
		sizeX *= globalRendering->pixelX;
		sizeY *= globalRendering->pixelY;
	}

	//! horizontal alignment (FONT_LEFT is default)
	if (options & FONT_CENTER) {
		x -= sizeX * 0.5f * GetTextWidth(text);
	} else if (options & FONT_RIGHT) {
		x -= sizeX * GetTextWidth(text);
	}


	//! vertical alignment
	y += sizeY * GetDescender(); //! move to baseline (note: descender is negative)
	if (options & FONT_BASELINE) {
		//! nothing
	} else if (options & FONT_DESCENDER) {
		y -= sizeY * GetDescender();
	} else if (options & FONT_VCENTER) {
		float textDescender;
		y -= sizeY * 0.5f * GetTextHeight(text,&textDescender);
		y -= sizeY * 0.5f * textDescender;
	} else if (options & FONT_TOP) {
		y -= sizeY * GetTextHeight(text);
	} else if (options & FONT_ASCENDER) {
		y -= sizeY * GetDescender();
		y -= sizeY;
	} else if (options & FONT_BOTTOM) {
		float textDescender;
		GetTextHeight(text,&textDescender);
		y -= sizeY * textDescender;
	}

	if (options & FONT_NEAREST) {
		x = (int)x;
		y = (int)y;
	}

	// backup text & outline colors (also ::ColorResetIndicator will reset to those)
	baseTextColor = textColor;
	baseOutlineColor = outlineColor;

	//! immediate mode?
	const bool immediate = !inBeginEnd;
	if (immediate) {
		Begin(!(options & (FONT_OUTLINE | FONT_SHADOW)));
	}


	//! select correct decoration RenderString function
	if (options & FONT_OUTLINE) {
		RenderStringOutlined(x, y, sizeX, sizeY, text);
	} else if (options & FONT_SHADOW) {
		RenderStringShadow(x, y, sizeX, sizeY, text);
	} else {
		RenderString(x, y, sizeX, sizeY, text);
	}


	//! immediate mode?
	if (immediate) {
		End();
	}

	//! reset text & outline colors (if changed via in text colorcodes)
	SetColors(&baseTextColor,&baseOutlineColor);
}

void CglFont::glPrintTable(float x, float y, float s, const int options, const std::string& text)
{
	int col = 0;
	int row = 0;
	std::vector<std::string> coltext;
	std::vector<int> coldata;
	coltext.reserve(text.length());
	coltext.push_back("");
	unsigned char curcolor[4];
	unsigned char defaultcolor[4];
	defaultcolor[0] = ColorCodeIndicator;
	for(int i = 0; i < 3; ++i)
		defaultcolor[i+1] = (unsigned char)(textColor[i]*255.0f);
	coldata.push_back(*(int *)&defaultcolor);
	for(int i = 0; i < 4; ++i)
		curcolor[i] = defaultcolor[i];

	for (int pos = 0; pos < text.length(); pos++) {
		const char& c = text[pos];
		switch(c) {
			//! inline colorcodes
			case ColorCodeIndicator:
				for(int i = 0; i < 4 && pos < text.length(); ++i, ++pos) {
					coltext[col] += text[pos];
					((unsigned char *)curcolor)[i] = text[pos];
				}
				coldata[col] = *(int *)curcolor;
				--pos;
				break;

			// column separator is `\t`==`horizontal tab`
			case '\x09':
				++col;
				if(col >= coltext.size()) {
					coltext.push_back("");
					for(int i = 0; i < row; ++i)
						coltext[col] += '\x0a';
					coldata.push_back(*(int *)&defaultcolor);
				}
				if(coldata[col] != *(int *)curcolor) {
					for(int i = 0; i < 4; ++i)
						coltext[col] += curcolor[i];
					coldata[col] = *(int *)curcolor;
				}
				break;

			//! newline
			case '\x0d': //! CR+LF
				if (pos+1 < text.length() && text[pos + 1] == '\x0a')
					pos++;
			case '\x0a': //! LF
				for(int i = 0; i < coltext.size(); ++i)
					coltext[i] += '\x0a';
				if(coldata[0] != *(int *)curcolor) {
					for(int i = 0; i < 4; ++i)
						coltext[0] += curcolor[i];
					coldata[0] = *(int *)curcolor;
				}
				col = 0;
				++row;
				break;

			//! printable char
			default:
				coltext[col] += c;
		}
	}

	float totalWidth = 0.0f;
	float maxHeight = 0.0f;
	float minDescender = 0.0f;
	for(int i = 0; i < coltext.size(); ++i) {
		float colwidth = GetTextWidth(coltext[i]);
		coldata[i] = *(int *)&colwidth;
		totalWidth += colwidth;
		float textDescender;
		float textHeight = GetTextHeight(coltext[i], &textDescender);
		if(textHeight > maxHeight)
			maxHeight = textHeight;
		if(textDescender < minDescender)
			minDescender = textDescender;
	}

	//! s := scale or absolute size?
	float ss = s;
	if (options & FONT_SCALE) {
		ss *= fontSize;
	}

	float sizeX = ss, sizeY = ss;

	//! render in normalized coords (0..1) instead of screencoords (0..~1024)
	if (options & FONT_NORM) {
		sizeX *= globalRendering->pixelX;
		sizeY *= globalRendering->pixelY;
	}

	//! horizontal alignment (FONT_LEFT is default)
	if (options & FONT_CENTER) {
		x -= sizeX * 0.5f * totalWidth;
	} else if (options & FONT_RIGHT) {
		x -= sizeX * totalWidth;
	}

	//! vertical alignment
	if (options & FONT_BASELINE) {
		//! nothing
	} else if (options & FONT_DESCENDER) {
		y -= sizeY * GetDescender();
	} else if (options & FONT_VCENTER) {
		y -= sizeY * 0.5f * maxHeight;
		y -= sizeY * 0.5f * minDescender;
	} else if (options & FONT_TOP) {
		y -= sizeY * maxHeight;
	} else if (options & FONT_ASCENDER) {
		y -= sizeY * GetDescender();
		y -= sizeY;
	} else if (options & FONT_BOTTOM) {
		y -= sizeY * minDescender;
	}

	for(int i = 0; i < coltext.size(); ++i) {
		glPrint(x, y, s, (options | FONT_BASELINE) & ~(FONT_RIGHT | FONT_CENTER), coltext[i]);
		int colwidth = coldata[i];
		x += sizeX * *(float *)&colwidth;
	}
}


//! macro for formatting printf-style
#define FORMAT_STRING(lastarg,fmt,out)                         \
		char out[512];                         \
		va_list ap;                            \
		if (fmt == NULL) return;               \
		va_start(ap, lastarg);                 \
		VSNPRINTF(out, sizeof(out), fmt, ap);  \
		va_end(ap);

void CglFont::glFormat(float x, float y, float s, const int options, const char* fmt, ...)
{
	FORMAT_STRING(fmt,fmt,text);
	glPrint(x, y, s, options, string(text));
}


void CglFont::glFormat(float x, float y, float s, const int options, const string& fmt, ...)
{
	FORMAT_STRING(fmt,fmt.c_str(),text);
	glPrint(x, y, s, options, string(text));
}
