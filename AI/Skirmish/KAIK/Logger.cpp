#include <ctime>
#include <sstream>

#include "Logger.h"
#include "Defines.h"
#include "Util.h"
#include "IncExternAI.h"

std::string CLogger::GetLogName() const {
	if (name.size() > 0) {
		return name;
	}

	time_t now1;
	time(&now1);
	struct tm* now2 = localtime(&now1);

	std::stringstream ss;
		ss << std::string(LOGFOLDER);
		ss << icb->GetMapName();
		ss << "_";
		ss << icb->GetModName();
		ss << "_";
		ss << now2->tm_mon + 1;
		ss << "-";
		ss << now2->tm_mday;
		ss << "-";
		ss << now2->tm_year + 1900;
		ss << "_";
		ss << now2->tm_hour;
		ss << now2->tm_min;
		ss << "_team";
		ss << icb->GetMyTeam();
		ss << ".txt";

	std::string relName = ss.str();
	std::string absName = AIUtil::GetAbsFileName(icb, relName);

	return absName;
}
