/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <vector>
#include <cassert>
#include <cstring>

#include "SmoothHeightMesh.h"

#include "Rendering/GL/myGL.h"
#include "float3.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "LogOutput.h"
#include "TimeProfiler.h"

#include "mmgr.h"

using std::vector;

SmoothHeightMesh *smoothGround = 0;


/// lifted from Ground.cpp
static float Interpolate(float x, float y, int maxx, int maxy, float res, const float* heightmap)
{
	if (x < 1)
		x = 1;
	else if (x > float3::maxxpos)
		x = float3::maxxpos;

	if (y < 1)
		y = 1;
	else if (y > float3::maxzpos)
		y = float3::maxzpos;

	const float invres = 1.0f / res;

	const int sx = (int) (x * invres);
	const int sy = (int) (y * invres);
	const float dx = (x - sx * res) * invres;
	const float dy = (y - sy * res) * invres;
	const int hs = sx + sy * (maxx);



	if (dx + dy < 1) {
		if (hs >= maxx * maxy) {
			return 0;
		}

		const float xdif = (dx) * (heightmap[hs +        1] - heightmap[hs]);
		const float ydif = (dy) * (heightmap[hs + maxx] - heightmap[hs]);

		return heightmap[hs] + xdif + ydif;
	}
	else {
		if (hs + 1 >= maxx * maxy) {
			return 0;
		}

		const float xdif = (1.0f - dx) * (heightmap[hs + maxx] - heightmap[hs + 1 + maxx]);
		const float ydif = (1.0f - dy) * (heightmap[hs        + 1] - heightmap[hs + 1 + maxx]);

		return heightmap[hs + 1 + maxx] + xdif + ydif;
	}
	return 0; // can not be reached
}


float SmoothHeightMesh::GetHeight(float x, float y)
{
	assert(mesh);

	return Interpolate(x, y, maxx, maxy, resolution, mesh);
}

float SmoothHeightMesh::GetHeight2(float x, float y)
{
	assert(mesh);
	float h = Interpolate(x, y, maxx, maxy, resolution, mesh);
	return (h < 0.0f ? 0.0f : h);
}


float SmoothHeightMesh::SetHeight(int index, float h)
{
	mesh[index] = h;
	return mesh[index];
}

float SmoothHeightMesh::AddHeight(int index, float h)
{
	mesh[index] += h;
	return mesh[index];
}

float SmoothHeightMesh::SetMaxHeight(int index, float h)
{
	mesh[index] = std::max(h, mesh[index]);
	return mesh[index];
}

void  SmoothHeightMesh::MakeSmoothMesh(const CGround *ground)
{
	ScopedOnceTimer timer("Calculating smooth mesh");
	if (!mesh) {
		size_t size = (size_t)((this->maxx+1) * (this->maxy + 1));
		mesh = new float[size];
	}

	float smallest = 1e30f;
	float largest = -1e30f;

	if (smoothRadius < 1)
		smoothRadius = 1;

	// sliding window of maximums to reduce computational complexity
	const int intrad = smoothRadius/resolution;

	vector<float> maximums;
	vector<int> rows;

	maximums.resize(maxx+1);
	rows.resize(maxx+1);

	// fill maximums with lowest possible value
	for(int x = 0; x <= maxx; x++)
		maximums[x] = -1e20f;

	// initialize the algorithm
	for (int y = 0; y <= std::min(maxy, intrad); ++y) {
		for (int x = 0; x <= maxx; ++x)  {
			float curx = x*resolution;
			float cury = y*resolution;
			float h = ground->GetHeight(curx, cury);
			if (maximums[x] < h) {
				maximums[x] = h;
				rows[x] = y;
			}
		}
	}

	for (int y = 0; y<=maxy; ++y) {
		float cury = y*resolution;
		// try to advance rows if they're equal to current maximum but are further away
		for (int x = 0; x <= maxx; ++x) {
			if (rows[x] == y-1) {
				float curx = x*resolution;
				float h = ground->GetHeight(curx, cury);
				if (h == maximums[x]) {
					rows[x] = y;
				}
				assert(h <= maximums[x]);
			}
		}
		for (int x = 0; x <= maxx; ++x) {
			float curx = x*resolution;
			float val = -1.f;

			// find current maximum in radius smoothRadius
			int startx = std::max(x-intrad, 0);
			int endx = std::min(maxx, x+intrad);
			for (int i = startx; i <= endx; ++i) {
				assert(i >= 0);
				assert(i <= maxx);

				float storedx = i * resolution;
				assert(ground->GetHeight2(storedx, cury) <= maximums[i]);

				if (val < maximums[i]) {
					val = maximums[i];
				}
			}

#if defined(_DEBUG) && defined(SMOOTHMESH_CORRECTNESS_CHECK)
			// naive algorithm
			float val2 = -1e20f;
			for (float y1 = cury - smoothRadius; y1 <= cury + smoothRadius; y1 += resolution) {
				for (float x1 = curx - smoothRadius; x1 <= curx + smoothRadius; x1 += resolution) {
					// CGround::GetHeight() never returns values < 0
					float h = ground->GetHeight(x1, y1);
					if (val2 < h) {
						val2 = h;
					}
				}
			}
			assert(val2 == val);
#endif

#ifndef NDEBUG
			float h = ground->GetHeight(curx, cury);
			assert(val <= std::max(readmap->currMaxHeight, 0.f));
			assert(val >= h);
#endif

			mesh[x + y*maxx] = val;
			// stats
			if (val < smallest) smallest = val;
			if (val > largest) largest = val;
		}
		// fix remaining maximums after a pass
		int nextrow = y + intrad + 1;		
		float nextrowy = nextrow * resolution;
		for (int x = 0; x <= maxx; ++x) {
#ifdef _DEBUG
			for (int y1 = std::max(0, y-intrad); y1<=std::min(maxy, y+intrad); ++y1) {
				assert(ground->GetHeight2(x*resolution, y1*resolution) <= maximums[x]);
			}
#endif
			float curx = x * resolution;
			if (rows[x] <= y-intrad) {
				// find a new maximum if the old one left the window
				maximums[x] = -1e20f;
				for (int y1 = std::max(0, y-intrad+1); y1<=std::min(maxy, nextrow); ++y1) {
					float h = ground->GetHeight(curx, y1*resolution);
					if (maximums[x] < h) {
						maximums[x] = h;
						rows[x] = y1;
					} else if (maximums[x] == h) {
						// if equal, move as far down as possible
						rows[x] = y1;
					}
				}
			} else if (nextrow <= maxy) {
				// else, just check if a new maximum has entered the window
				float h = ground->GetHeight(curx, nextrowy);
				if (maximums[x] < h) {
					maximums[x] = h;
					rows[x] = nextrow;
				}
			}
			assert(rows[x] <= nextrow);
			assert(rows[x] >= y - intrad + 1);
#ifdef _DEBUG
			for (int y1 = std::max(0, y-intrad+1); y1<=std::min(maxy, y+intrad+1); ++y1) {
				assert(ground->GetHeight2(curx, y1*resolution) <= maximums[x]);
			}
#endif
		}
#ifdef _DEBUG
		// check invariants
		if (y < maxy) {
			for (int x = 0; x <= maxx; ++x) {
				assert(rows[x] > y - intrad);
				assert(rows[x] <= maxy);
				assert(maximums[x] <= std::max(readmap->currMaxHeight, 0.f));
				assert(maximums[x] >= readmap->currMinHeight);
			}
		}
		for (int y1 = std::max(0, y-intrad+1); y1<=std::min(maxy, y+intrad+1); ++y1) {
			for (int x1 = 0; x1 <= maxx; ++x1) {
				assert(ground->GetHeight2(x1*resolution, y1*resolution) <= maximums[x1]);
			}
		}
#endif
	}


	// actually smooth
	const size_t smoothsize = (size_t)((this->maxx+1) * (this->maxy + 1));
	const int smoothrad = 3;
	float *smoothed = new float[smoothsize];
	/*
	for (int y = 0; y <= maxy; ++y) {
		for (int x = 0; x <= maxx; ++x) {
			// sum and average
			int counter = 0;
			int idx = x + maxx*y;
			smoothed[idx] = 0.f;
			for (int y1 = std::max(0, y-smoothrad); y1<=std::min(maxy, y+smoothrad); ++y1) {
				for (int x1 = std::max(0, x-smoothrad); x1 <= std::min(maxx, x+smoothrad); ++x1) {
					++counter;
					smoothed[idx] += mesh[x1 + y1 * maxx];
				}
			}
			smoothed[idx] = std::max(ground->GetHeight(x*resolution, y*resolution), smoothed[idx]/(float)counter);
		}
	}
	*/

	float n = 2*smoothrad + 1;
	float recipn = 1.f/n;

	// approximate gaussian blur
	for (int numBlurs = 3; numBlurs > 0; --numBlurs) {
		/* horizontal pass */
		for (int y = 0; y <= maxy; ++y) {
			float avg = 0;
			for (int x = 0; x <= 2*smoothrad; ++x)
				avg += mesh[x + y * maxx];
			for (int x = 0; x <= maxx; ++x) {
				int idx = x + y * maxx;
				if (x <= smoothrad) {
					smoothed[idx] = 0.;
					for (int x1 = 0; x1 <= x + smoothrad; ++x1)
						smoothed[idx] += mesh[x1 + y * maxx];
					smoothed[idx] = std::min(readmap->currMaxHeight,
								 std::max(ground->GetHeight(x*resolution, y*resolution),
									  smoothed[idx]/(x + smoothrad + 1)));
				} else if (x > maxx-smoothrad) {
					smoothed[idx] = 0.;
					for (int x1 = x-smoothrad; x1 <= maxx; ++x1)
						smoothed[idx] += mesh[x1 + y * maxx];
					smoothed[idx] = std::min(readmap->currMaxHeight,
								 std::max(ground->GetHeight(x*resolution, y*resolution),
									  smoothed[idx]/(maxx - (x - smoothrad) + 1)));
				} else {
					avg += mesh[idx + smoothrad] - mesh[idx - smoothrad - 1];
					smoothed[idx] = std::min(readmap->currMaxHeight,
								 std::max(ground->GetHeight(x*resolution, y*resolution),
									  recipn * avg));
				}
				assert(smoothed[idx] <= std::max(readmap->currMaxHeight, 0.f));
				assert(smoothed[idx] >= readmap->currMinHeight);
			}
		}
		memcpy(mesh, smoothed, smoothsize*sizeof(float));

		/* vertical pass */
		for (int x = 0; x <= maxx; ++x) {
			float avg = 0;
			for (int y = 0; y <= 2*smoothrad; ++y)
				avg += mesh[x + y * maxx];
			for (int y = 0; y <= maxy; ++y) {
				int idx = x + y * maxx;
				if (y <= smoothrad) {
					smoothed[idx] = 0.;
					for (int y1 = 0; y1 <= y + smoothrad; ++y1)
						smoothed[idx] += mesh[x + y1 * maxx];
					smoothed[idx] = std::min(readmap->currMaxHeight,
								 std::max(ground->GetHeight(x*resolution, y*resolution),
									  smoothed[idx]/(y + smoothrad + 1)));
				} else if (y > maxy-smoothrad) {
					smoothed[idx] = 0.;
					for (int y1 = y - smoothrad; y1 <= maxy; ++y1)
						smoothed[idx] += mesh[x + y1 * maxx];
					smoothed[idx] = std::min(readmap->currMaxHeight,
								 std::max(ground->GetHeight(x*resolution, y*resolution),
									  smoothed[idx]/(maxy - (y - smoothrad) + 1)));
				} else {
					avg += mesh[x + (y+smoothrad)*maxx] - mesh[x + (y-smoothrad-1)*maxx];
					smoothed[idx] = std::min(readmap->currMaxHeight,
								 std::max(ground->GetHeight(x*resolution, y*resolution),
									  recipn * avg));
				}
				assert(smoothed[idx] <= std::max(readmap->currMaxHeight, 0.f));
				assert(smoothed[idx] >= readmap->currMinHeight);
			}
		}
		memcpy(mesh, smoothed, smoothsize*sizeof(float));
	}

	delete[] origMesh;
	origMesh = mesh;
	mesh = smoothed;
}


void SmoothHeightMesh::DrawWireframe(float yoffset)
{
	if (!mesh)
		return;

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor3f(0.f, 1.f, 0.f);
	glLineWidth(1.f);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_CULL_FACE);


	glBegin(GL_QUADS);
	const float inc = 4*resolution;
	for (float z = 0; z < this->fmaxy; z += inc) {
		for (float x = 0; x < this->fmaxx; x += inc) {
			float h1 = this->GetHeight2(x, z);
			float h2 = this->GetHeight2(x + inc, z);
			float h3 = this->GetHeight2(x + inc, z + inc);
			float h4 = this->GetHeight2(x, z + inc);

			glVertex3f(x,
				   h1 + yoffset,
				   z
				   );
			glVertex3f((x + inc),
				   h2 + yoffset,
				   z
				   );
			glVertex3f((x + inc),
				   h3 + yoffset,
				   (z + inc)
				   );
			glVertex3f(x,
				   h4 + yoffset,
				   (z + inc)
				   );
		}
	}
	glEnd();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
