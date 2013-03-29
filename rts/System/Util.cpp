/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Util.h"
#if defined(_MSC_VER) && (_MSC_VER >= 1310)
#include <intrin.h>
#endif
#include <cstring>

std::string StringReplace(const std::string& text,
                          const std::string& from,
                          const std::string& to)
{
	std::string working = text;
	std::string::size_type pos = 0;
	while (true) {
		pos = working.find(from, pos);
		if (pos == std::string::npos) {
			break;
		}
		std::string tmp = working.substr(0, pos);
		tmp += to;
		tmp += working.substr(pos + from.size(), std::string::npos);
		pos += to.size();
		working = tmp;
	}
	return working;
}

/// @see http://www.codeproject.com/KB/stl/stdstringtrim.aspx
void StringTrimInPlace(std::string& str)
{
	static const std::string whiteSpaces(" \t\n\r");
	std::string::size_type pos = str.find_last_not_of(whiteSpaces);
	if (pos != std::string::npos) {
		str.erase(pos + 1);
		pos = str.find_first_not_of(whiteSpaces);
		if (pos != std::string::npos) {
			str.erase(0, pos);
		}
	} else {
		str.erase(str.begin(), str.end());
	}
}

std::string StringTrim(const std::string& str)
{
	std::string copy(str);
	StringTrimInPlace(copy);
	return copy;
}

bool StringToBool(std::string str)
{
	bool value = true;

	StringTrimInPlace(str);
	StringToLowerInPlace(str);

	// regex would probably be more appropriate,
	// but it is better not to rely on any external lib here
	if (
			(str.empty())    ||
			(str == "0")     ||
			(str == "n")     ||
			(str == "no")    ||
			(str == "f")     ||
			(str == "false") ||
			(str == "off")
		) {
		value = false;
	}

	return value;
}

bool StringStartsWith(const std::string& str, const char* prefix)
{
	if ((prefix == NULL) || (str.size() < strlen(prefix))) {
		return false;
	} else {
		return (str.compare(0, strlen(prefix), prefix) == 0);
	}
}

bool StringEndsWith(const std::string& str, const char* postfix)
{
	if ((postfix == NULL) || (str.size() < strlen(postfix))) {
		return false;
	} else {
		return (str.compare(str.size() - strlen(postfix), str.size(), postfix) == 0);
	}
}
