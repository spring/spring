#include <cassert>
#include <string>

#include "Util.h"
#include "IncExternAI.h"

namespace AIUtil {
	std::string GetAbsFileName(IAICallback* icb, const std::string& relFileName) {
		char        dst[2048] = {0};
		const char* src       = relFileName.c_str();
		const int   len       = relFileName.size();

		// last char ('\0') in dst
		// should not be overwritten
		assert(len < (2048 - 1));

		memcpy(dst, src, len);

		// get the absolute path to the file
		// (and create folders along the way)
		icb->GetValue(AIVAL_LOCATE_FILE_W, dst);

		return (std::string(dst));
	}

	bool IsFSGoodChar(const char c) {
		if ((c >= '0') && (c <= '9')) {
			return true;
		} else if ((c >= 'a') && (c <= 'z')) {
			return true;
		} else if ((c >= 'A') && (c <= 'Z')) {
			return true;
		} else if ((c == '.') || (c == '_') || (c == '-')) {
			return true;
		}

		return false;
	}
	std::string MakeFileSystemCompatible(const std::string& str) {
		std::string cleaned = str;

		for (std::string::size_type i=0; i < cleaned.size(); i++) {
			if (!IsFSGoodChar(cleaned[i])) {
				cleaned[i] = '_';
			}
		}

		return cleaned;
	}

	void StringToLowerInPlace(std::string& s) {
		std::transform(s.begin(), s.end(), s.begin(), (int (*)(int))tolower);
	}
	std::string StringToLower(std::string s) {
		StringToLowerInPlace(s);
		return s;
	}
}
