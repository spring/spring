/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <vector>
#include <cassert>
#include <limits>

#include "SmoothHeightMesh.h"

#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/ModInfo.h"
#include "System/float3.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"


using namespace SmoothHeightMeshNamespace;

#if 0
#define SMOOTH_MESH_DEBUG_MAXIMA
#endif

#if 0
#define SMOOTH_MESH_DEBUG_BLUR
#endif

#if 0
#define SMOOTH_MESH_DEBUG_GENERAL
#endif

SmoothHeightMesh smoothGround;


static float Interpolate(float x, float y, const int maxx, const int maxy, const float res, const float* heightmap)
{
	x = Clamp(x / res, 0.0f, (float)maxx);
	y = Clamp(y / res, 0.0f, (float)maxy);
	const int sx = std::min((int)x, maxx - 1);
	const int sy = std::min((int)y, maxy - 1);
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

void SmoothHeightMesh::Init(int2 max, int res, int smoothRad)
{
	Kill();

	enabled = modInfo.enableSmoothMesh;

	// we use SSE in performance sensitive code, don't let the window size be too small.
	if (smoothRad < 4) smoothRad = 4;

	fmaxx = max.x * SQUARE_SIZE;
	fmaxy = max.y * SQUARE_SIZE;
	fresolution = res * SQUARE_SIZE;

	resolution = res;
	maxx = max.x / resolution;
	maxy = max.y / resolution;

	smoothRadius = std::max(1, smoothRad);

	InitMapChangeTracking();
	InitDataStructures();

	if (enabled) MakeSmoothMesh();
}

void SmoothHeightMesh::InitMapChangeTracking() {
	const int damageTrackWidth = maxx / SAMPLES_PER_QUAD + (maxx % SAMPLES_PER_QUAD ? 1 : 0);
	const int damageTrackHeight = maxy / SAMPLES_PER_QUAD + (maxy % SAMPLES_PER_QUAD ? 1 : 0);
	const int damageTrackQuads = damageTrackWidth * damageTrackHeight;
	mapChangeTrack.damageMap.clear();
	mapChangeTrack.damageMap.resize(damageTrackQuads);
	mapChangeTrack.width = damageTrackWidth;
	mapChangeTrack.height = damageTrackHeight;
}

void SmoothHeightMesh::InitDataStructures() {
	assert(mesh.empty());
	maximaMesh.resize(maxx * maxy, 0.0f);
	mesh.resize(maxx * maxy, 0.0f);
	tempMesh.resize(maxx * maxy, 0.0f);
	origMesh.resize(maxx * maxy, 0.0f);
	colsMaxima.clear();
	colsMaxima.resize(maxx, -std::numeric_limits<float>::max());
	maximaRows.clear();
	maximaRows.resize(maxx, -1);
}

void SmoothHeightMesh::Kill() {
	while (!mapChangeTrack.damageQueue[0].empty()) { mapChangeTrack.damageQueue[0].pop(); }
	while (!mapChangeTrack.damageQueue[1].empty()) { mapChangeTrack.damageQueue[1].pop(); }
	while (!mapChangeTrack.horizontalBlurQueue.empty()) { mapChangeTrack.horizontalBlurQueue.pop(); }
	while (!mapChangeTrack.verticalBlurQueue.empty()) { mapChangeTrack.verticalBlurQueue.pop(); }

	mapChangeTrack.damageMap.clear();
	maximaMesh.clear();
	mesh.clear();
	origMesh.clear();
}

float SmoothHeightMesh::GetHeight(float x, float y)
{
	assert(!mesh.empty());
	return Interpolate(x, y, maxx, maxy, fresolution, &mesh[0]);
}

float SmoothHeightMesh::GetHeightAboveWater(float x, float y)
{
	assert(!mesh.empty());
	return std::max(0.0f, Interpolate(x, y, maxx, maxy, fresolution, &mesh[0]));
}

float SmoothHeightMesh::SetHeight(int index, float h)
{
	assert(index < maxx*maxy);
	return (mesh[index] = h);
}

float SmoothHeightMesh::AddHeight(int index, float h)
{
	assert(index < maxx*maxy);
	return (mesh[index] += h);
}

float SmoothHeightMesh::SetMaxHeight(int index, float h)
{
	assert(index < maxx*maxy);
	return (mesh[index] = std::max(h, mesh[index]));
}

inline static float GetRealGroundHeight(int x, int y, int resolution) {
	const float* heightMap = readMap->GetCornerHeightMapSynced();
	const int baseIndex = (x + y*mapDims.mapxp1)*resolution;

	return heightMap[baseIndex];
}

inline static void FindMaximumColumnHeights(
	const int2 map,
	const int y,
	const int minx,
	const int maxx,
	const int winSize,
	const int resolution,
	std::vector<float>& colsMaxima,
	std::vector<int>& maximaRows
) {
	// initialize the algorithm: find the maximum
	// height per column and the corresponding row

	const int miny = std::max(y - winSize, 0);
	const int maxy = std::min(y + winSize, map.y - 1);

	for (int y1 = miny; y1 <= maxy; ++y1) {
		for (int x = minx; x <= maxx; ++x)  {
			const float curh = GetRealGroundHeight(x, y1, resolution);

			if (curh >= colsMaxima[x]) {
				colsMaxima[x] = curh;
				maximaRows[x] = y1;
			}
		}
	}

#ifdef SMOOTH_MESH_DEBUG_MAXIMA
	for (int x = minx; x <= maxx; ++x)
		LOG("%s: y:%d col:%d row:%d max: %f", __func__, y, x, maximaRows[x], colsMaxima[x]);
#endif
}


inline static void FindRadialMaximum(
	const int2 map,
	int y,
	int minx,
	int maxx,
	int winSize,
	float resolution,
	const std::vector<float>& colsMaxima,
	      std::vector<float>& mesh
) {
	for (int x = minx; x <= maxx; ++x) {
		float maxRowHeight = -std::numeric_limits<float>::max();

		// find current maximum within radius smoothRadius
		// (in every column stack) along the current row
		const int startx = std::max(x - winSize, 0);
		const int endx = std::min(x + winSize, map.x - 1);

		// This may mean the last SSE max function compares some values have already been compared
		// This is harmless and avoids needlessly messy SSE code here
		const int endIdx = endx - 3;

		// Main loop for finding maximum height values
		__m128 best = _mm_loadu_ps(&colsMaxima[startx]);
		for (int i = startx + 4; i < endIdx; i += 4) {
			__m128 next = _mm_loadu_ps(&colsMaxima[i]);
			best = _mm_max_ps(best, next);
		}

		// Check the last few height values
		{
			__m128 next = _mm_loadu_ps(&colsMaxima[endIdx]);
			best = _mm_max_ps(best, next);
		}

		// This is an SSE horizontal compare
		{
			// split the four values into sets of two and compare
			__m128 bestAlt = _mm_movehl_ps(best, best);
			best = _mm_max_ps(best, bestAlt);

			// split the two values and compare
			bestAlt = _mm_shuffle_ps(best, best, _MM_SHUFFLE(0, 0, 0, 1));
			best = _mm_max_ss(best, bestAlt);
			_mm_store_ss(&maxRowHeight, best);
		}

		mesh[x + y * map.x] = maxRowHeight;

#ifdef SMOOTH_MESH_DEBUG_MAXIMA
		LOG("%s: y:%d x:%d local max: %f", __func__, y, x, maxRowHeight);
#endif
	}
}


inline static void AdvanceMaximas(
	const int2 map,
	const int y,
	const int minx,
	const int maxx,
	const int winSize,
	const int resolution,
	std::vector<float>& colsMaxima,
	std::vector<int>& maximaRows
) {
	// fix remaining maximums after a pass
	const int miny = std::max(y - winSize, 0);
	const int virtualRow = y + winSize;
	const int maxy = std::min(virtualRow, map.y - 1);

	for (int x = minx; x <= maxx; ++x) {
		if (maximaRows[x] < miny) {
			// find a new maximum if the old one left the window
			colsMaxima[x] = -std::numeric_limits<float>::max();

			for (int y1 = miny; y1 <= maxy; ++y1) {
				const float h = GetRealGroundHeight(x, y1, resolution);

				if (h >= colsMaxima[x]) {
					colsMaxima[x] = h;
					maximaRows[x] = y1;
				}
			}
		} else if (virtualRow < map.y) {
			// else, just check if a new maximum has entered the window
			const float h = GetRealGroundHeight(x, maxy, resolution);

			if (h >= colsMaxima[x]) {
				colsMaxima[x] = h;
				maximaRows[x] = maxy;
			}
		}

#ifdef SMOOTH_MESH_DEBUG_MAXIMA
		LOG("%s: y:%d x:%d column max: %f", __func__, y, x, colsMaxima[x]);
#endif

		assert(maximaRows[x] <= maxy);
		assert(maximaRows[x] >= y - winSize);
	}
}


inline static void BlurHorizontal(
	const int2 mapSize,
	const int2 min,
	const int2 max,
	const int blurSize,
	const int resolution,
	const std::vector<float>& mesh,
	      std::vector<float>& smoothed
) {
	const int lineSize = mapSize.x;
	const int mapMaxX = mapSize.x - 1;

	for (int y = min.y; y <= max.y; ++y)
	{
		float avg = 0.0f;
		float lv = 0;
		float rv = 0;
		float weight = 1.f / ((float)(blurSize*2 + 1));
		int li = min.x - blurSize;
		int ri = min.x + blurSize;

		// linear blending allows us to add up all the values to average for the first point
		// the rest of the points can be determined by taking the average after removing the
		// last oldest value and adding the next value.
		for (int x1 = li; x1 <= ri; ++x1) {
		 	avg += mesh[std::max(0, std::min(x1, mapMaxX)) + y * lineSize];
		}
		// ri should point to the next value to add to the averages
		ri++;

		for (int x = min.x; x <= max.x; ++x)
		{
			// Remove the oldest height value (lv) and add the newest height value (rv)
			avg += (-lv) + rv;
			const float gh = GetRealGroundHeight(x, y, resolution);
			smoothed[x + y * lineSize] = std::max(gh, avg*weight);

			// Get the values to add/remove for next iteration
			lv = mesh[std::max(0, std::min(li, mapMaxX)) + y * lineSize];
			rv = mesh[            std::min(ri, mapMaxX)  + y * lineSize];
			li++; ri++;

#ifdef SMOOTH_MESH_DEBUG_BLUR
			LOG ( "%s: x: %d, y: %d, avg: %f (%f) (g: %f) (max h: %f)"
				, __func__, x, y, avg, avg*weight, gh, readMap->GetCurrMaxHeight());
#endif
			assert(smoothed[x + y * lineSize] <= readMap->GetCurrMaxHeight() + 1.f);
			assert(smoothed[x + y * lineSize] >= readMap->GetCurrMinHeight() - 1.f);
		}
	}
}

inline static void BlurVertical(
	const int2 mapSize,
	const int2 min,
	const int2 max,
	const int blurSize,
	const int resolution,
	const std::vector<float>& mesh,
	      std::vector<float>& smoothed
) {
	// See BlurHorizontal for all the detailed comments.

	const int lineSize = mapSize.x;
	const int mapMaxY = mapSize.y - 1;

	for (int x = min.x; x <= max.x; ++x)
	{
		float avg = 0.0f;
		float lv = 0;
		float rv = 0;
		float weight = 1.f / ((float)(blurSize*2 + 1));
		int li = min.y - blurSize;
		int ri = min.y + blurSize;
		for (int y1 = li; y1 <= ri; ++y1) {
			avg += mesh[x + std::max(0, std::min(y1, mapMaxY)) * lineSize];
		}
		ri++;

#ifdef SMOOTH_MESH_DEBUG_BLUR
		LOG("%s: starting average is: %f (%f) (w: %f)", __func__, avg, avg*weight, weight);
#endif

		for (int y = min.y; y <= max.y; ++y)
		{
			avg += (-lv) + rv;
			const float gh = GetRealGroundHeight(x, y, resolution);
			smoothed[x + y * lineSize] = std::max(gh, avg*weight);

			lv = mesh[ x + std::max(0, std::min(li, mapMaxY)) * lineSize];
			rv = mesh[ x +             std::min(ri, mapMaxY)  * lineSize];
			li++; ri++;

#ifdef SMOOTH_MESH_DEBUG_BLUR
			LOG("%s: x: %d, y: %d, avg: %f (%f) (g: %f)", __func__, x, y, avg, avg*weight, gh);
			LOG("%s: for next line -%f +%f", __func__, lv, rv);
#endif
			assert(smoothed[x + y * lineSize] <= readMap->GetCurrMaxHeight() + 1.f);
			assert(smoothed[x + y * lineSize] >= readMap->GetCurrMinHeight() - 1.f);
		}
	}
}


inline static void CheckInvariants(
	int y,
	int maxx,
	int maxy,
	int winSize,
	int resolution,
	const std::vector<float>& colsMaxima,
	const std::vector<int>& maximaRows
) {
	// check invariants - only for initial mesh creationgit reset HEAD~
	if (y <= maxy) {
		for (int x = 0; x <= maxx; ++x) {
			assert(maximaRows[x] > y - winSize);
			assert(maximaRows[x] <= maxy);
			assert(colsMaxima[x] <= readMap->GetCurrMaxHeight() + 1.f);
			assert(colsMaxima[x] >= readMap->GetCurrMinHeight() - 1.f);
		}
	}
	for (int y1 = std::max(0, (y - winSize) + 1); y1 <= std::min(maxy, y + winSize + 1); ++y1) {
		for (int x1 = 0; x1 <= maxx; ++x1) {
			assert(GetRealGroundHeight(x1, y1, resolution) <= colsMaxima[x1] + 1.f);
		}
	}
}


void SmoothHeightMesh::MapChanged(int x1, int y1, int x2, int y2) {

	if (!enabled) return;

	const bool queueWasEmpty = mapChangeTrack.damageQueue[mapChangeTrack.activeBuffer].empty();
	const int res = resolution*SAMPLES_PER_QUAD;
	const int w = mapChangeTrack.width;
	const int h = mapChangeTrack.height;
	
	const int2 min  { std::max((x1 - smoothRadius) / res, 0)
					, std::max((y1 - smoothRadius) / res, 0)};
	const int2 max  { std::min((x2 + smoothRadius - 1) / res, (w-1))
					, std::min((y2 + smoothRadius - 1) / res, (h-1))};

	for (int y = min.y; y <= max.y; ++y) {
		int i = min.x + y*w;
		for (int x = min.x; x <= max.x; ++x, ++i) {
			if (!mapChangeTrack.damageMap[i]) {
				mapChangeTrack.damageMap[i] = true;
				mapChangeTrack.damageQueue[mapChangeTrack.activeBuffer].push(i);
			}
		}	
	}

	const bool queueWasUpdated = !mapChangeTrack.damageQueue[mapChangeTrack.activeBuffer].empty();

	if (queueWasEmpty && queueWasUpdated)
		mapChangeTrack.queueReleaseOnFrame = gs->frameNum + SMOOTH_MESH_UPDATE_DELAY;
}


inline static void CopyMeshPart
		( int mapx
		, int2 min
		, int2 max
		, const std::vector<float>& src
		, std::vector<float>& dest
		) {
	for (int y = min.y; y <= max.y; ++y) {
		const int startIdx = min.x + y*mapx;
		const int endIdx = max.x + y*mapx + 1;
		std::copy(src.begin() + startIdx, src.begin() + endIdx, dest.begin() + startIdx);
	}
}


inline static bool UpdateSmoothMeshRequired(SmoothHeightMesh::MapChangeTrack& mapChangeTrack) {
	const bool flushBuffer = !mapChangeTrack.activeBuffer;
	const bool activeBuffer = mapChangeTrack.activeBuffer;
	const bool currentWorkloadComplete = mapChangeTrack.damageQueue[flushBuffer].empty()
									  && mapChangeTrack.horizontalBlurQueue.empty()
									  && mapChangeTrack.verticalBlurQueue.empty();

#ifdef SMOOTH_MESH_DEBUG_GENERAL
	LOG("%s: flush buffer is %d; damage queue is %I64d; blur queue is %I64d"
		, __func__, (int)flushBuffer, meshDamageTrack.damageQueue[flushBuffer].size()
		, meshDamageTrack.blurQueue.size()
		);
#endif

	if (currentWorkloadComplete){
		const bool activeBufferReady = !mapChangeTrack.damageQueue[activeBuffer].empty()
									&& gs->frameNum >= mapChangeTrack.queueReleaseOnFrame;
		if (activeBufferReady) {
			mapChangeTrack.activeBuffer = !mapChangeTrack.activeBuffer;
#ifdef SMOOTH_MESH_DEBUG_GENERAL
			LOG("%s: opening new queue", __func__);
#endif
		}
	}

	return !currentWorkloadComplete;
}


void SmoothHeightMesh::UpdateSmoothMeshMaximas(int2 damageMin, int2 damageMax) {
	const int winSize = smoothRadius / resolution;
	const int blurSize = std::max(1, winSize / 2);
	int2 map{maxx, maxy};

	int2 impactRadius{winSize, winSize};
	int2 min = damageMin - impactRadius;
	int2 max = damageMax + impactRadius;

	min.x = std::clamp(min.x, 0, map.x - 1);
	min.y = std::clamp(min.y, 0, map.y - 1);
	max.x = std::clamp(max.x, 0, map.x - 1);
	max.y = std::clamp(max.y, 0, map.y - 1);

#ifdef SMOOTH_MESH_DEBUG_GENERAL
LOG("%s: quad index %d (%d,%d)-(%d,%d) (%d,%d)-(%d,%d) updating maxima"
	, __func__, damagedAreaIndex, damageMin.x, damageMin.y, damageMax.x, damageMax.y, min.x, min.y, max.x, max.y
	);

LOG("%s: quad area in world space (%f,%f) (%f,%f)", __func__
	, (float)damageMin.x * fresolution, (float)damageMin.y * fresolution
	, (float)damageMax.x * fresolution, (float)damageMax.y * fresolution
	);
#endif

	for (int i = min.x; i <= max.x; ++i)
		colsMaxima[i] = -std::numeric_limits<float>::max();

	for (int i = min.x; i <= max.x; ++i)
		maximaRows[i] = -1;

	FindMaximumColumnHeights(map, damageMin.y, min.x, max.x, winSize, resolution, colsMaxima, maximaRows);

	for (int y = damageMin.y; y <= damageMax.y; ++y) {
		FindRadialMaximum(map, y, damageMin.x, damageMax.x, winSize, resolution, colsMaxima, maximaMesh);
		AdvanceMaximas(map, y+1, min.x, max.x, winSize, resolution, colsMaxima, maximaRows);
	}
}


void SmoothHeightMesh::UpdateSmoothMesh() {
	if (!enabled) return;

	SCOPED_TIMER("Sim::SmoothHeightMesh::UpdateSmoothMesh");

	if (!UpdateSmoothMeshRequired(mapChangeTrack)) return;

	const bool flushBuffer = !mapChangeTrack.activeBuffer;
	const bool updateMaxima = !mapChangeTrack.damageQueue[flushBuffer].empty();
	const bool doHorizontalBlur = !mapChangeTrack.horizontalBlurQueue.empty();

	std::queue<int>* activeQueue = nullptr;
	if (updateMaxima) 
		activeQueue = &mapChangeTrack.damageQueue[flushBuffer];
	else if (doHorizontalBlur)
		activeQueue = &mapChangeTrack.horizontalBlurQueue;
	else
		activeQueue = &mapChangeTrack.verticalBlurQueue;

	const int damagedAreaIndex = activeQueue->front();
	activeQueue->pop();

	// area of the map which to recalculate the height values
	const int damageX = damagedAreaIndex % mapChangeTrack.width;
	const int damageY = damagedAreaIndex / mapChangeTrack.width;
	int2 damageMin{damageX*SAMPLES_PER_QUAD, damageY*SAMPLES_PER_QUAD};
	int2 damageMax = damageMin + int2{SAMPLES_PER_QUAD - 1, SAMPLES_PER_QUAD - 1};

	damageMin.x = std::clamp(damageMin.x, 0, maxx - 1);
	damageMin.y = std::clamp(damageMin.y, 0, maxy - 1);
	damageMax.x = std::clamp(damageMax.x, 0, maxx - 1);
	damageMax.y = std::clamp(damageMax.y, 0, maxy - 1);

	if (updateMaxima) {
		UpdateSmoothMeshMaximas(damageMin, damageMax);

		mapChangeTrack.horizontalBlurQueue.push(damagedAreaIndex);
		mapChangeTrack.damageMap[damagedAreaIndex] = false;
	} else {
		const int winSize = smoothRadius / resolution;
		const int blurSize = std::max(1, winSize / 2);
		const int2 map{maxx, maxy};

#ifdef SMOOTH_MESH_DEBUG_GENERAL
	LOG("%s: quad index %d (%d,%d)-(%d,%d) applying blur", __func__
		, damagedAreaIndex, damageMin.x, damageMin.y, damageMax.x, damageMax.y
		);

	LOG("%s: quad area in world space (%f,%f) (%f,%f)", __func__
		, (float)damageMin.x * fresolution, (float)damageMin.y * fresolution
		, (float)damageMax.x * fresolution, (float)damageMax.y * fresolution
		);
#endif

		if (doHorizontalBlur) {
			BlurHorizontal(map, damageMin, damageMax, blurSize, resolution, maximaMesh, tempMesh);
			mapChangeTrack.verticalBlurQueue.push(damagedAreaIndex);
		}
		else {
			BlurVertical(map, damageMin, damageMax, blurSize, resolution, tempMesh, mesh);
			CopyMeshPart(map.x, damageMin, damageMax, mesh, tempMesh);
		}
	}
}


void SmoothHeightMesh::MakeSmoothMesh() {
	ScopedOnceTimer timer("SmoothHeightMesh::MakeSmoothMesh");

	// A sliding window is used to reduce computational complexity
	const int winSize = smoothRadius / resolution;

	// blur size is half the window size to create a wider plateau
	const int blurSize = std::max(1, winSize / 2);
	int2 min{0, 0};
	int2 max{maxx-1, maxy-1};
	int2 map{maxx, maxy};

 	FindMaximumColumnHeights(map, 0, 0, max.x, winSize, resolution, colsMaxima, maximaRows);

	for (int y = 0; y <= max.y; ++y) {
		FindRadialMaximum(map, y, 0, max.x, winSize, resolution, colsMaxima, maximaMesh);
		AdvanceMaximas(map, y+1, 0, max.x, winSize, resolution, colsMaxima, maximaRows);

#ifdef _DEBUG
		CheckInvariants(y, max.x, max.y, winSize, resolution, colsMaxima, maximaRows);
#endif
	}

	BlurHorizontal(map, min, max, blurSize, resolution, maximaMesh, tempMesh);
	BlurVertical(map, min, max, blurSize, resolution, tempMesh, mesh);

	// <mesh> now contains the final smoothed heightmap, save it in origMesh
	std::copy(mesh.begin(), mesh.end(), origMesh.begin());

	// tempMesh should be kept inline with mesh to avoid bluring artefacts in dynamic updates
	std::copy(mesh.begin(), mesh.end(), tempMesh.begin());
}


// =============================================
// Old gaussian kernel reference code kept here for future reference
// =============================================
	// const auto fillGaussianKernelFunc = [blurSize](std::vector<float>& gaussianKernel, const float sigma) {
	// 	gaussianKernel.resize(blurSize + 1);

	// 	const auto gaussianG = [](const int x, const float sigma) -> float {
	// 		// 0.3989422804f = 1/sqrt(2*pi)
	// 		return 0.3989422804f * math::expf(-0.5f * x * x / (sigma * sigma)) / sigma;
	// 	};

	// 	float sum = (gaussianKernel[0] = gaussianG(0, sigma));

	// 	for (int i = 1; i < blurSize + 1; ++i) {
	// 		sum += 2.0f * (gaussianKernel[i] = gaussianG(i, sigma));
	// 	}

	// 	for (auto& gk : gaussianKernel) {
	// 		gk /= sum;
	// 	}
	// };

	// constexpr float gSigma = 5.0f;
	// fillGaussianKernelFunc(gaussianKernel, gSigma);
// =============================================
