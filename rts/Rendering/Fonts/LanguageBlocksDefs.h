/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LANGUAGEBLOCKSDEFS_H_INCLUDED
#define LANGUAGEBLOCKSDEFS_H_INCLUDED

#include <tuple>
#include <string>

#include "System/UnorderedMap.hpp"

#ifndef HEADLESS
// Predefined blocks
// It contains only the most widly used blocks (Latin and Cyrilic)
static const spring::unordered_map<std::string, std::tuple<char32_t, char32_t>> blocks {
	{"NULL",        std::make_tuple(0x0000, 0x0001)},
	{"ASCII",       std::make_tuple(0x0020, 0x007F)},
	{"Latin-1",     std::make_tuple(0x00A1, 0x0100)},
	{"Latin-A",     std::make_tuple(0x0100, 0x0180)},
	{"Latin-B",     std::make_tuple(0x0180, 0x0250)},
	{"Greek",       std::make_tuple(0x0370, 0x0400)},
	{"Cyrillic",    std::make_tuple(0x0400, 0x0500)},
	{"Hebrew",      std::make_tuple(0x0590, 0x0600)},
	{"Arabic",      std::make_tuple(0x0600, 0x0700)},
	{"FigureSpace", std::make_tuple(0x2007, 0x2008)}, // used by TextWrap
	{"Ellipsis",    std::make_tuple(0x2026, 0x2027)}, // ''
};

static const unsigned int undefBlocksSize = 32; // Any other blocks assumed to be 32 size

static bool IsInRange(char32_t ch, std::tuple<char32_t, char32_t>& range)
{
	const auto start = std::get<0>(range);
	const auto end   = std::get<1>(range);
	return (ch < end && ch >= start);
}

static char32_t GetUndefLanguageBlock(char32_t ch, char32_t& end)
{
	char32_t start = (ch/undefBlocksSize)*undefBlocksSize;
	end = start + undefBlocksSize;
	return start;
}


static char32_t GetLanguageBlock(char32_t ch, char32_t& end)
{
	for (auto it: blocks) {
		if (IsInRange(ch, it.second)) {
			end = std::get<1>(it.second);
			return std::get<0>(it.second);
		}
	}

	return GetUndefLanguageBlock(ch, end);
}
#endif

#endif // LANGUAGEBLOCKSDEFS_H_INCLUDED
