#include <cassert>
#include <string>

#include "Util.h"
#include "IncExternAI.h"

namespace AIUtil {
	std::string GetAbsFileName(IAICallback* cb, const std::string& relFileName) {
		char        dst[1024] = {0};
		const char* src       = relFileName.c_str();
		const int   len       = relFileName.size();

		// last char ('\0') in dst
		// should not be overwritten
		assert(len < (1024 - 1));

		memcpy(dst, src, len);

		// get the absolute path to the file
		// (and create folders along the way)
		cb->GetValue(AIVAL_LOCATE_FILE_W, dst);

		return (std::string(dst));
	}
}
