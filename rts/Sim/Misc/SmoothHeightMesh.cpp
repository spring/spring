/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <vector>
#include <cassert>
#include <limits>

#include "SmoothHeightMesh.h"

#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "System/float3.h"
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"



SmoothHeightMesh smoothGround;


static float Interpolate(float x, float y, const int maxx, const int maxy, const float res, const float* heightmap)
{
	x = Clamp(x / res, 0.0f, maxx - 1.0f);
	y = Clamp(y / res, 0.0f, maxy - 1.0f);
	const int sx = x;
	const int sy = y;
	const float dx = (x - sx);
	const float dy = (y - sy);

	const int sxp1 = std::min(sx + 1, maxx - 1);
	const int syp1 = std::min(sy + 1, maxy - 1);

	const float& h1 = heightmap[sx   + sy   * maxx];
	const float& h2 = heightmap[sxp1 + sy   * maxx];
	const float& h3 = heightmap[sx   + syp1 * maxx];
	const float& h4 = heightmap[sxp1 + syp1 * maxx];

	const float hi1 = mix(h1, h2, dx);
	const float hi2 = mix(h3, h4, dx);
	return mix(hi1, hi2, dy);
}


void SmoothHeightMesh::Init(float mx, float my, float res, float smoothRad)
{
	maxx = ((fmaxx = mx) / res) + 1;
	maxy = ((fmaxy = my) / res) + 1;

	resolution = res;
	smoothRadius = std::max(1.0f, smoothRad);

	MakeSmoothMesh();
}

void SmoothHeightMesh::Kill() {

	mesh.clear();
	origMesh.clear();
}



float SmoothHeightMesh::GetHeight(float x, float y)
{
	assert(!mesh.empty());
	return Interpolate(x, y, maxx, maxy, resolution, &mesh[0]);
}

float SmoothHeightMesh::GetHeightAboveWater(float x, float y)
{
	assert(!mesh.empty());
	return std::max(0.0f, Interpolate(x, y, maxx, maxy, resolution, &mesh[0]));
}



float SmoothHeightMesh::SetHeight(int index, float h)
{
	return (mesh[index] = h);
}

float SmoothHeightMesh::AddHeight(int index, float h)
{
	return (mesh[index] += h);
}

float SmoothHeightMesh::SetMaxHeight(int index, float h)
{
	return (mesh[index] = std::max(h, mesh[index]));
}



inline static void FindMaximumColumnHeights(
	const int maxx,
	const int maxy,
	const int winSize,
	const float resolution,
	std::vector<float>& colsMaxima,
	std::vector<int>& maximaRows
) {
	// initialize the algorithm: find the maximum
	// height per column and the corresponding row
	for (int y = 0; y <= std::min(maxy, winSize); ++y) {
		for (int x = 0; x <= maxx; ++x)  {
			const float curx = x * resolution;
			const float cury = y * resolution;
			const float curh = CGround::GetHeightAboveWater(curx, cury);

			if (curh > colsMaxima[x]) {
				colsMaxima[x] = curh;
				maximaRows[x] = y;
			}
		}
	}
}

inline static void AdvanceMaximaRows(
	const int y,
	const int maxx,
	const float resolution,
	const std::vector<float>& colsMaxima,
	      std::vector<int>& maximaRows
) {
	const float cury = y * resolution;

	// try to advance rows if they're equal to current maximum but are further away
	for (int x = 0; x <= maxx; ++x) {
		if (maximaRows[x] == (y - 1)) {
			const float curx = x * resolution;
			const float curh = CGround::GetHeightAboveWater(curx, cury);

			if (curh == colsMaxima[x]) {
				maximaRows[x] = y;
			}

			assert(curh <= colsMaxima[x]);
		}
	}
}



inline static void FindRadialMaximum(
	int y,
	int maxx,
	int winSize,
	float resolution,
	const std::vector<float>& colsMaxima,
	      std::vector<float>& mesh
) {
	const float cury = y * resolution;

	for (int x = 0; x <= maxx; ++x) {
		float maxRowHeight = -std::numeric_limits<float>::max();

		// find current maximum within radius smoothRadius
		// (in every column stack) along the current row
		const int startx = std::max(x - winSize, 0);
		const int endx = std::min(maxx, x + winSize);

		for (int i = startx; i <= endx; ++i) {
			assert(i >= 0);
			assert(i <= maxx);
			assert(CGround::GetHeightReal(i * resolution, cury) <= colsMaxima[i]);

			maxRowHeight = std::max(colsMaxima[i], maxRowHeight);
		}

#ifndef NDEBUG
		const float curx = x * resolution;
		assert(maxRowHeight <= std::max(readMap->GetCurrMaxHeight(), 0.0f));
		assert(maxRowHeight >= CGround::GetHeightAboveWater(curx, cury));

	#ifdef SMOOTHMESH_CORRECTNESS_CHECK
		// naive algorithm
		float maxRowHeightAlt = -std::numeric_limits<float>::max();

		for (float y1 = cury - smoothRadius; y1 <= cury + smoothRadius; y1 += resolution) {
			for (float x1 = curx - smoothRadius; x1 <= curx + smoothRadius; x1 += resolution) {
				maxRowHeightAlt = std::max(maxRowHeightAlt, CGround::GetHeightAboveWater(x1, y1));
			}
		}

		assert(maxRowHeightAlt == maxRowHeight);
	#endif
#endif

		mesh[x + y * maxx] = maxRowHeight;
	}
}



inline static void FixRemainingMaxima(
	const int y,
	const int maxx,
	const int maxy,
	const int winSize,
	const float resolution,
	std::vector<float>& colsMaxima,
	std::vector<int>& maximaRows
) {
	// fix remaining maximums after a pass
	const int nextrow = y + winSize + 1;
	const float nextrowy = nextrow * resolution;

	for (int x = 0; x <= maxx; ++x) {
#ifdef _DEBUG
		for (int y1 = std::max(0, y - winSize); y1 <= std::min(maxy, y + winSize); ++y1) {
			assert(CGround::GetHeightReal(x * resolution, y1 * resolution) <= colsMaxima[x]);
		}
#endif
		const float curx = x * resolution;

		if (maximaRows[x] <= (y - winSize)) {
			// find a new maximum if the old one left the window
			colsMaxima[x] = -std::numeric_limits<float>::max();

			for (int y1 = std::max(0, y - winSize + 1); y1 <= std::min(maxy, nextrow); ++y1) {
				const float h = CGround::GetHeightAboveWater(curx, y1 * resolution);

				if (h > colsMaxima[x]) {
					colsMaxima[x] = h;
					maximaRows[x] = y1;
				} else if (colsMaxima[x] == h) {
					// if equal, move as far down as possible
					maximaRows[x] = y1;
				}
			}
		} else if (nextrow <= maxy) {
			// else, just check if a new maximum has entered the window
			const float h = CGround::GetHeightAboveWater(curx, nextrowy);

			if (h > colsMaxima[x]) {
				colsMaxima[x] = h;
				maximaRows[x] = nextrow;
			}
		}

		assert(maximaRows[x] <= nextrow);
		assert(maximaRows[x] >= y - winSize + 1);

#ifdef _DEBUG
		for (int y1 = std::max(0, y - winSize + 1); y1 <= std::min(maxy, y + winSize + 1); ++y1) {
			assert(colsMaxima[x] >= CGround::GetHeightReal(curx, y1 * resolution));
		}
#endif
	}
}



inline static void BlurHorizontal(
	const int maxx,
	const int maxy,
	const int blurSize,
	const float resolution,
	const std::vector<float>& mesh,
	      std::vector<float>& smoothed
) {
	const float n = 2.0f * blurSize + 1.0f;
	const float recipn = 1.0f / n;
	const int lineSize = maxx + 1;

	for_mt(0, maxy+1, [&](const int y) {
		float avg = 0.0f;

		for (int x = 0; x <= 2 * blurSize; ++x) {
			avg += mesh[x + y * lineSize];
		}

		for (int x = 0; x <= maxx; ++x) {
			const int idx = x + y * lineSize;

			if (x <= blurSize || x > (maxx - blurSize)) {
				// map-border case
				smoothed[idx] = 0.0f;

				const int xstart = std::max(x - blurSize, 0);
				const int xend   = std::min(x + blurSize, maxx);

				for (int x1 = xstart; x1 <= xend; ++x1) {
					smoothed[idx] += mesh[x1 + y * lineSize];
				}

				const float gh = CGround::GetHeightAboveWater(x * resolution, y * resolution);
				const float sh = smoothed[idx] / (xend - xstart + 1);

				smoothed[idx] = std::min(readMap->GetCurrMaxHeight(), std::max(gh, sh));
			} else {
				// non-border case
				avg += mesh[idx + blurSize] - mesh[idx - blurSize - 1];

				const float gh = CGround::GetHeightAboveWater(x * resolution, y * resolution);
				const float sh = recipn * avg;

				smoothed[idx] = std::min(readMap->GetCurrMaxHeight(), std::max(gh, sh));
			}

			assert(smoothed[idx] <= std::max(readMap->GetCurrMaxHeight(), 0.0f));
			assert(smoothed[idx] >=          readMap->GetCurrMinHeight()       );
		}
	});
}

inline static void BlurVertical(
	const int maxx,
	const int maxy,
	const int blurSize,
	const float resolution,
	const std::vector<float>& mesh,
	      std::vector<float>& smoothed
) {
	const float n = 2.0f * blurSize + 1.0f;
	const float recipn = 1.0f / n;
	const int lineSize = maxx + 1;

	for_mt(0, maxx+1, [&](const int x) {
		float avg = 0.0f;

		for (int y = 0; y <= 2 * blurSize; ++y) {
			avg += mesh[x + y * lineSize];
		}

		for (int y = 0; y <= maxy; ++y) {
			const int idx = x + y * lineSize;

			if (y <= blurSize || y > (maxy - blurSize)) {
				// map-border case
				smoothed[idx] = 0.0f;

				const int ystart = std::max(y - blurSize, 0);
				const int yend   = std::min(y + blurSize, maxy);

				for (int y1 = ystart; y1 <= yend; ++y1) {
					smoothed[idx] += mesh[x + y1 * lineSize];
				}

				const float gh = CGround::GetHeightAboveWater(x * resolution, y * resolution);
				const float sh = smoothed[idx] / (yend - ystart + 1);

				smoothed[idx] = std::min(readMap->GetCurrMaxHeight(), std::max(gh, sh));
			} else {
				// non-border case
				avg += mesh[x + (y + blurSize) * lineSize] - mesh[x + (y - blurSize - 1) * lineSize];

				const float gh = CGround::GetHeightAboveWater(x * resolution, y * resolution);
				const float sh = recipn * avg;

				smoothed[idx] = std::min(readMap->GetCurrMaxHeight(), std::max(gh, sh));
			}

			assert(smoothed[idx] <= std::max(readMap->GetCurrMaxHeight(), 0.0f));
			assert(smoothed[idx] >=          readMap->GetCurrMinHeight()       );
		}
	});
}



inline static void CheckInvariants(
	int y,
	int maxx,
	int maxy,
	int winSize,
	float resolution,
	const std::vector<float>& colsMaxima,
	const std::vector<int>& maximaRows
) {
	// check invariants
	if (y < maxy) {
		for (int x = 0; x <= maxx; ++x) {
			assert(maximaRows[x] > y - winSize);
			assert(maximaRows[x] <= maxy);
			assert(colsMaxima[x] <= std::max(readMap->GetCurrMaxHeight(), 0.0f));
			assert(colsMaxima[x] >=          readMap->GetCurrMinHeight()       );
		}
	}
	for (int y1 = std::max(0, y - winSize + 1); y1 <= std::min(maxy, y + winSize + 1); ++y1) {
		for (int x1 = 0; x1 <= maxx; ++x1) {
			assert(CGround::GetHeightReal(x1 * resolution, y1 * resolution) <= colsMaxima[x1]);
		}
	}
}



void SmoothHeightMesh::MakeSmoothMesh()
{
	ScopedOnceTimer timer("SmoothHeightMesh::MakeSmoothMesh");

	// info:
	//   height-value array has size <maxx + 1> * <maxy + 1>
	//   and represents a grid of <maxx> cols by <maxy> rows
	//   maximum legal index is ((maxx + 1) * (maxy + 1)) - 1
	//
	//   row-width (number of height-value corners per row) is (maxx + 1)
	//   col-height (number of height-value corners per col) is (maxy + 1)
	//
	//   1st row has indices [maxx*(  0) + (  0), maxx*(1) + (  0)] inclusive
	//   2nd row has indices [maxx*(  1) + (  1), maxx*(2) + (  1)] inclusive
	//   3rd row has indices [maxx*(  2) + (  2), maxx*(3) + (  2)] inclusive
	//   ...
	//   Nth row has indices [maxx*(N-1) + (N-1), maxx*(N) + (N-1)] inclusive
	//
	// use sliding window of maximums to reduce computational complexity
	const     int winSize = smoothRadius / resolution;
	constexpr int blurSize = 3;

	assert(mesh.empty());
	mesh.resize((maxx + 1) * (maxy + 1), 0.0f);
	origMesh.resize((maxx + 1) * (maxy + 1), 0.0f);

	colsMaxima.clear();
	colsMaxima.resize(maxx + 1, -std::numeric_limits<float>::max());
	maximaRows.clear();
	maximaRows.resize(maxx + 1, -1);

	FindMaximumColumnHeights(maxx, maxy, winSize, resolution, colsMaxima, maximaRows);

	for (int y = 0; y <= maxy; ++y) {
		AdvanceMaximaRows(y, maxx, resolution, colsMaxima, maximaRows);
		FindRadialMaximum(y, maxx, winSize, resolution, colsMaxima, mesh);
		FixRemainingMaxima(y, maxx, maxy, winSize, resolution, colsMaxima, maximaRows);

#ifdef _DEBUG
		CheckInvariants(y, maxx, maxy, winSize, resolution, colsMaxima, maximaRows);
#endif
	}

	// actually smooth with approximate Gaussian blur passes
	for (int numBlurs = 3; numBlurs > 0; --numBlurs) {
		BlurHorizontal(maxx, maxy, blurSize, resolution, mesh, origMesh); mesh.swap(origMesh);
		BlurVertical(maxx, maxy, blurSize, resolution, mesh, origMesh); mesh.swap(origMesh);
	}

	// <mesh> now contains the final smoothed heightmap, save it in origMesh
	std::copy(mesh.begin(), mesh.end(), origMesh.begin());
}
