/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <vector>
#include <cassert>
#include <limits>

#include "SmoothHeightMesh.h"

#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "System/float3.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"
#include "System/ThreadPool.h"



SmoothHeightMesh* smoothGround = NULL;


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


SmoothHeightMesh::SmoothHeightMesh(const CGround* ground, float mx, float my, float res, float smoothRad)
	: maxx((mx / res) + 1)
	, maxy((my / res) + 1)
	, fmaxx(mx)
	, fmaxy(my)
	, resolution(res)
	, smoothRadius(std::max(1.0f, smoothRad))
{
	MakeSmoothMesh(ground);
}

SmoothHeightMesh::~SmoothHeightMesh() {

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
	const int intrad,
	const float resolution,
	std::vector<float>& colsMaxima,
	std::vector<int>& maximaRows)
{
	// initialize the algorithm: find the maximum
	// height per column and the corresponding row
	for (int y = 0; y <= std::min(maxy, intrad); ++y) {
		for (int x = 0; x <= maxx; ++x)  {
			const float curx = x * resolution;
			const float cury = y * resolution;
			const float curh = ground->GetHeightAboveWater(curx, cury);

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
	      std::vector<int>& maximaRows)
{
	const float cury = y * resolution;

	// try to advance rows if they're equal to current maximum but are further away
	for (int x = 0; x <= maxx; ++x) {
		if (maximaRows[x] == (y - 1)) {
			const float curx = x * resolution;
			const float curh = ground->GetHeightAboveWater(curx, cury);

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
	int intrad,
	float resolution,
	const std::vector<float>& colsMaxima,
	      std::vector<float>& mesh)
{
	const float cury = y * resolution;

	for (int x = 0; x <= maxx; ++x) {
		float maxRowHeight = -std::numeric_limits<float>::max();

		// find current maximum within radius smoothRadius
		// (in every column stack) along the current row
		const int startx = std::max(x - intrad, 0);
		const int endx = std::min(maxx, x + intrad);

		for (int i = startx; i <= endx; ++i) {
			assert(i >= 0);
			assert(i <= maxx);
			assert(ground->GetHeightReal(i * resolution, cury) <= colsMaxima[i]);

			maxRowHeight = std::max(colsMaxima[i], maxRowHeight);
		}

#ifndef NDEBUG
		const float curx = x * resolution;
		assert(maxRowHeight <= std::max(readmap->currMaxHeight, 0.0f));
		assert(maxRowHeight >= ground->GetHeightAboveWater(curx, cury));

	#ifdef SMOOTHMESH_CORRECTNESS_CHECK
		// naive algorithm
		float maxRowHeightAlt = -std::numeric_limits<float>::max();

		for (float y1 = cury - smoothRadius; y1 <= cury + smoothRadius; y1 += resolution) {
			for (float x1 = curx - smoothRadius; x1 <= curx + smoothRadius; x1 += resolution) {
				maxRowHeightAlt = std::max(maxRowHeightAlt, ground->GetHeightAboveWater(x1, y1));
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
	const int intrad,
	const float resolution,
	std::vector<float>& colsMaxima,
	std::vector<int>& maximaRows)
{
	// fix remaining maximums after a pass
	const int nextrow = y + intrad + 1;
	const float nextrowy = nextrow * resolution;

	for (int x = 0; x <= maxx; ++x) {
#ifdef _DEBUG
		for (int y1 = std::max(0, y - intrad); y1 <= std::min(maxy, y + intrad); ++y1) {
			assert(ground->GetHeightReal(x * resolution, y1 * resolution) <= colsMaxima[x]);
		}
#endif
		const float curx = x * resolution;

		if (maximaRows[x] <= (y - intrad)) {
			// find a new maximum if the old one left the window
			colsMaxima[x] = -std::numeric_limits<float>::max();

			for (int y1 = std::max(0, y - intrad + 1); y1 <= std::min(maxy, nextrow); ++y1) {
				const float h = ground->GetHeightAboveWater(curx, y1 * resolution);

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
			const float h = ground->GetHeightAboveWater(curx, nextrowy);

			if (h > colsMaxima[x]) {
				colsMaxima[x] = h;
				maximaRows[x] = nextrow;
			}
		}

		assert(maximaRows[x] <= nextrow);
		assert(maximaRows[x] >= y - intrad + 1);

#ifdef _DEBUG
		for (int y1 = std::max(0, y - intrad + 1); y1 <= std::min(maxy, y + intrad + 1); ++y1) {
			assert(colsMaxima[x] >= ground->GetHeightReal(curx, y1 * resolution));
		}
#endif
	}
}



inline static void BlurHorizontal(
	const int maxx,
	const int maxy,
	const int smoothrad,
	const float resolution,
	const std::vector<float>& mesh,
	std::vector<float>& smoothed)
{
	const float n = 2.0f * smoothrad + 1.0f;
	const float recipn = 1.0f / n;

	for_mt(0, maxy+1, [&](const int y) {
		float avg = 0.0f;

		for (int x = 0; x <= 2 * smoothrad; ++x) {
			avg += mesh[x + y * maxx];
		}

		for (int x = 0; x <= maxx; ++x) {
			const int idx = x + y * maxx;

			if (x <= smoothrad || x > (maxx - smoothrad)) {
				// map-border case
				smoothed[idx] = 0.0f;

				const int xstart = std::max(x - smoothrad, 0);
				const int xend   = std::min(x + smoothrad, maxx);

				for (int x1 = xstart; x1 <= xend; ++x1) {
					smoothed[idx] += mesh[x1 + y * maxx];
				}

				const float gh = ground->GetHeightAboveWater(x * resolution, y * resolution);
				const float sh = smoothed[idx] / (xend - xstart + 1);

				smoothed[idx] = std::min(readmap->currMaxHeight, std::max(gh, sh));
			} else {
				// non-border case
				avg += mesh[idx + smoothrad] - mesh[idx - smoothrad - 1];

				const float gh = ground->GetHeightAboveWater(x * resolution, y * resolution);
				const float sh = recipn * avg;

				smoothed[idx] = std::min(readmap->currMaxHeight, std::max(gh, sh));
			}

			assert(smoothed[idx] <= std::max(readmap->currMaxHeight, 0.0f));
			assert(smoothed[idx] >=          readmap->currMinHeight       );
		}
	});
}

inline static void BlurVertical(
	const int maxx,
	const int maxy,
	const int smoothrad,
	const float resolution,
	const std::vector<float>& mesh,
	std::vector<float>& smoothed)
{
	const float n = 2.0f * smoothrad + 1.0f;
	const float recipn = 1.0f / n;

	for_mt(0, maxx+1, [&](const int x) {
		float avg = 0.0f;

		for (int y = 0; y <= 2 * smoothrad; ++y) {
			avg += mesh[x + y * maxx];
		}

		for (int y = 0; y <= maxy; ++y) {
			const int idx = x + y * maxx;

			if (y <= smoothrad || y > (maxy - smoothrad)) {
				// map-border case
				smoothed[idx] = 0.0f;

				const int ystart = std::max(y - smoothrad, 0);
				const int yend   = std::min(y + smoothrad, maxy);

				for (int y1 = ystart; y1 <= yend; ++y1) {
					smoothed[idx] += mesh[x + y1 * maxx];
				}

				const float gh = ground->GetHeightAboveWater(x * resolution, y * resolution);
				const float sh = smoothed[idx] / (yend - ystart + 1);

				smoothed[idx] = std::min(readmap->currMaxHeight, std::max(gh, sh));
			} else {
				// non-border case
				avg += mesh[x + (y + smoothrad) * maxx] - mesh[x + (y - smoothrad - 1) * maxx];

				const float gh = ground->GetHeightAboveWater(x * resolution, y * resolution);
				const float sh = recipn * avg;

				smoothed[idx] = std::min(readmap->currMaxHeight, std::max(gh, sh));
			}

			assert(smoothed[idx] <= std::max(readmap->currMaxHeight, 0.0f));
			assert(smoothed[idx] >=          readmap->currMinHeight       );
		}
	});
}



inline static void CheckInvariants(
	int y,
	int maxx,
	int maxy,
	int intrad,
	float resolution,
	const std::vector<float>& colsMaxima,
	const std::vector<int>& maximaRows)
{
	// check invariants
	if (y < maxy) {
		for (int x = 0; x <= maxx; ++x) {
			assert(maximaRows[x] > y - intrad);
			assert(maximaRows[x] <= maxy);
			assert(colsMaxima[x] <= std::max(readmap->currMaxHeight, 0.0f));
			assert(colsMaxima[x] >=          readmap->currMinHeight       );
		}
	}
	for (int y1 = std::max(0, y - intrad + 1); y1 <= std::min(maxy, y + intrad + 1); ++y1) {
		for (int x1 = 0; x1 <= maxx; ++x1) {
			assert(ground->GetHeightReal(x1 * resolution, y1 * resolution) <= colsMaxima[x1]);
		}
	}
}



void SmoothHeightMesh::MakeSmoothMesh(const CGround* ground)
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
	const size_t size = (this->maxx + 1) * (this->maxy + 1);
	// use sliding window of maximums to reduce computational complexity
	const int intrad = smoothRadius / resolution;
	const int smoothrad = 3;

	assert(mesh.empty());
	mesh.resize(size);

	std::vector<float> colsMaxima(maxx + 1, -std::numeric_limits<float>::max());
	std::vector<int> maximaRows(maxx + 1, -1);
	std::vector<float> smoothed(size);

	FindMaximumColumnHeights(maxx, maxy, intrad, resolution, colsMaxima, maximaRows);

	for (int y = 0; y <= maxy; ++y) {
		AdvanceMaximaRows(y, maxx, resolution, colsMaxima, maximaRows);
		FindRadialMaximum(y, maxx, intrad, resolution, colsMaxima, mesh);
		FixRemainingMaxima(y, maxx, maxy, intrad, resolution, colsMaxima, maximaRows);

#ifdef _DEBUG
		CheckInvariants(y, maxx, maxy, intrad, resolution, colsMaxima, maximaRows);
#endif
	}

	// actually smooth with approximate Gaussian blur passes
	for (int numBlurs = 3; numBlurs > 0; --numBlurs) {
		BlurHorizontal(maxx, maxy, smoothrad, resolution, mesh, smoothed); mesh.swap(smoothed);
		BlurVertical(maxx, maxy, smoothrad, resolution, mesh, smoothed); mesh.swap(smoothed);
	}

	// `mesh` now contains the smoothed heightmap, save it in origMesh
	origMesh.resize(size);
	std::copy(mesh.begin(), mesh.end(), origMesh.begin());
}
