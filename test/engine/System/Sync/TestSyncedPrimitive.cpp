#ifndef SYNCCHECK
	#error "This test requires SYNCCHECK to be defined on the compiler command line."
#endif
#include "System/Sync/SyncedPrimitive.h"

#define BOOST_TEST_MODULE SyncedPrimitive
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(ImplicitConversions)
{
	// NOTE: this is a compile time test only

	ENTER_SYNCED_CODE();

	SyncedSshort ss = 4;
	SyncedSchar sl = 3;

	(void)(ss + sl);

	SyncedFloat sf = 3.14;
	SyncedSint si = 17;
	SyncedSint si2 = 18;

	(void)(sf * si);
	(void)(si == sf);
	(void)(si == 16);
	(void)(14 <= si2);

	// these aren't supposed to work
	//std::min(sf, si);
	//std::max(si, 18);
	//std::max(1.0f, 3);

	// these are supposed to work
	std::min<float>(sf, si);
	std::max<int>(si, 18);
	std::max<int>(si, si2);

	LEAVE_SYNCED_CODE();
}
