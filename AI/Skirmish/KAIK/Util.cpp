#include <string>

#include "Util.h"
#include "IncExternAI.h"

namespace AIUtil {
	std::string GetAbsFileName(IAICallback* cb, const std::string& relFileName) {
		static char buffer[1024] = {0};

		SNPRINTF(buffer, 1024 - 1, "%s", relFileName.c_str());

		// get the absolute path to the file
		// (and create folders along the way)
		cb->GetValue(AIVAL_LOCATE_FILE_W, buffer);

		return (std::string(buffer));
	}
}
