#ifndef KAIK_LOGGER_HDR
#define KAIK_LOGGER_HDR

#include <string>
#include <fstream>

class IAICallback;

enum LogLevel {
	LOG_BASIC,
	LOG_DEBUG,
};

class CLogger {
	public:
		CLogger(IAICallback* cb) {
			icb  = cb;
			name = GetLogName();

			log.open(name.c_str());
		}
		~CLogger() {
			log.flush();
			log.close();
		}

		std::string GetLogName() const;

		template<typename T> CLogger& Log(const T& t, LogLevel lvl = LOG_BASIC) {
			switch (lvl) {
				case LOG_BASIC: {
					log << t; log << std::endl;
				} break;
				case LOG_DEBUG: {
					/* TODO */
				} break;
				default: {
				} break;
			}

			return *this;
		}

	private:
		IAICallback* icb;
		std::string name;
		std::ofstream log;
};

#endif
