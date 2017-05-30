
#include "System/Log/ILog.h"
#include "System/Log/FileSink.h"
#include "System/Log/StreamSink.h"
#include "System/Log/LogUtil.h"
#include "System/StringUtil.h" // IntToString() -> header only

#define BOOST_TEST_MODULE ILog
#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>
using boost::test_tools::output_test_stream;

#include <cstdarg>



#define LOG_SECTION_EMPTY      ""
#define LOG_SECTION_DEFINED    "defined-section"
#define LOG_SECTION_ONE_TIME_0 "one-time-section"
#define LOG_SECTION_ONE_TIME_1 "other-one-time-section"

LOG_REGISTER_SECTION(LOG_SECTION_EMPTY)
LOG_REGISTER_SECTION(LOG_SECTION_DEFINED)
LOG_REGISTER_SECTION(LOG_SECTION_ONE_TIME_0)
// test what happens if not registered
//LOG_REGISTER_SECTION(LOG_SECTION_ONE_TIME_1)

namespace {
	struct LogStream {
		LogStream() {
			logFile = GetTempLogFile();
			printf("\tNOTE: logging to temporary log file: %s\n", logFile.c_str());
			log_file_addLogFile(logFile.c_str());
			log_sink_stream_setLogStream(&logStream);
		}
		~LogStream() {
			log_sink_stream_setLogStream(NULL);
			log_file_removeLogFile(logFile.c_str());
			remove(logFile.c_str());
		}

		std::string GetTempLogFile() {

			char* tmpName = tmpnam(NULL);
			BOOST_REQUIRE_MESSAGE((tmpName != NULL),
					"Failed to fetch a temporary log file name");
			return tmpName;
		}

		output_test_stream logStream;
		std::string logFile;
	};
}

static inline void test_log_sl(int line, output_test_stream& logStream,
		bool enabled, const char* section, int level, const char* fmt, ...)
{
	if (enabled) {
		char logMessage[64 + 1024];
		va_list arguments;
		va_start(arguments, fmt);
		vsnprintf(logMessage, sizeof(logMessage), fmt, arguments);
		va_end(arguments);
		std::string expected = std::string()
				+ (!LOG_SECTION_IS_DEFAULT(section) ? (std::string("[") + std::string(section) + "] ") : "")
				+ ((level != LOG_LEVEL_INFO) ? (std::string(log_util_levelToString(level)) + ": ") : "")
				+ logMessage
				+ "\n";
		BOOST_CHECK_MESSAGE(logStream.is_equal(expected.c_str()),
				"line(" << line << ") Expected: \"" << expected.c_str() << "\"");
	} else {
		BOOST_CHECK_MESSAGE(logStream.is_empty(),
				"line(" << line << ") Expected: \"\"");
	}
}

#define TLOG_SL(section, level, fmt, ...) \
	test_log_sl(__LINE__, logStream, LOG_IS_ENABLED_S(section, level), section, LOG_LEVE##level, fmt, ##__VA_ARGS__)

#define TLOG_S(section, fmt, ...) \
	TLOG_SL(section, L_INFO, fmt, ##__VA_ARGS__)

#define TLOG_L(level, fmt, ...) \
	TLOG_SL(LOG_SECTION_DEFAULT, level, fmt, ##__VA_ARGS__)

#define TLOG(fmt, ...) \
	TLOG_SL(LOG_SECTION_DEFAULT, L_INFO, fmt, ##__VA_ARGS__)

namespace {
	class PrePostMainLogTest {
	public:
		PrePostMainLogTest() {

			//output_test_stream logStream;
			//log_sink_stream_setLogStream(&logStream);
			LOG( "static ctor log test");
			// We can not use Boost Test here, cause it is not yet initialized,
			// so we depend on asserts spread throught the logging system.
			//TLOG("static ctor log test");
			//log_sink_stream_setLogStream(NULL);
		}

		~PrePostMainLogTest() {

			//output_test_stream logStream;
			//log_sink_stream_setLogStream(&logStream);
			LOG( "static dtor log test");
			// We can not use Boost Test here, cause it is already
			// uninitialized, so we depend on asserts spread throught the
			// logging system.
			//TLOG("static dtor log test");
			//log_sink_stream_setLogStream(NULL);
		}
	};
}


BOOST_FIXTURE_TEST_SUITE(everything, LogStream)

// test if logging works very early & very late in program live-time
static PrePostMainLogTest prePostMainLogTest;


BOOST_AUTO_TEST_CASE(Default)
{
	LOG( "Testing default logging level (INFO)");
	TLOG("Testing default logging level (INFO)");
}


BOOST_AUTO_TEST_CASE(Levels)
{
	LOG_L( L_ERROR, "Static min log level is: %i", _LOG_LEVEL_MIN);
	TLOG_L(L_ERROR, "Static min log level is: %i", _LOG_LEVEL_MIN);

#define TEST_LOG_LEVEL(level) \
	LOG_L( level, "Testing logging level: (%i) %s", LOG_LEVE##level, #level); \
	TLOG_L(level, "Testing logging level: (%i) %s", LOG_LEVE##level, #level)

	TEST_LOG_LEVEL(L_DEBUG);
	TEST_LOG_LEVEL(L_INFO);
	TEST_LOG_LEVEL(L_WARNING);
	TEST_LOG_LEVEL(L_ERROR);
	TEST_LOG_LEVEL(L_FATAL);

#undef TEST_LOG_LEVEL
}


BOOST_AUTO_TEST_CASE(IsSingleInstruction)
{
	// do NOT add braces here, as that would invalidate the test!
	int x = 0;
	if      (x == 0) {
		LOG("(IsSingleInstruction) Test");
	} else if (x == 1) {
		LOG("(IsSingleInstruction) if");
	} else {
		LOG("(IsSingleInstruction) LOG() is a single instruction.");
	}

	TLOG("(IsSingleInstruction) Test");
}


static bool TestDefaultSection1()
{
	return (LOG_SECTION_EQUAL("", LOG_SECTION_DEFAULT));
}

static bool TestDefaultSection2()
{
	const std::string foo = "";
	return (LOG_SECTION_EQUAL(foo.c_str(), LOG_SECTION_DEFAULT));
}


BOOST_AUTO_TEST_CASE(Sections)
{
	LOG( "Testing logging section: <default> (level: default)");
	TLOG("Testing logging section: <default> (level: default)");

	BOOST_CHECK(TestDefaultSection1());
	BOOST_CHECK(TestDefaultSection2());

	LOG_L( L_INFO, "Testing logging section: <default> (level: info)");
	TLOG_L(L_INFO, "Testing logging section: <default> (level: info)");

	#undef  LOG_SECTION_CURRENT
	#define LOG_SECTION_CURRENT LOG_SECTION_EMPTY
	LOG_L(                     L_DEBUG, "Testing logging section: \"" LOG_SECTION_EMPTY "\"");
	TLOG_SL(LOG_SECTION_EMPTY, L_DEBUG, "Testing logging section: \"" LOG_SECTION_EMPTY "\"");

	#undef  LOG_SECTION_CURRENT
	#define LOG_SECTION_CURRENT LOG_SECTION_DEFINED
	LOG_L(                       L_DEBUG, "Testing logging section: \"" LOG_SECTION_DEFINED "\"");
	TLOG_SL(LOG_SECTION_DEFINED, L_DEBUG, "Testing logging section: \"" LOG_SECTION_DEFINED "\"");

	LOG_S( LOG_SECTION_ONE_TIME_0, "Testing logging section: <temporary-section> (level: default)");
	TLOG_S(LOG_SECTION_ONE_TIME_0, "Testing logging section: <temporary-section> (level: default)");

	LOG_SL( LOG_SECTION_ONE_TIME_1, L_INFO, "Testing logging section: <temporary-section> (level: INFO)");
	TLOG_SL(LOG_SECTION_ONE_TIME_1, L_INFO, "Testing logging section: <temporary-section> (level: INFO)");

	#undef  LOG_SECTION_CURRENT
	#define LOG_SECTION_CURRENT LOG_SECTION_DEFAULT
}


BOOST_AUTO_TEST_CASE(IsEnabled)
{
	if (LOG_IS_ENABLED_STATIC(L_DEBUG)) {
		// *do heavy, log-output related processing here*
		LOG_L( L_DEBUG, "Testing LOG_IS_ENABLED_STATIC");
		TLOG_L(L_DEBUG, "Testing LOG_IS_ENABLED_STATIC");
	}

	if (LOG_IS_ENABLED(L_DEBUG)) {
		// *do heavy, log-output related processing here*
		LOG_L(L_DEBUG, "Testing LOG_IS_ENABLED");
	}
	TLOG_L(   L_DEBUG, "Testing LOG_IS_ENABLED");


	if (LOG_IS_ENABLED_STATIC_S("one-time-section", L_DEBUG)) {
		// *do heavy, log-output related processing here*
		LOG_SL( "one-time-section", L_DEBUG, "Testing LOG_IS_ENABLED_STATIC_S");
		TLOG_SL("one-time-section", L_DEBUG, "Testing LOG_IS_ENABLED_STATIC_S");
	}

	if (LOG_IS_ENABLED_S("other-one-time-section", L_DEBUG)) {
		// *do heavy, log-output related processing here*
		LOG_SL("other-one-time-section", L_DEBUG, "Testing LOG_IS_ENABLED_S");
	}
	TLOG_SL(   "other-one-time-section", L_DEBUG, "Testing LOG_IS_ENABLED_S");
}


BOOST_AUTO_TEST_SUITE_END()

