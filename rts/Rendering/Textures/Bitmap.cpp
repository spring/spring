/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <utility>
#include <cstring>

#include <IL/il.h>
#include <SDL_video.h>

#ifndef HEADLESS
	#include "Rendering/GL/myGL.h"
	#include "System/TimeProfiler.h"
#endif

#include "Bitmap.h"
#include "Rendering/GlobalRendering.h"
#include "System/bitops.h"
#include "System/ScopedFPUSettings.h"
#include "System/ContainerUtil.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"
#include "System/Threading/ThreadPool.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Threading/SpringThreading.h"
#include "System/SpringMath.h"

#define ENABLE_TEXMEMPOOL 1


struct InitializeOpenIL {
	InitializeOpenIL() { ilInit(); }
	~InitializeOpenIL() { ilShutDown(); }
} static initOpenIL;


struct TexMemPool {
private:
	// (index, size)
	typedef std::pair<size_t, size_t> FreePair;

	std::vector<uint8_t> memArray;
	std::vector<FreePair> freeList;

	// libIL is not thread-safe, neither are {Alloc,Free}
	spring::mutex bmpMutex;

	size_t numAllocs = 0;
	size_t allocSize = 0;
	size_t numFrees  = 0;
	size_t freeSize  = 0;

public:
	void GrabLock() { bmpMutex.lock(); }
	void FreeLock() { bmpMutex.unlock(); }

	spring::mutex& GetMutex() { return bmpMutex; }

	size_t Size() const { return (memArray.size()); }
	size_t AllocIdx(size_t size) { return (Alloc(size) - Base()); }
	size_t AllocIdxRaw(size_t size) { return (AllocRaw(size) - Base()); }

	uint8_t* Base() { return (memArray.data()); }
	uint8_t* Alloc(size_t size) {
		std::lock_guard<spring::mutex> lck(bmpMutex);
		return (AllocRaw(size));
	}

	uint8_t* AllocRaw(size_t size) {
		#if (ENABLE_TEXMEMPOOL == 0)
		uint8_t* mem = new uint8_t[size];
		#else
		uint8_t* mem = nullptr;

		size_t bestPair = size_t(-1);
		size_t bestSize = size_t(-1);
		size_t diffSize = size_t(-1);

		for (bool runDefrag: {true, false}) {
			bestPair = size_t(-1);
			bestSize = size_t(-1);
			diffSize = size_t(-1);

			// find chunk with smallest size difference
			for (size_t i = 0, n = freeList.size(); i < n; i++) {
				if (freeList[i].second < size)
					continue;

				if ((freeList[i].second - size) > diffSize)
					continue;

				bestSize = freeList[bestPair = i].second;
				diffSize = std::min(bestSize - size, diffSize);
			}

			if (bestPair == size_t(-1)) {
				if (runDefrag && DefragRaw())
					continue;

				// give up
				LOG_L(L_ERROR, "[TexMemPool::%s] failed to allocate bitmap of size " _STPF_ "u from pool of total size " _STPF_ "u", __func__, size, Size());
				throw std::bad_alloc();
				return mem;
			}

			break;
		}

		mem = &memArray[freeList[bestPair].first];

		if (bestSize > size) {
			freeList[bestPair].first += size;
			freeList[bestPair].second -= size;
		} else {
			// exact match, erase
			freeList[bestPair] = freeList.back();
			freeList.pop_back();
		}

		numAllocs += 1;
		allocSize += size;
		#endif
		return mem;
	}


	void Free(uint8_t* mem, size_t size) {
		std::lock_guard<spring::mutex> lck(bmpMutex);
		FreeRaw(mem, size);
	}

	void FreeRaw(uint8_t* mem, size_t size) {
		#if (ENABLE_TEXMEMPOOL == 0)
		delete[] mem;
		#else
		if (mem == nullptr)
			return;

		assert(size != 0);
		memset(mem, 0, size);
		freeList.emplace_back(mem - &memArray[0], size);

		#if 0
		{
			// check if freed mem overlaps any existing chunks
			const FreePair& p = freeList.back();

			for (size_t i = 0, n = freeList.size() - 1; i < n; i++) {
				const FreePair& c = freeList[i];

				assert(!((p.first < c.first) && (p.first + p.second) > c.first));
				assert(!((c.first < p.first) && (c.first + c.second) > p.first));
			}
		}
		#endif

		numFrees += 1;
		freeSize += size;
		allocSize -= size;

		// most bitmaps are transient, so keep the list short
		// longer-lived textures should be allocated ASAP s.t.
		// rest of the pool remains unfragmented
		// TODO: split into power-of-two subpools?
		if (freeList.size() >= 64 || freeSize >= (memArray.size() >> 4))
			DefragRaw();
		#endif
	}


	void Dispose() {
		freeList = {};
		memArray = {};

		numAllocs = 0;
		allocSize = 0;
		numFrees  = 0;
		freeSize  = 0;
	}
	void Resize(size_t size) {
		std::lock_guard<spring::mutex> lck(bmpMutex);

		if (memArray.empty()) {
			freeList.reserve(32);
			freeList.emplace_back(0, size);

			memArray.resize(size, 0);
		} else {
			assert(size > Size());

			freeList.emplace_back(Size(), size - Size());
			memArray.resize(size, 0);
		}

		LOG_L(L_INFO, "[TexMemPool::%s] poolSize=" _STPF_ "u allocSize=" _STPF_ "u texCount=" _STPF_ "u", __func__, size, allocSize, numAllocs - numFrees);
	}


	bool Defrag() {
		if (freeList.empty())
			return false;

		std::lock_guard<spring::mutex> lck(bmpMutex);
		return (DefragRaw());
	}

	bool DefragRaw() {
		const auto sortPred = [](const FreePair& a, const FreePair& b) { return (a.first < b.first); };
		const auto accuPred = [](const FreePair& a, const FreePair& b) { return FreePair{0, a.second + b.second}; };

		std::sort(freeList.begin(), freeList.end(), sortPred);

		// merge adjacent chunks
		for (size_t i = 0, n = freeList.size(); i < n; /**/) {
			FreePair& currPair = freeList[i++];

			for (size_t j = i; j < n; j++) {
				FreePair& nextPair = freeList[j];

				assert(!((currPair.first + currPair.second) > nextPair.first));

				if ((currPair.first + currPair.second) != nextPair.first)
					break;

				currPair.second += nextPair.second;
				nextPair.second = 0;

				i += 1;
			}
		}


		size_t i = 0;
		size_t j = 0;

		// cleanup zero-length chunks
		while (i < freeList.size()) {
			freeList[j] = freeList[i];

			j += (freeList[i].second != 0);
			i += 1;
		}

		if (j >= freeList.size())
			return false;

		// shrink
		freeList.resize(j);

		const auto freeBeg  = freeList.begin();
		const auto freeEnd  = freeList.end();
		      auto freePair = FreePair{0, 0};

		freePair = std::accumulate(freeBeg, freeEnd, freePair, accuPred);
		freeSize = freePair.second;
		return true;
	}
};

static TexMemPool texMemPool;


// this is a minimal list of file formats that (should) be available at all platforms
static constexpr int formatList[] = {
	IL_PNG, IL_JPG, IL_TGA, IL_DDS, IL_BMP, IL_TIF, IL_HDR, IL_EXR,
	IL_RGBA, IL_RGB, IL_BGRA, IL_BGR,
	IL_COLOUR_INDEX, IL_LUMINANCE, IL_LUMINANCE_ALPHA
};

static bool IsValidImageFormat(int format) {
	return std::find(std::cbegin(formatList), std::cend(formatList), format) != std::cend(formatList);
}

//////////////////////////////////////////////////////////////////////
// BitmapAction
//////////////////////////////////////////////////////////////////////

#ifndef HEADLESS
class BitmapAction {
public:
	BitmapAction() = delete;
	BitmapAction(CBitmap* bmp_)
		: bmp{ bmp_ }
	{}

	BitmapAction(const BitmapAction& ba) = delete;
	BitmapAction(BitmapAction&& ba) noexcept = delete;

	BitmapAction& operator=(const BitmapAction& ba) = delete;
	BitmapAction& operator=(BitmapAction&& ba) noexcept = delete;

	virtual void CreateAlpha(uint8_t red, uint8_t green, uint8_t blue) = 0;
	virtual void ReplaceAlpha(float a) = 0;
	virtual void SetTransparent(const SColor& c, const SColor trans = SColor(0, 0, 0, 0)) = 0;

	virtual void Renormalize(const float3& newCol) = 0;
	virtual void Blur(int iterations = 1, float weight = 1.0f) = 0;
	virtual void Fill(const SColor& c) = 0;

	virtual void InvertColors() = 0;
	virtual void InvertAlpha() = 0;
	virtual void MakeGrayScale() = 0;
	virtual void Tint(const float tint[3]) = 0;

	virtual CBitmap CreateRescaled(int newx, int newy) = 0;

	static std::unique_ptr<BitmapAction> GetBitmapAction(CBitmap* bmp);
protected:
	CBitmap* bmp;
};

template<typename T, uint32_t ch>
class TBitmapAction : public BitmapAction {
public:
	static constexpr size_t PixelTypeSize = sizeof(T) * ch;

	using ChanType  = T;
	using PixelType = T[ch];

	using AccumChanType = typename std::conditional<std::is_same_v<T, float>, float, uint32_t>::type;

	using ChanTypeRep  = uint8_t[sizeof(T) *  1];
	using PixelTypeRep = uint8_t[PixelTypeSize ];
public:
	TBitmapAction() = delete;
	TBitmapAction(CBitmap* bmp_)
		: BitmapAction(bmp_)
	{}

	constexpr const ChanType GetMaxNormValue() const {
		if constexpr (std::is_same_v<T, float>) {
			return 1.0f;
		}
		else {
			return std::numeric_limits<T>::max();
		}
	}

	PixelType& GetRef(uint32_t xyOffset) {
		auto* mem = bmp->GetRawMem();
		assert(mem && xyOffset >= 0 && xyOffset <= bmp->GetMemSize() - sizeof(PixelTypeRep));
		//return *static_cast<PT*>(static_cast<PTR*>(mem[xyOffset]));
		return *(reinterpret_cast<PixelType*>(&mem[PixelTypeSize * xyOffset]));
	}

	ChanType& GetRef(uint32_t xyOffset, uint32_t chan) {
		assert(chan >= 0 && chan < 4);
		return GetRef(xyOffset)[chan];
	}

	void CreateAlpha(uint8_t red, uint8_t green, uint8_t blue) override;
	void ReplaceAlpha(float a) override;
	void SetTransparent(const SColor& c, const SColor trans) override;

	void Renormalize(const float3& newCol) override;
	void Blur(int iterations = 1, float weight = 1.0f) override;
	void Fill(const SColor& c) override;

	void InvertColors() override;
	void InvertAlpha() override;
	void MakeGrayScale() override;
	void Tint(const float tint[3]) override;

	CBitmap CreateRescaled(int newx, int newy) override;
};

//fugly way to make CH compile time constant
#define GET_BITMAP_ACTION_HELPER(CH) do { \
	if (bmp->channels == CH) { \
		switch (bmp->dataType) { \
			case GL_FLOAT         : { \
				return std::make_unique<TBitmapAction<float   , CH>>(bmp); \
			} break; \
			case GL_UNSIGNED_SHORT: { \
				return std::make_unique<TBitmapAction<uint16_t, CH>>(bmp); \
			} break; \
			case GL_UNSIGNED_BYTE : { \
				return std::make_unique<TBitmapAction<uint8_t , CH>>(bmp); \
			} break; \
		} \
	} \
} while (0)

std::unique_ptr<BitmapAction> BitmapAction::GetBitmapAction(CBitmap* bmp)
{
	GET_BITMAP_ACTION_HELPER(4);
	GET_BITMAP_ACTION_HELPER(3);
	GET_BITMAP_ACTION_HELPER(2);
	GET_BITMAP_ACTION_HELPER(1);

	assert(false);
	return nullptr;
}

#undef GET_BITMAP_ACTION_HELPER

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::CreateAlpha(uint8_t red, uint8_t green, uint8_t blue)
{
	//if constexpr needed here to avoid compilation errors
	if constexpr (ch != 4) {
		assert(false);
		return;
	}
	else {
		const ChanType N = GetMaxNormValue();
		const float4 fRGBA = SColor{ red, green, blue, 0u };
		const PixelType tRGBA = {
			static_cast<ChanType>(fRGBA.r * N),
			static_cast<ChanType>(fRGBA.g * N),
			static_cast<ChanType>(fRGBA.b * N),
			static_cast<ChanType>(fRGBA.a * N)
		};

		float3 aCol;
		for (int a = 0; a < 3; ++a) {
			float cCol = 0.0f;
			int numCounted = 0;

			for (int y = 0; y < bmp->ysize; ++y) {
				int32_t yOffset = (y * bmp->xsize);
				for (int x = 0; x < bmp->xsize; ++x) {
					auto& pixel = GetRef(yOffset + x);

					if (pixel[3] == ChanType{ 0 })
						continue;
					if (pixel[0] == tRGBA[0] && pixel[1] == tRGBA[1] && pixel[2] == tRGBA[2])
						continue;

					cCol += static_cast<float>(pixel[a]);
					numCounted += 1;
				}
			}

			if (numCounted != 0)
				aCol[a] = static_cast<float>(cCol / GetMaxNormValue() / numCounted);
		}

		const SColor c(red, green, blue);
		const SColor a(aCol.x, aCol.y, aCol.z, 0.0f);
		SetTransparent(c, a);
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::ReplaceAlpha(float a)
{
	if (ch != 4) {
		assert(false);
		return;
	}

	for (int32_t y = 0; y < bmp->ysize; ++y) {
		int32_t yOffset = (y * bmp->xsize);
		for (int32_t x = 0; x < bmp->xsize; ++x) {
			auto& alpha = GetRef(yOffset + x, ch - 1);
			alpha = static_cast<ChanType>(GetMaxNormValue() * a);
		}
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::SetTransparent(const SColor& c, const SColor t)
{
	//if constexpr needed here to avoid compilation errors
	if constexpr (ch != 4) {
		assert(false);
		return;
	}
	else {
		const ChanType N = GetMaxNormValue();

		const float4 fC = c;
		const float4 fT = t;
		const PixelType tC = {
			static_cast<ChanType>(fC.r * N),
			static_cast<ChanType>(fC.g * N),
			static_cast<ChanType>(fC.b * N),
			static_cast<ChanType>(fC.a * N)
		};
		const PixelType tT = {
			static_cast<ChanType>(fT.r * N),
			static_cast<ChanType>(fT.g * N),
			static_cast<ChanType>(fT.b * N),
			static_cast<ChanType>(fT.a * N)
		};

		for (int y = 0; y < bmp->ysize; ++y) {
			int32_t yOffset = (y * bmp->xsize);
			for (int x = 0; x < bmp->xsize; ++x) {
				auto& pixel = GetRef(yOffset + x);

				if (pixel[0] == tC[0] && pixel[1] == tC[1] && pixel[2] == tC[2]) {
					pixel[0] =  tT[0];   pixel[1] =  tT[1];   pixel[2] =  tT[2];
				}
			}
		}
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::Renormalize(const float3& newCol)
{
	if (ch != 4) {
		assert(false);
		return;
	}

	float3 aCol;
	float3 colorDif;

	for (int a = 0; a < 3; ++a) {
		float cCol = 0.0f;
		int numCounted = 0;

		for (int y = 0; y < bmp->ysize; ++y) {
			int32_t yOffset = (y * bmp->xsize);
			for (int x = 0; x < bmp->xsize; ++x) {
				auto& pixel = GetRef(yOffset + x);

				if (pixel[3] != ChanType{ 0 }) {
					cCol += static_cast<float>(pixel[a]);
					numCounted += 1;
				}
			}
		}

		if (numCounted != 0)
			aCol[a] = static_cast<float>(cCol / GetMaxNormValue() / numCounted);

		//cCol /= xsize*ysize; //??
		colorDif[a] = newCol[a] - aCol[a];
	}

	for (int a = 0; a < 3; ++a) {
		for (int y = 0; y < bmp->ysize; ++y) {
			int32_t yOffset = (y * bmp->xsize);
			for (int x = 0; x < bmp->xsize; ++x) {
				auto& pixel = GetRef(yOffset + x);

				float nc = static_cast<float>(pixel[a]) / GetMaxNormValue() + colorDif[a];
				pixel[a] = static_cast<ChanType>(std::max(0.0f, nc * GetMaxNormValue()));
			}
		}
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::Blur(int iterations, float weight)
{
	static constexpr float blurkernel[9] = {
		1.0f / 16.0f, 2.0f / 16.0f, 1.0f / 16.0f,
		2.0f / 16.0f, 4.0f / 16.0f, 2.0f / 16.0f,
		1.0f / 16.0f, 2.0f / 16.0f, 1.0f / 16.0f
	};

	CBitmap tmp(nullptr, bmp->xsize, bmp->ysize, bmp->channels, bmp->dataType);

	CBitmap* src =  bmp;
	CBitmap* dst = &tmp;

	//don't use "this" here
	auto srcAction = BitmapAction::GetBitmapAction(src);
	auto dstAction = BitmapAction::GetBitmapAction(dst);

	using ThisType = decltype(this);

	for (int i = 0; i < iterations; ++i) {
		for_mt(0, src->ysize, [&](const int y) {
			for (int x = 0; x < src->xsize; x++) {
				int yBaseOffset = (y * src->xsize);
				for (int a = 0; a < src->channels; a++) {

					///////////////////////////////////////
					float fragment = 0.0f;

					for (int i = 0; i < 9; ++i) {
						int yoffset = (i / 3) - 1;
						int xoffset = (i - (yoffset + 1) * 3) - 1;

						const int tx = x + xoffset;
						const int ty = y + yoffset;

						xoffset *= ((tx >= 0) && (tx < src->xsize));
						yoffset *= ((ty >= 0) && (ty < src->ysize));

						const int offset = (yoffset * src->xsize + xoffset);

						auto& srcChannel = static_cast<ThisType>(srcAction.get())->GetRef(yBaseOffset + x + offset, a);

						if (i == 4)
							fragment += (weight * blurkernel[i] * srcChannel);
						else
							fragment += (         blurkernel[i] * srcChannel);
					}

					auto& dstChannel = static_cast<ThisType>(dstAction.get())->GetRef(yBaseOffset + x, a);

					if constexpr (std::is_same_v<ChanType, float>) {
						dstChannel = static_cast<ChanType>(std::max(fragment, 0.0f));
					}
					else {
						dstChannel = static_cast<ChanType>(std::clamp(fragment, 0.0f, static_cast<float>(GetMaxNormValue())));
					}
					///////////////////////////////////////
				}
			}
		});

		std::swap(srcAction, dstAction);
		std::swap(src, dst);
	}

	// if dst points to temporary, we are done
	// otherwise need to perform one more swap
	// (e.g. if iterations=1)
	if (dst != bmp)
		return;

	std::swap(src, dst);
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::Fill(const SColor& c)
{
	//if constexpr needed here to avoid compilation errors
	if constexpr (ch != 4) {
		assert(false);
		return;
	}
	else {
		const ChanType N = GetMaxNormValue();
		const float4 fRGBA = c;
		const PixelType tRGBA = {
			static_cast<ChanType>(fRGBA.r * N),
			static_cast<ChanType>(fRGBA.g * N),
			static_cast<ChanType>(fRGBA.b * N),
			static_cast<ChanType>(fRGBA.a * N)
		};

		for (uint32_t i = 0, n = bmp->xsize * bmp->ysize; i < n; i++) {
			auto& pixel = GetRef(i);
			memcpy(&pixel, &tRGBA, PixelTypeSize);
		}
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::InvertColors()
{
	if (ch != 4) {
		assert(false);
		return;
	}

	for (int y = 0; y < bmp->ysize; ++y) {
		uint32_t yOffset = (y * bmp->xsize);
		for (int x = 0; x < bmp->xsize; ++x) {
			auto& pixel = GetRef(yOffset + x);

			// do not invert alpha
			for (int a = 0; a < ch - 1; ++a)
				pixel[a] = GetMaxNormValue() - std::clamp(pixel[a], ChanType{ 0 }, GetMaxNormValue());
		}
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::InvertAlpha()
{
	if (ch != 4) {
		assert(false);
		return;
	}

	for (int y = 0; y < bmp->ysize; ++y) {
		uint32_t yOffset = (y * bmp->xsize);
		for (int x = 0; x < bmp->xsize; ++x) {
			auto& pixel = GetRef(yOffset + x);

			pixel[ch - 1] = GetMaxNormValue() - std::clamp(pixel[ch - 1], ChanType{ 0 }, GetMaxNormValue());
		}
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::MakeGrayScale()
{
	if (ch != 4) {
		assert(false);
		return;
	}

	const ChanType N = GetMaxNormValue();

	for (int y = 0; y < bmp->ysize; ++y) {
		uint32_t yOffset = (y * bmp->xsize);
		for (int x = 0; x < bmp->xsize; ++x) {
			auto& pixel = GetRef(yOffset + x);

			float3 rgb = {
				static_cast<float>(pixel[0]) / N,
				static_cast<float>(pixel[1]) / N,
				static_cast<float>(pixel[2]) / N
			};

			const float luma =
				(rgb.r * 0.299f) +
				(rgb.g * 0.587f) +
				(rgb.b * 0.114f);

			const AccumChanType val = std::max(
				static_cast<AccumChanType>((256.0f / 255.0f) * luma),
				AccumChanType(0)
			);

			if constexpr (std::is_same_v<ChanType, float>) {
				pixel[0] = val;
				pixel[1] = val;
				pixel[2] = val;
			}
			else {
				const ChanType cval = static_cast<ChanType>( std::min(val, static_cast<AccumChanType>(N)) );
				pixel[0] = cval;
				pixel[1] = cval;
				pixel[2] = cval;
			}
		}
	}
}

template<typename T, uint32_t ch>
void TBitmapAction<T, ch>::Tint(const float tint[3])
{
	if (ch != 4) {
		assert(false);
		return;
	}

	const AccumChanType N = GetMaxNormValue();

	for (int y = 0; y < bmp->ysize; ++y) {
		uint32_t yOffset = (y * bmp->xsize);
		for (int x = 0; x < bmp->xsize; ++x) {
			auto& pixel = GetRef(yOffset + x);

			// don't touch the alpha channel
			for (int a = 0; a < ch - 1; ++a) {
				AccumChanType val = pixel[a] * tint[a];
				if constexpr (std::is_same_v<ChanType, float>) {
					pixel[a] = static_cast<ChanType>(std::max  (val, AccumChanType{ 0 }   ));
				}
				else {
					pixel[a] = static_cast<ChanType>(std::clamp(val, AccumChanType{ 0 }, N));
				}
			}
		}
	}
}

template<typename T, uint32_t ch>
CBitmap TBitmapAction<T, ch>::CreateRescaled(int newx, int newy)
{
	CBitmap dst;

	if (ch != 4) {
		assert(false);
		dst.AllocDummy();
		return dst;
	}

	using ThisType = decltype(this);
	const AccumChanType N = GetMaxNormValue();

	dst.Alloc(newx, newy, bmp->channels, bmp->dataType);
	auto dstAction = BitmapAction::GetBitmapAction(&dst);

	const float dx = static_cast<float>(bmp->xsize) / static_cast<float>(newx);
	const float dy = static_cast<float>(bmp->ysize) / static_cast<float>(newy);

	float cy = 0;
	for (int y = 0; y < newy; ++y) {
		const int sy = (int)cy;
		cy += dy;
		int ey = (int)cy;
		if (ey == sy)
			ey = sy + 1;

		float cx = 0;
		for (int x = 0; x < newx; ++x) {
			const int sx = (int)cx;
			cx += dx;
			int ex = (int)cx;
			if (ex == sx)
				ex = sx + 1;

			std::array<AccumChanType, ch> rgba = {0};

			for (int y2 = sy; y2 < ey; ++y2) {
				for (int x2 = sx; x2 < ex; ++x2) {
					const int index = y2 * bmp->xsize + x2;
					auto& srcPixel = GetRef(index);

					for (int a = 0; a < ch; ++a)
						rgba[a] += srcPixel[a];
				}
			}
			const int denom = ((ex - sx) * (ey - sy));

			const int index = (y * dst.xsize + x);
			auto& dstPixel = static_cast<ThisType>(dstAction.get())->GetRef(index);

			for (int a = 0; a < ch; ++a) {
				if constexpr (std::is_same_v<ChanType, float>) {
					dstPixel[a] = static_cast<ChanType>(std::max  (rgba[a] / denom, AccumChanType{ 0 }   ));
				}
				else {
					dstPixel[a] = static_cast<ChanType>(std::clamp(rgba[a] / denom, AccumChanType{ 0 }, N));
				}
			}
		}
	}

	return dst;
}
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBitmap::~CBitmap()
{
	texMemPool.Free(GetRawMem(), GetMemSize());
}

CBitmap::CBitmap()
	: xsize(0)
	, ysize(0)
	, channels(4)
	, dataType(0x1401)
	, compressed(false)
{}

CBitmap::CBitmap(const uint8_t* data, int _xsize, int _ysize, int _channels, uint32_t reqDataType)
	: xsize(_xsize)
	, ysize(_ysize)
	, channels(_channels)
	, dataType(reqDataType == 0 ? 0x1401/*GL_UNSIGNED_BYTE*/ : reqDataType)
	, compressed(false)
{
	assert(GetMemSize() > 0);
	memIdx = texMemPool.AllocIdx(GetMemSize());

	if (data != nullptr) {
		assert(!((GetRawMem() < data) && (GetRawMem() + GetMemSize()) > data));
		assert(!((data < GetRawMem()) && (data + GetMemSize()) > GetRawMem()));

		std::memcpy(GetRawMem(), data, GetMemSize());
	} else {
		std::memset(GetRawMem(), 0, GetMemSize());
	}
}


CBitmap& CBitmap::operator=(const CBitmap& bmp)
{
	if (this != &bmp) {
		// NB: Free preserves size for asserts
		texMemPool.Free(GetRawMem(), GetMemSize());

		if (bmp.GetRawMem() != nullptr) {
			assert(!bmp.compressed);
			assert(bmp.GetMemSize() != 0);

			assert(!((    GetRawMem() < bmp.GetRawMem()) && (    GetRawMem() +     GetMemSize()) > bmp.GetRawMem()));
			assert(!((bmp.GetRawMem() <     GetRawMem()) && (bmp.GetRawMem() + bmp.GetMemSize()) >     GetRawMem()));

			memIdx = texMemPool.AllocIdx(bmp.GetMemSize());

			std::memcpy(GetRawMem(), bmp.GetRawMem(), bmp.GetMemSize());
		} else {
			memIdx = size_t(-1);
		}

		xsize = bmp.xsize;
		ysize = bmp.ysize;
		channels = bmp.channels;
		dataType = bmp.dataType;
		compressed = bmp.compressed;

		#ifndef HEADLESS
		textype = bmp.textype;

		ddsimage = bmp.ddsimage;
		#endif
	}

	assert(GetMemSize() == bmp.GetMemSize());
	assert((GetRawMem() != nullptr) == (bmp.GetRawMem() != nullptr));
	return *this;
}

CBitmap& CBitmap::operator=(CBitmap&& bmp) noexcept
{
	if (this != &bmp) {
		std::swap(memIdx, bmp.memIdx);
		std::swap(xsize, bmp.xsize);
		std::swap(ysize, bmp.ysize);
		std::swap(channels, bmp.channels);
		std::swap(dataType, bmp.dataType);
		std::swap(compressed, bmp.compressed);

		#ifndef HEADLESS
		std::swap(textype, bmp.textype);

		std::swap(ddsimage, bmp.ddsimage);
		#endif
	}

	return *this;
}


void CBitmap::InitPool(size_t size)
{
	// only allow expansion; config-size is in MB
	size *= (1024 * 1024);

	if (size > texMemPool.Size())
		texMemPool.Resize(size);

	texMemPool.Defrag();
}


const uint8_t* CBitmap::GetRawMem() const { return ((memIdx == size_t(-1))? nullptr: (texMemPool.Base() + memIdx)); }
      uint8_t* CBitmap::GetRawMem()       { return ((memIdx == size_t(-1))? nullptr: (texMemPool.Base() + memIdx)); }


void CBitmap::Alloc(int w, int h, int c, uint32_t glType)
{
	if (!Empty())
		texMemPool.Free(GetRawMem(), GetMemSize());

	memIdx = texMemPool.AllocIdx((xsize = w) * (ysize = h) * (channels = c));
	memset(GetRawMem(), 0, GetMemSize());
}

void CBitmap::AllocDummy(const SColor fill)
{
	compressed = false;

	Alloc(1, 1, sizeof(SColor), dataType);
	Fill(fill);
}

#ifndef HEADLESS
int32_t CBitmap::GetIntFmt() const
{
	constexpr uint32_t intFormats[3][5] = {
			{ 0, GL_R8   , GL_RG8  , GL_RGB8  , GL_RGBA8   },
			{ 0, GL_R16  , GL_RG16 , GL_RGB16 , GL_RGBA16  },
			{ 0, GL_R32F , GL_RG32F, GL_RGB32F, GL_RGBA32F }
	};
	switch (dataType) {
	case GL_FLOAT:
		return intFormats[2][channels];
	case GL_UNSIGNED_SHORT:
		return intFormats[1][channels];
	case GL_UNSIGNED_BYTE:
		return intFormats[0][channels];
	default:
		assert(false);
		return 0;
	}
}

int32_t CBitmap::GetExtFmt(uint32_t ch)
{
	constexpr uint32_t extFormats[] = { 0, GL_RED, GL_RG , GL_RGB , GL_RGBA }; // GL_R is not accepted for [1]
	return extFormats[ch];
}

int32_t CBitmap::ExtFmtToChannels(int32_t extFmt)
{
	switch (extFmt) {
	case GL_RED:
		return 1;
	case GL_RG:
		return 2;
	case GL_RGB:
		return 3;
	case GL_RGBA:
		return 4;
	default:
		assert(false);
		return 0;
	}
}

uint32_t CBitmap::GetDataTypeSize() const
{
	switch (dataType) {
	case GL_FLOAT:
		return sizeof(float);
	case GL_UNSIGNED_SHORT:
		return sizeof(uint16_t);
	case GL_UNSIGNED_BYTE:
		return sizeof(uint8_t);
	default:
		assert(false);
		return 0;
	}
}
#else
int32_t CBitmap::GetIntFmt() const { return 0; }
int32_t CBitmap::GetExtFmt(uint32_t ch) { return 0; }
int32_t CBitmap::ExtFmtToChannels(int32_t extFmt) { return 0; }
uint32_t CBitmap::GetDataTypeSize() const { return 0; }
#endif

bool CBitmap::Load(std::string const& filename, float defaultAlpha, uint32_t reqChannel, uint32_t reqDataType)
{
	bool isLoaded = false;
	bool isValid  = false;
	bool noAlpha  =  true;

	// LHS is only true for "image.dds", "IMAGE.DDS" would be loaded by IL
	// which does not vertically flip DDS images by default, unlike nv_dds
	// most Spring games do not seem to store DDS buildpics pre-flipped so
	// files ending in ".DDS" would appear upside-down if loaded by nv_dds
	//
	// const bool loadDDS = (filename.find(".dds") != std::string::npos || filename.find(".DDS") != std::string::npos);
	const bool loadDDS = (FileSystem::GetExtension(filename) == "dds"); // always lower-case
	const bool flipDDS = (filename.find("unitpics") == std::string::npos); // keep buildpics as-is

	const size_t curMemSize = GetMemSize();


	channels = 4;
	#ifndef HEADLESS
	textype = GL_TEXTURE_2D;
	#endif

	#define BITMAP_USE_NV_DDS
	#ifdef BITMAP_USE_NV_DDS
	if (loadDDS) {
		#ifndef HEADLESS
		compressed = true;
		xsize = 0;
		ysize = 0;
		channels = 0;

		ddsimage.clear();
		if (!ddsimage.load(filename, flipDDS))
			return false;

		xsize = ddsimage.get_width();
		ysize = ddsimage.get_height();
		channels = ddsimage.get_components();
		switch (ddsimage.get_type()) {
			case nv_dds::TextureFlat :
				textype = GL_TEXTURE_2D;
				break;
			case nv_dds::Texture3D :
				textype = GL_TEXTURE_3D;
				break;
			case nv_dds::TextureCubemap :
				textype = GL_TEXTURE_CUBE_MAP;
				break;
			case nv_dds::TextureNone :
			default :
				break;
		}
		return true;
		#else
		// allocate a dummy texture, dds aren't supported in headless
		AllocDummy();
		return true;
		#endif
	}

	compressed = false;
	#else
	compressed = loadDDS;
	#endif


	CFileHandler file(filename);
	std::vector<uint8_t> buffer;

	if (!file.FileExists()) {
		AllocDummy();
		return false;
	}

	if (!file.IsBuffered()) {
		buffer.resize(file.FileSize(), 0);
		file.Read(buffer.data(), buffer.size());
	} else {
		// steal if file was loaded from VFS
		buffer = std::move(file.GetBuffer());
	}


	{
		std::lock_guard<spring::mutex> lck(texMemPool.GetMutex());

		// do not preserve the image origin since IL does not
		// vertically flip DDS images by default, unlike nv_dds
		ilOriginFunc((loadDDS && flipDDS)? IL_ORIGIN_LOWER_LEFT: IL_ORIGIN_UPPER_LEFT);
		ilEnable(IL_ORIGIN_SET);

		ILuint imageID = 0;
		ilGenImages(1, &imageID);
		ilBindImage(imageID);

		ILint currFormat;
		{
			// do not signal floating point exceptions in devil library
			ScopedDisableFpuExceptions fe;

			isLoaded = !!ilLoadL(IL_TYPE_UNKNOWN, buffer.data(), buffer.size());
			currFormat = ilGetInteger(IL_IMAGE_FORMAT);
			isValid = (isLoaded && IsValidImageFormat(currFormat));
			//noAlpha = (isValid && (ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL) != 4));
			noAlpha = (isValid && currFormat == 0x1907/*GL_RGB*/);
			dataType = ilGetInteger(IL_IMAGE_TYPE);

			// FPU control word has to be restored as well
			streflop::streflop_init<streflop::Simple>();
		}

		if (isValid) {
			{
				// conditional transformation
				ILenum dstFormat;
				if (reqChannel == 0) {
					dstFormat = currFormat;
					channels = ExtFmtToChannels(dstFormat);
				}
				else {
					dstFormat = GetExtFmt(reqChannel);
					channels = reqChannel;
				}

				if (reqDataType != 0)
					dataType = reqDataType;

				ilConvertImage(dstFormat, dataType);
			}

			xsize = ilGetInteger(IL_IMAGE_WIDTH);
			ysize = ilGetInteger(IL_IMAGE_HEIGHT);
			// format = ilGetInteger(IL_IMAGE_FORMAT);

			texMemPool.FreeRaw(GetRawMem(), curMemSize);
			memIdx = texMemPool.AllocIdxRaw(GetMemSize());

			// ilCopyPixels(0, 0, 0, xsize, ysize, 0, IL_RGBA, IL_UNSIGNED_BYTE, GetRawMem());
			for (const ILubyte* imgData = ilGetData(); imgData != nullptr; imgData = nullptr) {
				std::memset(GetRawMem(), 0xFF   , GetMemSize());
				std::memcpy(GetRawMem(), imgData, GetMemSize());
			}
		}

		ilDisable(IL_ORIGIN_SET);
		ilDeleteImages(1, &imageID);
	}

	// has to be outside the mutex scope; AllocDummy will acquire it again and
	// LOG can indirectly cause other bitmaps to be loaded through FontTexture
	if (!isValid) {
		LOG_L(L_ERROR, "[BMP::%s] invalid bitmap \"%s\" (loaded=%d)", __func__, filename.c_str(), isLoaded);
		AllocDummy();
		return false;
	}

	if (noAlpha)
		ReplaceAlpha(defaultAlpha);

	return true;
}


bool CBitmap::LoadGrayscale(const std::string& filename)
{
	const size_t curMemSize = GetMemSize();

	compressed = false;
	channels = 1;


	CFileHandler file(filename);

	if (!file.FileExists())
		return false;

	std::vector<uint8_t> buffer;

	if (!file.IsBuffered()) {
		buffer.resize(file.FileSize() + 1, 0);
		file.Read(buffer.data(), file.FileSize());
	} else {
		// steal if file was loaded from VFS
		buffer = std::move(file.GetBuffer());
	}

	{
		std::lock_guard<spring::mutex> lck(texMemPool.GetMutex());

		ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
		ilEnable(IL_ORIGIN_SET);

		ILuint imageID = 0;
		ilGenImages(1, &imageID);
		ilBindImage(imageID);

		const bool success = !!ilLoadL(IL_TYPE_UNKNOWN, buffer.data(), buffer.size());
		ilDisable(IL_ORIGIN_SET);

		if (!success)
			return false;

		ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
		xsize = ilGetInteger(IL_IMAGE_WIDTH);
		ysize = ilGetInteger(IL_IMAGE_HEIGHT);

		texMemPool.FreeRaw(GetRawMem(), curMemSize);
		memIdx = texMemPool.AllocIdxRaw(GetMemSize());

		for (const ILubyte* imgData = ilGetData(); imgData != nullptr; imgData = nullptr) {
			std::memset(GetRawMem(), 0xFF, GetMemSize());
			std::memcpy(GetRawMem(), imgData, GetMemSize());
		}

		ilDeleteImages(1, &imageID);
	}

	return true;
}


bool CBitmap::Save(std::string const& filename, unsigned quality, bool opaque, bool logged) const
{
	if (compressed) {
		#ifndef HEADLESS
		return ddsimage.save(filename);
		#else
		return false;
		#endif
	}

	if (GetMemSize() == 0)
		return false;


	std::lock_guard<spring::mutex> lck(texMemPool.GetMutex());

	const uint8_t* mem = GetRawMem();
	      uint8_t* buf = texMemPool.AllocRaw(xsize * ysize * 4);

	/* HACK Flip the image so it saves the right way up.
		(Fiddling with ilOriginFunc didn't do anything?)
		Duplicated with ReverseYAxis. */
	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			const int bi = 4 * (x + (xsize * ((ysize - 1) - y)));
			const int mi = 4 * (x + (xsize * (              y)));

			for (int ch = 0; ch < 3; ++ch) {
				buf[bi + ch] = (ch < channels) ? mem[mi + ch] : 0xFF;
			}

			buf[bi + 3] = (!opaque && channels == 4) ? mem[mi + 3] : 0xFF;
		}
	}

	// clear any previous errors
	while (ilGetError() != IL_NO_ERROR);

	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	ilHint(IL_COMPRESSION_HINT, IL_NO_COMPRESSION);
	ilSetInteger(IL_JPG_QUALITY, quality);

	ILuint imageID = 0;
	ilGenImages(1, &imageID);
	ilBindImage(imageID);
	ilTexImage(xsize, ysize, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, buf);
	assert(ilGetError() == IL_NO_ERROR);

	texMemPool.FreeRaw(buf, xsize * ysize * 4);


	const std::string& fsImageExt = FileSystem::GetExtension(filename);
	const std::string& fsFullPath = dataDirsAccess.LocateFile(filename, FileQueryFlags::WRITE);
	const std::wstring& ilFullPath = std::wstring(fsFullPath.begin(), fsFullPath.end());

	bool success = false;

	if (logged)
		LOG("[CBitmap::%s] saving \"%s\" to \"%s\" (IL_VERSION=%d IL_UNICODE=%d)", __func__, filename.c_str(), fsFullPath.c_str(), IL_VERSION, sizeof(ILchar) != 1);

	if (sizeof(void*) >= 4) {
		#if 0
		// NOTE: all Windows buildbot libIL's crash in ilSaveF (!)
		std::vector<ILchar> ilFullPath(fsFullPath.begin(), fsFullPath.end());

		// null-terminate; vectors are not strings
		ilFullPath.push_back(0);

		// IL might be unicode-aware in which case it uses wchar_t{*} strings
		// should not even be necessary because ASCII and UTFx are compatible
		switch (sizeof(ILchar)) {
			case (sizeof( char  )): {                                                                                                     } break;
			case (sizeof(wchar_t)): { std::mbstowcs(reinterpret_cast<wchar_t*>(ilFullPath.data()), fsFullPath.data(), fsFullPath.size()); } break;
			default: { assert(false); } break;
		}
		#endif

		const ILchar* p = (sizeof(ILchar) != 1)?
			reinterpret_cast<const ILchar*>(ilFullPath.data()):
			reinterpret_cast<const ILchar*>(fsFullPath.data());

		switch (int(fsImageExt[0])) {
			case 'b': case 'B': { success = ilSave(IL_BMP, p); } break;
			case 'j': case 'J': { success = ilSave(IL_JPG, p); } break;
			case 'p': case 'P': { success = ilSave(IL_PNG, p); } break;
			case 't': case 'T': { success = ilSave(IL_TGA, p); } break;
			case 'd': case 'D': { success = ilSave(IL_DDS, p); } break;
		}
	} else {
		FILE* file = fopen(fsFullPath.c_str(), "wb");

		if (file != nullptr) {
			switch (int(fsImageExt[0])) {
				case 'b': case 'B': { success = ilSaveF(IL_BMP, file); } break;
				case 'j': case 'J': { success = ilSaveF(IL_JPG, file); } break;
				case 'p': case 'P': { success = ilSaveF(IL_PNG, file); } break;
				case 't': case 'T': { success = ilSaveF(IL_TGA, file); } break;
				case 'd': case 'D': { success = ilSaveF(IL_DDS, file); } break;
			}

			fclose(file);
		}
	}

	if (logged) {
		if (success) {
			LOG("[CBitmap::%s] saved \"%s\" to \"%s\"", __func__, filename.c_str(), fsFullPath.c_str());
		} else {
			LOG("[CBitmap::%s] error 0x%x saving \"%s\" to \"%s\"", __func__, ilGetError(), filename.c_str(), fsFullPath.c_str());
		}
	}

	ilDeleteImages(1, &imageID);
	ilDisable(IL_ORIGIN_SET);
	return success;
}


bool CBitmap::SaveGrayScale(const std::string& filename) const
{
	if (compressed)
		return false;

	CBitmap bmp = *this;

	for (uint8_t* mem = bmp.GetRawMem(); mem != nullptr; mem = nullptr) {
		// approximate luminance
		bmp.MakeGrayScale();

		// convert RGBA tuples to normalized FLT32 values expected by SaveFloat; GBA are destroyed
		for (int y = 0; y < ysize; ++y) {
			for (int x = 0; x < xsize; ++x) {
				*reinterpret_cast<float*>(&mem[(y * xsize + x) * 4]) = static_cast<float>(mem[(y * xsize + x) * 4 + 0] / 255.0f);
			}
		}

		// save FLT32 data in 16-bit ushort format
		return (bmp.SaveFloat(filename));
	}

	return false;
}


bool CBitmap::SaveFloat(std::string const& filename) const
{
	// must have four channels; each RGBA tuple is reinterpreted as a single FLT32 value
	if (GetMemSize() == 0 || channels != 4)
		return false;

	std::lock_guard<spring::mutex> lck(texMemPool.GetMutex());

	// seems IL_ORIGIN_SET only works in ilLoad and not in ilTexImage nor in ilSaveImage
	// so we need to flip the image ourselves
	const uint8_t* u8mem = GetRawMem();
	const float* f32mem = reinterpret_cast<const float*>(&u8mem[0]);

	uint16_t* u16mem = reinterpret_cast<uint16_t*>(texMemPool.AllocRaw(xsize * ysize * sizeof(uint16_t)));

	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			const int bi = x + (xsize * ((ysize - 1) - y));
			const int mi = x + (xsize * (              y));
			const uint16_t us = f32mem[mi] * 0xFFFF; // convert float 0..1 to ushort
			u16mem[bi] = us;
		}
	}

	ilHint(IL_COMPRESSION_HINT, IL_USE_COMPRESSION);
	ilSetInteger(IL_JPG_QUALITY, 80);

	ILuint imageID = 0;
	ilGenImages(1, &imageID);
	ilBindImage(imageID);
	// note: DevIL only generates a 16bit grayscale PNG when format is IL_UNSIGNED_SHORT!
	//       IL_FLOAT is converted to RGB with 8bit colordepth!
	ilTexImage(xsize, ysize, 1, 1, IL_LUMINANCE, IL_UNSIGNED_SHORT, u16mem);

	texMemPool.FreeRaw(reinterpret_cast<uint8_t*>(u16mem), xsize * ysize * sizeof(uint16_t));


	const std::string& fsImageExt = FileSystem::GetExtension(filename);
	const std::string& fsFullPath = dataDirsAccess.LocateFile(filename, FileQueryFlags::WRITE);

	FILE* file = fopen(fsFullPath.c_str(), "wb");
	bool success = false;

	if (file != nullptr) {
		switch (int(fsImageExt[0])) {
			case 'b': case 'B': { success = ilSaveF(IL_BMP, file); } break;
			case 'j': case 'J': { success = ilSaveF(IL_JPG, file); } break;
			case 'p': case 'P': { success = ilSaveF(IL_PNG, file); } break;
			case 't': case 'T': { success = ilSaveF(IL_TGA, file); } break;
			case 'd': case 'D': { success = ilSaveF(IL_DDS, file); } break;
		}

		fclose(file);
	}

	ilDeleteImages(1, &imageID);
	return success;
}


#ifndef HEADLESS
unsigned int CBitmap::CreateTexture(float aniso, float lodBias, bool mipmaps, uint32_t texID) const
{
	if (compressed)
		return CreateDDSTexture(texID, aniso, lodBias, mipmaps);

	if (GetMemSize() == 0)
		return 0;

	// jcnossen: Some drivers return "2.0" as a version string,
	// but switch to software rendering for non-power-of-two textures.
	// GL_ARB_texture_non_power_of_two indicates that the hardware will actually support it.
	if (!globalRendering->supportNonPowerOfTwoTex && (xsize != next_power_of_2(xsize) || ysize != next_power_of_2(ysize))) {
		CBitmap bm = CreateRescaled(next_power_of_2(xsize), next_power_of_2(ysize));
		return bm.CreateTexture(aniso, mipmaps);
	}

	if (texID == 0)
		glGenTextures(1, &texID);

	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (lodBias != 0.0f)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, lodBias);
	if (aniso > 0.0f)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);

	if (mipmaps) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glBuildMipmaps(GL_TEXTURE_2D, GetIntFmt(), xsize, ysize, GetExtFmt(), dataType, GetRawMem());
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GetIntFmt(), xsize, ysize, 0, GetExtFmt(), dataType, GetRawMem());
	}

	return texID;
}


static void HandleDDSMipmap(GLenum target, bool mipmaps, int num_mipmaps)
{
	if (num_mipmaps > 0) {
		// dds included the MipMaps use them
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	} else {
		if (mipmaps && IS_GL_FUNCTION_AVAILABLE(glGenerateMipmap)) {
			// create the mipmaps at runtime
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glGenerateMipmap(target);
		} else {
			// no mipmaps
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
	}
}

unsigned int CBitmap::CreateDDSTexture(unsigned int texID, float aniso, float lodBias, bool mipmaps) const
{
	glPushAttrib(GL_TEXTURE_BIT);

	if (texID == 0)
		glGenTextures(1, &texID);

	switch (ddsimage.get_type()) {
		case nv_dds::TextureNone:
			glDeleteTextures(1, &texID);
			texID = 0;
			break;

		case nv_dds::TextureFlat:    // 1D, 2D, and rectangle textures
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texID);

			if (!ddsimage.upload_texture2D(0, GL_TEXTURE_2D)) {
				glDeleteTextures(1, &texID);
				texID = 0;
				break;
			}

			if (lodBias != 0.0f)
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, lodBias);
			if (aniso > 0.0f)
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);

			HandleDDSMipmap(GL_TEXTURE_2D, mipmaps, ddsimage.get_num_mipmaps());
			break;

		case nv_dds::Texture3D:
			glEnable(GL_TEXTURE_3D);
			glBindTexture(GL_TEXTURE_3D, texID);

			if (!ddsimage.upload_texture3D()) {
				glDeleteTextures(1, &texID);
				texID = 0;
				break;
			}

			if (lodBias != 0.0f)
				glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_LOD_BIAS, lodBias);

			HandleDDSMipmap(GL_TEXTURE_3D, mipmaps, ddsimage.get_num_mipmaps());
			break;

		case nv_dds::TextureCubemap:
			glEnable(GL_TEXTURE_CUBE_MAP);
			glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

			if (!ddsimage.upload_textureCubemap()) {
				glDeleteTextures(1, &texID);
				texID = 0;
				break;
			}

			if (lodBias != 0.0f)
				glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_LOD_BIAS, lodBias);
			if (aniso > 0.0f)
				glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);

			HandleDDSMipmap(GL_TEXTURE_CUBE_MAP, mipmaps, ddsimage.get_num_mipmaps());
			break;

		default:
			assert(false);
			break;
	}

	glPopAttrib();
	return texID;
}
#else  // !HEADLESS

unsigned int CBitmap::CreateTexture(float aniso, float lodBias, bool mipmaps, uint32_t texID) const {
	return 0;
}

unsigned int CBitmap::CreateDDSTexture(unsigned int texID, float aniso, float lodBias, bool mipmaps) const {
	return 0;
}
#endif // !HEADLESS


void CBitmap::CreateAlpha(uint8_t red, uint8_t green, uint8_t blue)
{
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->CreateAlpha(red, green, blue);
#endif
}


void CBitmap::SetTransparent(const SColor& c, const SColor trans)
{
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->SetTransparent(c, trans);
#endif
}


void CBitmap::Renormalize(const float3& newCol)
{
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->Renormalize(newCol);
#endif
}

void CBitmap::Blur(int iterations, float weight)
{
#ifndef HEADLESS
	if (compressed)
		return;


	auto action = BitmapAction::GetBitmapAction(this);
	action->Blur(iterations, weight);
#endif
}


void CBitmap::Fill(const SColor& c)
{
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->Fill(c);
#endif
}

void CBitmap::ReplaceAlpha(float a)
{
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->ReplaceAlpha(a);
#endif
}


void CBitmap::CopySubImage(const CBitmap& src, int xpos, int ypos)
{
	if ((xpos + src.xsize) > xsize || (ypos + src.ysize) > ysize) {
		LOG_L(L_WARNING, "CBitmap::CopySubImage src image does not fit into dst!");
		return;
	}

	if (compressed || src.compressed) {
		LOG_L(L_WARNING, "CBitmap::CopySubImage can't copy compressed textures!");
		return;
	}

	const uint8_t* srcMem = src.GetRawMem();
	      uint8_t* dstMem =     GetRawMem();

	const auto dts = GetDataTypeSize();
	for (int y = 0; y < src.ysize; ++y) {
		const int pixelDst = (((ypos + y) *     xsize) + xpos) * channels * dts;
		const int pixelSrc = ((        y  * src.xsize) +    0) * channels * dts;

		// copy the whole line
		std::copy(&srcMem[pixelSrc], &srcMem[pixelSrc] + src.xsize * channels * dts, &dstMem[pixelDst]);
	}
}


CBitmap CBitmap::CanvasResize(const int newx, const int newy, const bool center) const
{
	CBitmap bm;

	if (xsize > newx || ysize > newy) {
		LOG_L(L_WARNING, "CBitmap::CanvasResize can only upscale (tried to resize %ix%i to %ix%i)!", xsize,ysize,newx,newy);
		bm.AllocDummy();
		return bm;
	}

	const int borderLeft = (center) ? (newx - xsize) / 2 : 0;
	const int borderTop  = (center) ? (newy - ysize) / 2 : 0;

	bm.Alloc(newx, newy, channels, dataType);
	bm.CopySubImage(*this, borderLeft, borderTop);

	return bm;
}


SDL_Surface* CBitmap::CreateSDLSurface()
{
	SDL_Surface* surface = nullptr;

	if (channels < 3 && GetDataTypeSize() != 1) {
		LOG_L(L_WARNING, "CBitmap::CreateSDLSurface works only with 24bit RGB and 32bit RGBA pictures!");
		return surface;
	}

	// this will only work with 24bit RGB and 32bit RGBA pictures
	// note: does NOT create a copy of mem, must keep this around
	surface = SDL_CreateRGBSurfaceFrom(GetRawMem(), xsize, ysize, 8 * channels, xsize * channels, 0x000000FF, 0x0000FF00, 0x00FF0000, (channels == 4) ? 0xFF000000 : 0);

	if (surface == nullptr)
		LOG_L(L_WARNING, "CBitmap::CreateSDLSurface Failed!");

	return surface;
}


CBitmap CBitmap::CreateRescaled(int newx, int newy) const
{
	newx = std::max(1, newx);
	newy = std::max(1, newy);

#ifndef HEADLESS
	if (compressed) {
		LOG_L(L_WARNING, "CBitmap::CreateRescaled doesn't work with compressed textures!");
		CBitmap bm;
		bm.AllocDummy();
		return bm;
	}

	if (channels != 4) {
		LOG_L(L_WARNING, "CBitmap::CreateRescaled only works with RGBA data!");
		CBitmap bm;
		bm.AllocDummy();
		return bm;
	}

	auto action = BitmapAction::GetBitmapAction(const_cast<CBitmap*>(this));
	return action->CreateRescaled(newx, newy);
#else
	CBitmap bm;
	bm.AllocDummy();
	return bm;
#endif
}


void CBitmap::InvertColors()
{
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->InvertColors();
#endif
}


void CBitmap::InvertAlpha()
{
#ifndef HEADLESS
	if (compressed)
		return; // Don't try to invert DDS

	auto action = BitmapAction::GetBitmapAction(this);
	action->InvertAlpha();
#endif
}


void CBitmap::MakeGrayScale()
{
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->MakeGrayScale();
#endif
}

void CBitmap::Tint(const float tint[3])
{
#ifndef HEADLESS
	if (compressed)
		return;

	auto action = BitmapAction::GetBitmapAction(this);
	action->Tint(tint);
#endif
}


void CBitmap::ReverseYAxis()
{
#ifndef HEADLESS
	if (compressed)
		return; // don't try to flip DDS

	const auto dts = GetDataTypeSize();
	const auto memSize = xsize * channels * dts;

	uint8_t* tmp = texMemPool.Alloc(memSize);
	uint8_t* mem = GetRawMem();

	for (int y = 0; y < (ysize / 2); ++y) {
		const int pixelL = (((y            ) * xsize) + 0) * channels * dts;
		const int pixelH = (((ysize - 1 - y) * xsize) + 0) * channels * dts;

		// copy the whole line
		std::copy(mem + pixelH, mem + pixelH + memSize, tmp         );
		std::copy(mem + pixelL, mem + pixelL + memSize, mem + pixelH);
		std::copy(tmp, tmp + memSize, mem + pixelL);
	}

	texMemPool.Free(tmp, memSize);
#endif
}

