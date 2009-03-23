#ifndef KAIK_LOGGER_HDR
#define KAIK_LOGGER_HDR

#include <string>
#include <sstream>
#include <fstream>

class CLogger {
	public:
		CLogger(const std::string& fname) {
			log = new std::ofstream(fname.c_str());
		}
		~CLogger() {
			delete log;
		}

		CLogger& operator << (const char c) {
			*log << c; *log << std::endl; return *this;
		}
		CLogger& operator << (const std::string& s) {
			*log << s; *log << std::endl; return *this;
		}
		CLogger& operator << (const std::stringstream& ss) {
			*log << ss.str(); *log << std::endl; return *this;
		}

	private:
		std::ofstream* log;
};

#endif
