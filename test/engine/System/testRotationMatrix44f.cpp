/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <vector>
#include "System/Matrix44f.h"
#include "System/Log/ILog.h"

#define BOOST_TEST_MODULE RotationMatrix44f
#include <boost/test/unit_test.hpp>

// FIXME: Add to float3.h?
namespace std
{
  ostream& operator<<(ostream& s, float3 const& f) {
    s << "x:" << f.x << ", y:" << f.y << " z:" << f.z;
    return s;
  }
}

BOOST_AUTO_TEST_CASE( GetEulerAnglesRgtHand )
{
	const std::vector<float3> testAngles = {
		//Order is PYR, in degrees because it's easier to understand
		float3(0, 0, 0),
		float3(30, 30, 30),
		float3(45, 45, 45),
		float3(0, 83, 0), //breaks
		float3(0, 265, 0), //breaks
		// not wrong but order isn't ideal
		//float3(0, 90, 50), // produces float3(-50, 90, 0)
		//float(140, 120, 45), // produces float3(-40, 60, -135)
	};
	CMatrix44f m;
	for (int i = 0; i < testAngles.size(); i++) {
		float3 origAngles = testAngles[i];
		m.LoadIdentity();
		m.RotateEulerXYZ(float3(origAngles.x / 180 * PI, origAngles.y / 180 * PI, origAngles.z / 180 * PI));
		float3 resultAngles = m.GetEulerAnglesLftHand();
		resultAngles.x = resultAngles.x * 180 / PI;
		resultAngles.y = resultAngles.y * 180 / PI;
		resultAngles.z = resultAngles.z * 180 / PI;
		BOOST_CHECK_EQUAL(origAngles, resultAngles);
	}
}