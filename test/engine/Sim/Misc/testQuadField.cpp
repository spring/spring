/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Sim/Misc/QuadField.h"
#include "System/float3.h"
#include "System/SpringMath.h"
#include <stdlib.h>
#include <time.h>

#define CATCH_CONFIG_MAIN
#include "lib/catch.hpp"

static inline float randf()
{
	return rand() / float(RAND_MAX);
}



TEST_CASE("QuadField")
{
	srand( time(nullptr) );

	static constexpr int WIDTH  = 3;
	static constexpr int HEIGHT = 3;
	static constexpr int TEST_RUNS = 50000;

	quadField.Init(int2(WIDTH, HEIGHT), SQUARE_SIZE);

	int bitmap[WIDTH * HEIGHT];

	bool fail = false;

	for (int n = 0; n < TEST_RUNS; ++n) {
		// clear
		for (int i = 0; i < WIDTH * HEIGHT; ++i) {
			bitmap[i] = 0;
		}

		// generate ray
		float3 start;
			start.x = randf() *  WIDTH * 3 - WIDTH;
			start.z = randf() * HEIGHT * 3 - HEIGHT;
		float3 dir;
			dir.x = randf() - 0.5f;
			dir.z = randf() - 0.5f;
			dir.x *= (randf() < 0.2f) ? 0 : 1; // special case #1: dir.x/z == 0 are a special cases
			dir.z *= (randf() < 0.2f) ? 0 : 1; // in GetQuadsOnRay(). We want to check those codes, too
			dir.SafeNormalize();
		float length = randf() * (WIDTH + HEIGHT) * 0.5f;
			length = (randf() < 0.2f) ? randf() : length; // special case #1: when start & end are in the same quad

		// #1: very naive way to raytrace (step-based interpolation)
		auto plot = [&](float3 pos) {
			const int x = Clamp<int>(pos.x, 0,  WIDTH - 1);
			const int z = Clamp<int>(pos.z, 0, HEIGHT - 1);
			const int idx = z * WIDTH + x;
			bitmap[idx] |= 1;
		};
		for (float l = 0.f; l <= length; l+=0.001f) {
			float3 pos = start + dir * l;
			plot(pos);
		}
		plot(start);
		plot(start + dir * length);

		// #2: raytrace via QuadField
		{
			QuadFieldQuery qfQuery;
			quadField.GetQuadsOnRay(qfQuery, start * SQUARE_SIZE, dir, length * SQUARE_SIZE);
			assert( std::adjacent_find(qfQuery.quads->begin(), qfQuery.quads->end()) == qfQuery.quads->end() ); // check for duplicates
			for (int qi: *qfQuery.quads) {
				bitmap[qi] |= 2;
			}
		}

		// check if #1 & #2 iterate same quads
		for (int i = 0; i < WIDTH * HEIGHT; ++i) {
			if (bitmap[i] == 0 || bitmap[i] == 3)
				continue;

			// only break when GetQuadsOnRay() returned LESS quads than the naive solution
			// cause the naive solution uses step-based interpolation, and so it might jump
			// over some edges when the ray is very close to the corner intersection (of 4 quads)
			bool isAdditionalQuad = (bitmap[i] == 2);
			if (!isAdditionalQuad) {
				int* istart  = reinterpret_cast<int*>(&start.x);
				int* idir    = reinterpret_cast<int*>(&dir.x);
				int* ilength = reinterpret_cast<int*>(&length);

				printf("start: 0x%X 0x%X 0x%X dir: 0x%X 0x%X 0x%X length: 0x%X\n", istart[0], istart[1], istart[2], idir[0], idir[1], idir[2], *ilength);
				fail = true;
				break;
			}
		}

		if (fail) break;
	}

	INFO("Too little quads returned!");
	CHECK_FALSE(fail);
}
