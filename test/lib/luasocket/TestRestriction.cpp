#include "lib/luasocket/src/restrictions.h"

#include <string>
#include <cstdio>

#define BOOST_TEST_MODULE TestRestriction
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE(TestRestriction)
{
	CLuaSocketRestrictions rest = CLuaSocketRestrictions();
	#define CheckAccess(result, rules, host, port) \
		BOOST_CHECK(rest.isAllowed((CLuaSocketRestrictions::RestrictType)rules, host, port) == result);
	#define AllowAccess(valid, rules, host, port) \
		if ( i == valid) { \
			CheckAccess(true, rules, host, port); \
		} else { \
			CheckAccess(false, rules, host, port); \
		}
	for(int i=0; i<CLuaSocketRestrictions::ALL_RULES; i++) {
		CheckAccess(false, i, "localhost", 80);
		CheckAccess(false, i, "localhost", -1);
		CheckAccess(false, i, "springrts.com", 80);
		CheckAccess(false, i, "springrts.com", -1);
		CheckAccess(false, i, "94.23.170.70", 80);
		CheckAccess(false, i, "94.23.170.70", -1);
		CheckAccess(false, i, "zero-k.info", 80);
		CheckAccess(false, i, "zero-k.info", -1);
	}

	rest.addRule(CLuaSocketRestrictions::TCP_CONNECT, "springrts.com", 80, true);

	for(int i=0; i<CLuaSocketRestrictions::ALL_RULES; i++) {
		CheckAccess(false, i, "localhost", 80);
		CheckAccess(false, i, "localhost", -1);
		AllowAccess(CLuaSocketRestrictions::TCP_CONNECT, i, "springrts.com", 80);
		AllowAccess(CLuaSocketRestrictions::TCP_CONNECT, i, "springrts.com", -1);
		CheckAccess(false, i, "94.23.170.70", 80);
		CheckAccess(false, i, "94.23.170.70", -1);
		CheckAccess(false, i, "zero-k.info", 80);
		CheckAccess(false, i, "zero-k.info", -1);
	}

	rest.addIP("springrts.com", "94.23.170.70");

	for(int i=0; i<CLuaSocketRestrictions::ALL_RULES; i++) {
		CheckAccess(false, i, "localhost", 80);
		CheckAccess(false, i, "localhost", -1);
		AllowAccess(CLuaSocketRestrictions::TCP_CONNECT, i, "springrts.com", 80);
		AllowAccess(CLuaSocketRestrictions::TCP_CONNECT, i, "springrts.com", -1);
		AllowAccess(CLuaSocketRestrictions::TCP_CONNECT, i, "94.23.170.70", 80);
		AllowAccess(CLuaSocketRestrictions::TCP_CONNECT, i, "94.23.170.70", -1);
		CheckAccess(false, i, "zero-k.info", 80);
		CheckAccess(false, i, "zero-k.info", -1);

	}

	rest.addRule(CLuaSocketRestrictions::UDP_LISTEN, "localhost", 80, true);

	for(int i=0; i<CLuaSocketRestrictions::ALL_RULES; i++) {
		AllowAccess(CLuaSocketRestrictions::UDP_LISTEN, i, "localhost", -1);
		AllowAccess(CLuaSocketRestrictions::UDP_LISTEN, i, "localhost", 80);
		AllowAccess(CLuaSocketRestrictions::TCP_CONNECT, i, "springrts.com", 80);
		AllowAccess(CLuaSocketRestrictions::TCP_CONNECT, i, "springrts.com", -1);
		AllowAccess(CLuaSocketRestrictions::TCP_CONNECT, i, "94.23.170.70", 80);
		AllowAccess(CLuaSocketRestrictions::TCP_CONNECT, i, "94.23.170.70", -1);
		CheckAccess(false, i, "zero-k.info", 80);
		CheckAccess(false, i, "zero-k.info", -1);
	}

}

