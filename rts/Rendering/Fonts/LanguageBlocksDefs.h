/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LANGUAGEBLOCKSDEFS_H_INCLUDED
#define LANGUAGEBLOCKSDEFS_H_INCLUDED

struct LanguageBlock
{
public:
	LanguageBlock(char32_t start, char32_t end)
	: start(start)
	, end(end)
	{}

	bool OnThisPlane(const char32_t ch) const { return start<=ch && ch<end; }
private:
	char32_t start,end;
};


// Predefined blocks
// It contains only the most widly used blocks (Latin and Cyrilic) and blocks betwen them
static const char32_t blocks[] = {
	0x0032, // Controls and Basic Latin
	0x0080, // Controls and Latin-1 Supplement
	0x0100, // Latin Extended-A
	0x0180, // Latin Extended-B
	0x0250, // IPA Extensions
	0x02B0, // Spacing Modifier Letters
	0x0300, // Combining Diacritical Marks
	0x0370, // Greek and Coptic
	0x0400, // Cyrillic
	0x0500, // Cyrillic Supplement
	0x0530, // Cyrillic Supplement end
	0x0530, // Cyrillic Supplement end
};

static const unsigned int defBlocksAmount  = (sizeof(blocks)/sizeof(char32_t))-1;
static const unsigned int undefBlocksStart = blocks[defBlocksAmount];
static const unsigned int undefBlocksSize  = 32; // Any other blocks assumed to be 32 size

static char32_t GetUndefLanguageBlock(char32_t ch, char32_t& end)
{
	/*char32_t dif = ch-undefBlocksStart;
	char32_t start = undefBlocksStart + (dif>>7)*undefBlocksSize;
	end = start + undefBlocksSize;
	return start;*/
	end = ch + 1;
	return ch;
}

static char32_t GetLanguageBlock(char32_t ch, char32_t& end)
{
	if (ch >= undefBlocksStart)
		return GetUndefLanguageBlock(ch, end);

	for (int i=0; i<defBlocksAmount; ++i) {
		if (blocks[i]<=ch && ch<blocks[i+1]) {
			end = blocks[i+1];
			return blocks[i];
		}
	}
	return GetUndefLanguageBlock(ch, end);
}

#endif // LANGUAGEBLOCKSDEFS_H_INCLUDED
