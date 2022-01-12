/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/float3.h"
#include "System/SpringMath.h"
#include <stdlib.h>
#include <time.h>

#define CATCH_CONFIG_MAIN
#include "lib/catch.hpp"


static inline float3 randfloat3()
{
	float3 r(rand() + 1.0f, rand() + 1.0f, rand() + 1.0f);
	//Disallow insane ellipsoids
	float minValue = std::fmax(r.x, std::max(r.y, r.z)) * 0.02;
	r.x = std::max(r.x, minValue);
	r.y = std::max(r.y, minValue);
	r.z = std::max(r.z, minValue);

	return r;
}

static inline float getdistSq(float x, float y, float z, float a, float b, float c, float theta, float phi) {
	const float cost = cos(theta);
	const float sint = sin(theta);
	const float sinp = sin(phi);
	const float cosp = cos(phi);
	const float fx = a * cosp * cost - x;
	const float fy = b * cosp * sint - y;
	const float fz = c * sinp - z;

	return fx * fx + fy * fy + fz * fz;
}


#define TEST_RUNS 100000
#define MAX_ITERATIONS 20
#define THRESHOLD 0.001
#define MAX_FAILS 5 //allow some fails

//We Fail if after half the iterations we have error > 5%
#define FAIL_THRESHOLD 0.05f

TEST_CASE("Ellipsoid")
{
	srand( time(NULL) );
	unsigned failCount = 0;

	float relError[MAX_ITERATIONS + 1] = {};
	float maxError[MAX_ITERATIONS + 1] = {};
	float tempdist[MAX_ITERATIONS + 1] = {};
	float  errorsq[MAX_ITERATIONS + 1] = {};

	unsigned finalIteration[MAX_ITERATIONS + 1] = {};

	for (int j = 0; j < TEST_RUNS; ++j) {
		float3 halfScales = randfloat3();
		float3 pv = randfloat3();
		const float& a = halfScales.x;
		const float& b = halfScales.y;
		const float& c = halfScales.z;
		const float& x = pv.x;
		const float& y = pv.y;
		const float& z = pv.z;
		float weightedLengthSq = (x * x) / (a * a)  + (y * y) / (b * b) + (z * z) / (c * c);
		while (weightedLengthSq <= 1.0f) {
			halfScales = randfloat3();
			pv = randfloat3();
			weightedLengthSq = (x * x) / (a * a)  + (y * y) / (b * b) + (z * z) / (c * c);
		}

		const float a2 = a * a;
		const float b2 = b * b;
		const float c2 = c * c;
		const float x2 = x * x;
		const float y2 = y * y;
		const float a2b2 = a2 - b2;
		const float xa = x * a;
		const float yb = y * b;
		const float zc = z * c;


		//Initial guess
		float theta = math::atan2(a * y, b * x);
		float phi = math::atan2(z, c * math::sqrt(x2 / a2 + y2 / b2));


		//Iterations
		for (int i = 0; true; ) {
			const float cost = math::cos(theta);
			const float sint = math::sin(theta);
			const float sinp = math::sin(phi);
			const float cosp = math::cos(phi);
			const float sin2t = sint * sint;
			const float xacost_ybsint = xa * cost + yb * sint;
			const float xasint_ybcost = xa * sint - yb * cost;
			const float a2b2costsint = a2b2 * cost * sint;
			const float a2cos2t_b2sin2t_c2 = a2 * cost * cost + b2 * sin2t - c2;

			const float d1 = a2b2costsint * cosp - xasint_ybcost;
			const float d2 = a2cos2t_b2sin2t_c2 * sinp * cosp - sinp * xacost_ybsint + zc * cosp;


			{
				const float fx = a * cosp * cost - x;
				const float fy = b * cosp * sint - y;
				const float fz = c * sinp - z;

				tempdist[i] = math::sqrt(fx * fx + fy * fy + fz * fz);

				if (i > 1 && (std::abs(tempdist[i] - tempdist[i - 1]) < THRESHOLD * tempdist[i]))
					++finalIteration[i];

				if (i++ == MAX_ITERATIONS)
					break;
			}

			//Derivative matrix
			const float a11 = a2b2 * (1 - 2 * sin2t) * cosp - xacost_ybsint;
			const float a12 = -a2b2costsint * sinp;
			const float a21 = 2 * a12 * cosp + sinp * xasint_ybcost;
			const float a22 = a2cos2t_b2sin2t_c2 * (1 - 2 * sinp * sinp) - cosp * xacost_ybsint - zc;

			const float invDet = 1.0f / (a11 * a22 - a21 * a12);

			theta += (a12 * d2 - a22 * d1) * invDet;
			theta  = Clamp(theta, 0.0f, math::HALFPI);
			phi += (a21 * d1 - a11 * d2) * invDet;
			phi  = Clamp(phi, 0.0f, math::HALFPI);
		}

		bool failed = false;

		for (int i = 0; i < MAX_ITERATIONS; ++i) {
			//Relative error for every iteration
			const float tempError = std::abs(tempdist[i] - tempdist[MAX_ITERATIONS]) / tempdist[MAX_ITERATIONS];

			relError[i] += tempError;
			maxError[i] = std::max(tempError, maxError[i]);

			if (i > MAX_ITERATIONS / 2 && tempError > FAIL_THRESHOLD && !failed) {
				failed = true;
				++failCount;
				printf("x: %f, y: %f, z: %f, a: %f, b: %f, c: %f\n", x,y,z,a,b,c);

			}
			errorsq[i] += tempError * tempError;
		}
	}

	for (int i = 0; i < MAX_ITERATIONS; ++i) {
		const float meanError = relError[i] / TEST_RUNS;
		const float  devError = math::sqrt(errorsq[i] / TEST_RUNS - meanError * meanError);
		const float pct = 100.0f * (1.0f - float(finalIteration[i]) / TEST_RUNS);

		printf("Iteration %d:\n\tError: (Mean: %f, Dev: %f, Max: %f)\n\tPercent remaining: %.3f%%\n", i, meanError, devError, maxError[i], pct);
	}

	INFO("Inaccurate ellipsoid distance approximation!");
	CHECK(failCount < MAX_FAILS);
}
