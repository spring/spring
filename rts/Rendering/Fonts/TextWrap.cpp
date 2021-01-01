/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TextWrap.h"
#include "glFont.h"
#include "FontLogSection.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"


static const char32_t spaceUTF16    = 0x20;
static const char32_t ellipsisUTF16 = 0x2026;
static const std::string ellipsisUTF8 = utf8::FromUnicode(ellipsisUTF16);

static constexpr const char* spaceStringTable[1 + 10] = {
	"",
	" ",
	"  ",
	"   ",
	"    ",
	"     ",
	"      ",
	"       ",
	"        ",
	"         ",
	"          ",
};



/*******************************************************************************/
/*******************************************************************************/

template <typename T>
static inline int SkipColorCodes(const std::u8string& text, T* pos, SColor* color)
{
	int colorFound = 0;
	while (text[(*pos)] == CTextWrap::ColorCodeIndicator) {
		(*pos) += 4;
		if ((*pos) >= text.size()) {
			return -(1 + colorFound);
		} else {
			color->r = text[(*pos)-3];
			color->g = text[(*pos)-2];
			color->b = text[(*pos)-1];
			colorFound = 1;
		}
	}
	return colorFound;
}


/*******************************************************************************/
/*******************************************************************************/

CTextWrap::CTextWrap(const std::string& fontfile, int size, int outlinewidth, float  outlineweight)
: CFontTexture(fontfile,size,outlinewidth,outlineweight)
{
}


/*******************************************************************************/
/*******************************************************************************/

/**
 * @brief IsUpperCase
 * @return true if the given char is an uppercase
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
		// lowercase char
		return 1.0f + (strlen - strpos);
	} else if (c >= 0x30 && c <= 0x39) {
		// is number
		return 1.0f + (strlen - strpos)*0.9;
	} else if (IsUpperCase(c)) {
		// uppercase char
		return 1.0f + (strlen - strpos)*0.75;
	}

	// any special chars
	return Square(dist / 4);
}


CTextWrap::word CTextWrap::SplitWord(CTextWrap::word& w, float wantedWidth, bool smart)
{
	// returns two pieces 'L'eft and 'R'ight of the split word (returns L, *wi becomes R)

	word w2;
	w2.pos = w.pos;

	const float spaceAdvance = GetGlyph(spaceUTF16).advance;
	if (w.isLineBreak) {
		// shouldn't happen
		w2 = w;
		w.isSpace = true;
	} else if (w.isSpace) {
		const int split = (int)std::floor(wantedWidth / spaceAdvance);
		w2.isSpace   = true;
		w2.numSpaces = split;
		w2.width     = spaceAdvance * w2.numSpaces;
		w.numSpaces -= split;
		w.width      = spaceAdvance * w.numSpaces;
		w.pos       += split;
	} else {
		if (smart) {
			if (
				(wantedWidth < 8 * spaceAdvance) ||
				(w.text.length() < 1)
			) {
				w2.isSpace = true;
				return w2;
			}
		}

		float width = 0.0f;
		int i = 0;
		float min_penalty = 1e9;
		unsigned int goodbreak = 0;
		char32_t c = utf8::GetNextChar(w.text,i);
		const GlyphInfo* curGlyph = &GetGlyph(c);
		const GlyphInfo* nextGlyph = curGlyph;

		do {
			const int lastCharPos = i;
			const char32_t co     = c;
			curGlyph = nextGlyph;
			c = utf8::GetNextChar(w.text,i);
			nextGlyph = &GetGlyph(c);
			width += GetKerning(*curGlyph, *nextGlyph);

			if (width > wantedWidth) {
				break;
			}

			if (smart) {
				const float penalty = GetPenalty(co, lastCharPos, w.text.length());
				if (penalty < min_penalty) {
					min_penalty = penalty;
					goodbreak   = lastCharPos;
				}
			} else {
				goodbreak = i;
			}
		} while(i < w.text.length());

		w2.text  = toustring(w.text.substr(0,goodbreak));
		w2.width = GetTextWidth(w2.text);
		w.text.erase(0,goodbreak);
		w.width  = GetTextWidth(w.text);
		w.pos   += goodbreak;
	}
	return w2;
}


void CTextWrap::AddEllipsis(std::list<line>& lines, std::list<word>& words, float maxWidth)
{
	const float ellipsisAdvance = GetGlyph(ellipsisUTF16).advance;
	const float spaceAdvance = GetGlyph(spaceUTF16).advance;

	if (ellipsisAdvance > maxWidth)
		return;

	line* l = &(lines.back());

	// If the last line ends with a linebreak, remove it
	std::list<word>::iterator wi_end = l->end;
	if (wi_end->isLineBreak) {
		if (l->start == l->end || l->end == words.begin()) {
			// there is just the linebreak in that line, so replace linebreak with just a null space
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

	// remove as many words until we have enough free space for the ellipsis
	while (l->end != l->start) {
		word& w = *l->end;

		// we have enough free space
		if (l->width + ellipsisAdvance < maxWidth)
			break;

		// we can cut the last word to get enough freespace (so show as many as possible characters of that word)
		if (
			((l->width - w.width + ellipsisAdvance) < maxWidth) &&
			(w.width > ellipsisAdvance)
		) {
			break;
		}

		l->width -= w.width;
		--(l->end);
	}

	// we don't even have enough space for the ellipsis
	word& w = *l->end;
	if ((l->width - w.width) + ellipsisAdvance > maxWidth)
		return;

	// sometimes words aren't hyphenated for visual aspects
	// but if we put an ellipsis in there, it is better to show as many as possible characters of those words
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

	// the last word in the line needs to be cut
	if (l->width + ellipsisAdvance > maxWidth) {
		word& w = *l->end;
		l->width -= w.width;
		float spaceLeft = maxWidth - (l->width + ellipsisAdvance);
		l->end = words.insert( l->end, SplitWord(w, spaceLeft, false) );
		l->width += l->end->width;
	}

	// put in a space between words and the ellipsis (if there is enough space)
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

	// add our ellipsis
	word ellipsis;
	ellipsis.text  = toustring(ellipsisUTF8);
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


void CTextWrap::WrapTextConsole(std::list<word>& words, float maxWidth, float maxHeight)
{
	if (words.empty() || (GetLineHeight()<=0.0f))
		return;
	const bool splitAllWords = false;
	const unsigned int maxLines = (unsigned int)std::floor(std::max(0.0f, maxHeight / GetLineHeight()));

	line* currLine;
	word linebreak;
	linebreak.isLineBreak = true;

	bool addEllipsis = false;
	bool currLineValid = false; // true if there was added any data to the current line

	std::list<word>::iterator wi = words.begin();

	std::list<line> lines;
	lines.emplace_back();
	currLine = &(lines.back());
	currLine->start = words.begin();

	for (; ;) {
		currLineValid = true;
		if (wi->isLineBreak) {
			currLine->forceLineBreak = true;
			currLine->end = wi;

			// start a new line after the '\n'
			lines.emplace_back();
			currLineValid = false;
			currLine = &(lines.back());
			currLine->start = wi;
			++currLine->start;
		} else {
			currLine->width += wi->width;
			currLine->end = wi;

			if (currLine->width > maxWidth) {
				currLine->width -= wi->width;

				// line grew too long by adding the last word, insert a LineBreak
				const bool splitLastWord = (wi->width > (0.5 * maxWidth));
				const float freeWordSpace = (maxWidth - currLine->width);

				if (splitAllWords || splitLastWord) {
					// last word W is larger than 0.5 * maxLineWidth, split it into
					// get 'L'eft and 'R'ight parts of the split (wL becomes Left, *wi becomes R)

					bool restart = (currLine->start == wi);
					// turns *wi into R
					word wL = SplitWord(*wi, freeWordSpace);

					if (splitLastWord && wL.width == 0.0f) {
						// With smart splitting it can happen that the word isn't split at all,
						// this can cause a race condition when the word is longer than maxWidth.
						// In this case we have to force an unaesthetic split.
						wL = SplitWord(*wi, freeWordSpace, false);
					}

					// increase by the width of the L-part of *wi
					currLine->width += wL.width;

					// insert the L-part right before R
					wi = words.insert(wi, wL);
					if (restart)
						currLine->start = wi;
					++wi;
				}

				// insert the forced linebreak (either after W or before R)
				linebreak.pos = wi->pos;
				currLine->end = words.insert(wi, linebreak);

				while (wi != words.end() && wi->isSpace)
					wi = words.erase(wi);

				lines.emplace_back();
				currLineValid = false;
				currLine = &(lines.back());
				currLine->start = wi;
				--wi; // compensate the wi++ downwards
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

	// empty row
	if (!currLineValid || (currLine->start == words.end() && !currLine->forceLineBreak)) {
		lines.pop_back();
		currLine = &(lines.back());
	}

	// if we had to cut the text because of missing space, add an ellipsis
	if (addEllipsis)
		AddEllipsis(lines, words, maxWidth);

	wi = currLine->end;
	++wi;
	wi = words.erase(wi, words.end());
}


void CTextWrap::SplitTextInWords(const std::u8string& text, std::list<word>* words, std::list<colorcode>* colorcodes)
{
	const unsigned int length = (unsigned int)text.length();
	const float spaceAdvance = GetGlyph(spaceUTF16).advance;

	words->push_back(word());
	word* w = &(words->back());

	unsigned int numChar = 0;
	for (int pos = 0; pos < length; pos++) {
		const char8_t& c = text[pos];
		switch(c) {
			// space
			case spaceUTF16:
				if (!w->isSpace) {
					if (!w->isLineBreak) {
						w->width = GetTextWidth(w->text);
					}
					words->push_back(word());
					w = &(words->back());
					w->isSpace = true;
					w->pos     = numChar;
				}
				w->numSpaces++;
				w->width = spaceAdvance * w->numSpaces;
				break;

			// inlined colorcodes
			case ColorCodeIndicator: {
				colorcodes->push_back(colorcode());
				colorcode& cc = colorcodes->back();
				cc.pos = numChar;

				SkipColorCodes(text, &pos, &(cc.color));

				if (pos < 0) {
					pos = length;
				} else {
					// SkipColorCodes jumps 1 too far (it jumps on the first non
					// colorcode char, but our for-loop will still do "pos++;")
					pos--;
				}
			} break;
			case ColorResetIndicator: {
				if (!colorcodes->empty()) {
					colorcode* cc = &colorcodes->back();

					if (cc->pos != numChar) {
						colorcodes->push_back(colorcode());
						cc = &colorcodes->back();
						cc->pos = numChar;
					}

					cc->resetColor = true;
				}
			} break;

			// newlines
			case 0x0d: // CR+LF
				pos += (pos + 1 < length && text[pos+1] == 0x0a);
			case 0x0a: // LF
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

			// printable chars
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


void CTextWrap::RemergeColorCodes(std::list<word>* words, std::list<colorcode>& colorcodes) const
{
	auto wi = words->begin();
	auto wi2 = words->begin();
	for (auto& c: colorcodes) {
		while(wi != words->end() && wi->pos <= c.pos) {
			wi2 = wi;
			++wi;
		}

		word wc;
		wc.pos = c.pos;
		wc.isColorCode = true;
		wc.text = toustring(c.tostring());

		if (wi2->isSpace || wi2->isLineBreak) {
			while(wi2 != words->end() && (wi2->isSpace || wi2->isLineBreak))
				++wi2;

			if (wi2 == words->end() || (wi == words->end() && (wi2->pos + wi2->numSpaces) < c.pos))
				return;

			wi2 = words->insert(wi2, wc);
		} else {
			if (wi == words->end() && (wi2->pos + wi2->text.size()) < (c.pos + 1)) {
				return;
			}

			size_t pos = c.pos - wi2->pos;
			if (pos < wi2->text.size() && pos > 0) {
				word w2;
				w2.text = toustring(wi2->text.substr(0,pos));
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


int CTextWrap::WrapInPlace(std::u8string& text, float _fontSize, float maxWidth, float maxHeight)
{
	// TODO make an option to insert '-' for word wrappings (and perhaps try to syllabificate)

	if (_fontSize <= 0.0f)
		_fontSize = GetSize();

	const float maxWidthf  = maxWidth / _fontSize;
	const float maxHeightf = maxHeight / _fontSize;

	// includes the empty string
	constexpr size_t numSpaceStrings = sizeof(spaceStringTable) / sizeof(spaceStringTable[0]);

	std::list<word> words;
	std::list<colorcode> colorcodes;

	SplitTextInWords(text, &words, &colorcodes);
	WrapTextConsole(words, maxWidthf, maxHeightf);
	//WrapTextKnuth(&lines, words, maxWidthf, maxHeightf);
	RemergeColorCodes(&words, colorcodes);

	// create the wrapped string
	text.clear();
	text.reserve(words.size());

	if (words.empty())
		return 0;

	unsigned int numlines = 1;

	for (const auto& w: words) {
		if (w.isSpace) {
			if (w.numSpaces < numSpaceStrings) {
				text.append(spaceStringTable[w.numSpaces]);
			} else {
				text.append(spaceStringTable[numSpaceStrings - 1]);
				text.append(w.numSpaces - (numSpaceStrings - 1), ' ');
			}
		} else if (w.isLineBreak) {
			text.append("\x0d\x0a");
			numlines++;
		} else {
			text.append(w.text);
		}
	}

	return numlines;
}


std::u8string CTextWrap::Wrap(const std::u8string& text, float _fontSize, float maxWidth, float maxHeight)
{
	std::u8string out(text);
	WrapInPlace(out, _fontSize, maxWidth, maxHeight);
	return out;
}

/*******************************************************************************/
/*******************************************************************************/
