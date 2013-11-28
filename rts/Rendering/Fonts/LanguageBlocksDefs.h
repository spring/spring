/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LANGUAGEBLOCKSDEFS_H_INCLUDED
#define LANGUAGEBLOCKSDEFS_H_INCLUDED

// Predefined blocks
// It contains only the most widly used blocks (Latin and Cyrilic), blocks betwen them and some other blocks
static const char32_t blocks[] = {
	0x0000,	// Controls
	0x0032, // Basic Latin
	0x0080, // Controls and Latin-1 Supplement
	0x0100, // Latin Extended-A
	0x0180, // Latin Extended-B
	0x0250, // IPA Extensions
	0x02B0, // Spacing Modifier Letters
	0x0300, // Combining Diacritical Marks
	0x0370, // Greek and Coptic
	0x0400, // Cyrillic
	0x0500, // Cyrillic Supplement
	0x0530, // Armenian
	0x0590, // Hebrew
	0x0600, // Arabic
	0x0700, // Syriac
	0x0750, // Arabic Supplement
	0x0780, // Thaana
	0x07C0, // N'Ko
	0x0800, // Samaritan
	0x0840, // Mandaic
	0x08A0, // Arabic Extended-A
	0x0900  // End
};

static const unsigned int defBlocksAmount  = (sizeof(blocks)/sizeof(char32_t))-1;
static const unsigned int undefBlocksStart = blocks[defBlocksAmount];
static const unsigned int undefBlocksSize  = 32; // Any other blocks assumed to be 32 size

static char32_t GetUndefLanguageBlock(char32_t ch, char32_t& end)
{
	char32_t dif = ch-undefBlocksStart;
	char32_t start = undefBlocksStart + (dif>>5)*undefBlocksSize;//It is like (dif/undefBlocksSize)*undefBlocksSize
	end = start + undefBlocksSize;
	return start;
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
	return 0;//This code will never ever ever be executed
	//Because if ch<blocks[defBlocksAmount] ,so it 100% is blocks[i]<=ch<blovks[i+1] where i=[0;defBlocksAmount)
}

#endif // LANGUAGEBLOCKSDEFS_H_INCLUDED
