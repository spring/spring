#ifndef KAIK_LOGGER_HDR
#define KAIK_LOGGER_HDR

#include <string>
#include <fstream>

class IAICallback;

class CLogger {
	public:
		CLogger(IAICallback* cb) {
			icb  = cb;
			name = GetLogName();
			log  = new std::ofstream(name.c_str());
		}
		~CLogger() {
			log->close();
			delete log;
		}

		const std::string& GetLogName();

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
		IAICallback* icb;
		std::string name;
		std::ofstream* log;
};

#endif
