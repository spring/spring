#ifndef LANGUAGEBLOCKSDEFS_H_INCLUDED
#define LANGUAGEBLOCKSDEFS_H_INCLUDED

//! Predefined blocks
//! It contains only the most widly used blocks (Latin and Cyrilic) and blocks betwen them
const char32_t blocks[]={
				0x0000,	//Controls and Basic Latin
				0x0080,	//Controls and Latin-1 Supplement
				0x0100,	//Latin Extended-A
				0x0180,	//Latin Extended-B
				0x0250,	//IPA Extensions
				0x02B0, //Spacing Modifier Letters
				0x0300,	//Combining Diacritical Marks
				0x0370,	//Greek and Coptic
				0x0400,	//Cyrillic
				0x0500,	//Cyrillic Supplement
				0x0530, //Cyrillic Supplement end
			};

const unsigned int defBlocksAmount=(sizeof(blocks)/sizeof(char32_t))-1;
//! Any other blocks assumed to be 128 size
const unsigned int undefBlocksStart=blocks[defBlocksAmount];
const unsigned int undefBlocksSize=128;

char32_t GetUndefLanguageBlock(char32_t ch,char32_t& end)
{
	char32_t dif=ch-undefBlocksStart;
	//(dif/128)*128
	char32_t start=undefBlocksStart+(dif>>7)*undefBlocksSize;
	end=start+undefBlocksSize;
	return start;
}

char32_t GetLanguageBlock(char32_t ch,char32_t& end)
{
	if(ch>=undefBlocksStart)
		return GetUndefLanguageBlock(ch,end);

	for(int i=0;i<defBlocksAmount;i++)
	{
		if(blocks[i]<=ch && ch<blocks[i+1])
		{
			end=blocks[i+1];
			return blocks[i];
		}
	}
}

#endif // LANGUAGEBLOCKSDEFS_H_INCLUDED
