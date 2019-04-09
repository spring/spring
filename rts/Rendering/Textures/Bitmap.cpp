/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <utility>
#include <cstring>

#include <IL/il.h>
#include <SDL_video.h>

#ifndef BITMAP_NO_OPENGL
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



static constexpr float blurkernel[9] = {
	1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f,
	2.0f/16.0f, 4.0f/16.0f, 2.0f/16.0f,
	1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f
};
// this is a minimal list of file formats that (should) be available at all platforms
static constexpr int formatList[] = {
	IL_PNG, IL_JPG, IL_TGA, IL_DDS, IL_BMP,
	IL_RGBA, IL_RGB, IL_BGRA, IL_BGR,
	IL_COLOUR_INDEX, IL_LUMINANCE, IL_LUMINANCE_ALPHA
};

static bool IsValidImageFormat(int format) {
	constexpr size_t N = sizeof(formatList) / sizeof(formatList[0]);
	return (std::find(formatList, formatList + N, format) != (formatList + N));
}



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBitmap::~CBitmap()
{
	texMemPool.Free(GetRawMem(), GetMemSize());
}

CBitmap::CBitmap(const uint8_t* data, int _xsize, int _ysize, int _channels)
	: xsize(_xsize)
	, ysize(_ysize)
	, channels(_channels)
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
		compressed = bmp.compressed;

		#ifndef BITMAP_NO_OPENGL
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
		std::swap(compressed, bmp.compressed);

		#ifndef BITMAP_NO_OPENGL
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


void CBitmap::Alloc(int w, int h, int c)
{
	if (!Empty())
		texMemPool.Free(GetRawMem(), GetMemSize());

	memIdx = texMemPool.AllocIdx((xsize = w) * (ysize = h) * (channels = c));
	memset(GetRawMem(), 0, GetMemSize());
}

void CBitmap::AllocDummy(const SColor fill)
{
	compressed = false;

	Alloc(1, 1, sizeof(SColor));
	memcpy(GetRawMem(), &fill.r, sizeof(SColor));
}

bool CBitmap::Load(const std::string& filename, uint8_t defaultAlpha)
{
	bool isLoaded = false;
	bool isValid  = false;
	bool noAlpha  =  true;

	const bool loadDDS = (FileSystem::GetExtension(filename) == "dds"); // always lower-case
	const bool flipDDS = true; // default, also assumed by gl.TexRect

	const size_t curMemSize = GetMemSize();


	channels = 4;
	#ifndef BITMAP_NO_OPENGL
	textype = GL_TEXTURE_2D;
	#endif


	#define BITMAP_USE_NV_DDS
	#ifdef BITMAP_USE_NV_DDS
	if (loadDDS) {
		#ifndef BITMAP_NO_OPENGL
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

		{
			// do not signal floating point exceptions in devil library
			ScopedDisableFpuExceptions fe;

			isLoaded = !!ilLoadL(IL_TYPE_UNKNOWN, buffer.data(), buffer.size());
			isValid = (isLoaded && IsValidImageFormat(ilGetInteger(IL_IMAGE_FORMAT)));
			noAlpha = (isValid && (ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL) != 4));

			// FPU control word has to be restored as well
			streflop::streflop_init<streflop::Simple>();
		}

		if (isValid) {
			ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

			xsize = ilGetInteger(IL_IMAGE_WIDTH);
			ysize = ilGetInteger(IL_IMAGE_HEIGHT);

			texMemPool.FreeRaw(GetRawMem(), curMemSize);
			memIdx = texMemPool.AllocIdxRaw(GetMemSize());

			// ilCopyPixels(0, 0, 0, xsize, ysize, 0, IL_RGBA, IL_UNSIGNED_BYTE, GetRawMem());
			for (const ILubyte* imgData = ilGetData(); imgData != nullptr; imgData = nullptr) {
				std::memset(GetRawMem(), 0xFF, GetMemSize());
				std::memcpy(GetRawMem(), imgData, GetMemSize());
			}
		} else {
			LOG_L(L_ERROR, "[BMP::%s] failed to load \"%s\" or invalid format %d", __func__, filename.c_str(), ilGetInteger(IL_IMAGE_FORMAT));
		}

		ilDisable(IL_ORIGIN_SET);
		ilDeleteImages(1, &imageID);
	}

	// has to be outside the mutex scope
	if (!isValid) {
		AllocDummy();
		return false;
	}

	if (noAlpha) {
		uint8_t* mem = GetRawMem();

		for (int y = 0; y < ysize; ++y) {
			for (int x = 0; x < xsize; ++x) {
				mem[((y * xsize + x) * 4) + 3] = defaultAlpha;
			}
		}
	}

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


bool CBitmap::Save(const std::string& filename, bool opaque, bool logged) const
{
	if (compressed) {
		#ifndef BITMAP_NO_OPENGL
		return ddsimage.save(filename);
		#else
		return false;
		#endif
	}

	if (GetMemSize() == 0 || channels != 4)
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
			buf[bi + 0] = mem[mi + 0];
			buf[bi + 1] = mem[mi + 1];
			buf[bi + 2] = mem[mi + 2];
			buf[bi + 3] = opaque ? 0xff : mem[mi + 3];
		}
	}

	// clear any previous errors
	while (ilGetError() != IL_NO_ERROR);

	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	ilHint(IL_COMPRESSION_HINT, IL_USE_COMPRESSION);
	ilSetInteger(IL_JPG_QUALITY, 80);

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


bool CBitmap::SaveFloat(const std::string& filename) const
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


#ifndef BITMAP_NO_OPENGL
unsigned int CBitmap::CreateTexture(float aniso, float lodBias, bool mipmaps) const
{
	if (compressed)
		return CreateDDSTexture(0, aniso, lodBias, mipmaps);

	if (GetMemSize() == 0)
		return 0;


	constexpr unsigned int intFormats[] = {0, GL_R8 , GL_RG8, GL_RGB8, GL_RGBA8};
	constexpr unsigned int extFormats[] = {0, GL_RED, GL_RG , GL_RGB , GL_RGBA }; // GL_R is not accepted for [1]

	unsigned int texture = 0;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (lodBias != 0.0f)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, lodBias);
	if (aniso > 0.0f)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);

	if (mipmaps) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glBuildMipmaps(GL_TEXTURE_2D, intFormats[channels], xsize, ysize, extFormats[channels], GL_UNSIGNED_BYTE, GetRawMem());
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, intFormats[channels], xsize, ysize, 0, extFormats[channels], GL_UNSIGNED_BYTE, GetRawMem());
	}

	return texture;
}


static void HandleDDSMipmap(GLenum target, bool mipmaps, int num_mipmaps)
{
	if (num_mipmaps > 0) {
		// dds included the MipMaps use them
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		return;
	}

	if (mipmaps) {
		// create the mipmaps at runtime
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glGenerateMipmap(target);
		return;
	}

	// no mipmaps
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

unsigned int CBitmap::CreateDDSTexture(unsigned int texID, float aniso, float lodBias, bool mipmaps) const
{
	assert(Threading::IsMainThread());
	glAttribStatePtr->PushTextureBit();

	if (texID == 0)
		glGenTextures(1, &texID);

	switch (ddsimage.get_type()) {
		case nv_dds::TextureNone:
			glDeleteTextures(1, &texID);
			texID = 0;
			break;

		case nv_dds::TextureFlat:    // 1D, 2D, and rectangle textures
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

	glAttribStatePtr->PopBits();
	return texID;
}
#else  // !BITMAP_NO_OPENGL

unsigned int CBitmap::CreateTexture(float aniso, float lodBias, bool mipmaps) const {
	return 0;
}

unsigned int CBitmap::CreateDDSTexture(unsigned int texID, float aniso, float lodBias, bool mipmaps) const {
	return 0;
}
#endif // !BITMAP_NO_OPENGL


void CBitmap::CreateAlpha(uint8_t red, uint8_t green, uint8_t blue)
{
	uint8_t* mem = GetRawMem();

	float3 aCol;

	for (int a = 0; a < 3; ++a) {
		int cCol = 0;
		int numCounted = 0;

		for (int y = 0; y < ysize; ++y) {
			for (int x = 0; x < xsize; ++x) {
				const int index = (y * xsize + x) * 4;

				if (mem[index + 3] == 0)
					continue;
				if (mem[index + 0] == red && mem[index + 1] == green && mem[index + 2] == blue)
					continue;

				cCol += mem[index + a];
				numCounted += 1;
			}
		}

		if (numCounted != 0)
			aCol[a] = cCol / 255.0f / numCounted;
	}

	const SColor c(red, green, blue);
	const SColor a(aCol.x, aCol.y, aCol.z, 0.0f);
	SetTransparent(c, a);
}


void CBitmap::SetTransparent(const SColor& c, const SColor trans)
{
	if (compressed)
		return;

	constexpr uint32_t RGB = 0x00FFFFFF;

	uint32_t* mem_i = reinterpret_cast<uint32_t*>(GetRawMem());

	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			if ((*mem_i & RGB) == (c.i & RGB))
				*mem_i = trans.i;

			mem_i++;
		}
	}
}


void CBitmap::Renormalize(float3 newCol)
{
	uint8_t* mem = GetRawMem();

	float3 aCol;
	float3 colorDif;

	for (int a = 0; a < 3; ++a) {
		int cCol = 0;
		int numCounted = 0;
		for (int y = 0; y < ysize; ++y) {
			for (int x = 0; x < xsize; ++x) {
				const unsigned int index = (y* xsize + x) * 4;
				if (mem[index + 3] != 0) {
					cCol += mem[index + a];
					numCounted += 1;
				}
			}
		}
		aCol[a] = cCol / 255.0f / numCounted;
		//cCol /= xsize*ysize; //??
		colorDif[a] = newCol[a] - aCol[a];
	}

	for (int a = 0; a < 3; ++a) {
		for (int y = 0; y < ysize; ++y) {
			for (int x = 0; x < xsize; ++x) {
				const unsigned int index = (y * xsize + x) * 4;
				float nc = float(mem[index + a]) / 255.0f + colorDif[a];
				mem[index + a] = (uint8_t) (std::min(255.f, std::max(0.0f, nc*255)));
			}
		}
	}
}


inline static void kernelBlur(
	uint8_t* dstMem,
	const uint8_t* srcMem,
	int xsize,
	int ysize,
	int nchannels,
	int x,
	int y,
	int channel,
	float weight
) {
	float fragment = 0.0f;

	const int pos = (x + y * xsize) * nchannels + channel;

	for (int i = 0; i < 9; ++i) {
		int yoffset = (i                 / 3) - 1;
		int xoffset = (i - (yoffset + 1) * 3) - 1;

		const int tx = x + xoffset;
		const int ty = y + yoffset;

		xoffset *= ((tx >= 0) && (tx < xsize));
		yoffset *= ((ty >= 0) && (ty < ysize));

		const int offset = (yoffset * xsize + xoffset) * nchannels;

		if (i == 4) {
			fragment += (weight * blurkernel[i] * srcMem[pos + offset]);
		} else {
			fragment +=           blurkernel[i] * srcMem[pos + offset];
		}
	}

	dstMem[pos] = static_cast<uint8_t>(Clamp(fragment, 0.0f, 255.0f));
}


void CBitmap::Blur(int iterations, float weight)
{
	if (compressed)
		return;

	CBitmap tmp(nullptr, xsize, ysize, channels);

	CBitmap* src = this;
	CBitmap* dst = &tmp;

	for (int i = 0; i < iterations; ++i) {
		const uint8_t* srcMem = src->GetRawMem();
		      uint8_t* dstMem = dst->GetRawMem();

		for_mt(0, ysize, [&](const int y) {
			for (int x = 0; x < xsize; x++) {
				for (int j = 0; j < channels; j++) {
					kernelBlur(dstMem, srcMem, xsize, ysize, channels, x, y, j, weight);
				}
			}
		});

		std::swap(src, dst);
	}

	// if dst points to temporary, we are done
	// otherwise need to perform one more swap
	// (e.g. if iterations=1)
	if (dst != this)
		return;

	std::swap(src, dst);
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

	for (int y = 0; y < src.ysize; ++y) {
		const int pixelDst = (((ypos + y) *     xsize) + xpos) * channels;
		const int pixelSrc = ((        y  * src.xsize) +    0) * channels;

		// copy the whole line
		std::copy(&srcMem[pixelSrc], &srcMem[pixelSrc] + channels * src.xsize, &dstMem[pixelDst]);
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

	bm.Alloc(newx, newy, channels);
	bm.CopySubImage(*this, borderLeft, borderTop);

	return bm;
}


SDL_Surface* CBitmap::CreateSDLSurface()
{
	SDL_Surface* surface = nullptr;

	if (channels < 3) {
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

	CBitmap bm;

	if (compressed) {
		LOG_L(L_WARNING, "CBitmap::CreateRescaled doesn't work with compressed textures!");
		bm.AllocDummy();
		return bm;
	}

	if (channels != 4) {
		LOG_L(L_WARNING, "CBitmap::CreateRescaled only works with RGBA data!");
		bm.AllocDummy();
		return bm;
	}

	bm.Alloc(newx, newy);

	const uint8_t* srcMem =    GetRawMem();
	      uint8_t* dstMem = bm.GetRawMem();

	const float dx = (float) xsize / newx;
	const float dy = (float) ysize / newy;

	float cy = 0;
	for (int y = 0; y < newy; ++y) {
		const int sy = (int) cy;
		cy += dy;
		int ey = (int) cy;
		if (ey == sy)
			ey = sy + 1;

		float cx = 0;
		for (int x = 0; x < newx; ++x) {
			const int sx = (int) cx;
			cx += dx;
			int ex = (int) cx;
			if (ex == sx)
				ex = sx + 1;

			int r = 0;
			int g = 0;
			int b = 0;
			int a = 0;

			for (int y2 = sy; y2 < ey; ++y2) {
				for (int x2 = sx; x2 < ex; ++x2) {
					const int index = (y2*xsize + x2) * 4;

					r += srcMem[index + 0];
					g += srcMem[index + 1];
					b += srcMem[index + 2];
					a += srcMem[index + 3];
				}
			}

			const int index = (y*bm.xsize + x) * 4;
			const int denom = (ex - sx) * (ey - sy);

			dstMem[index + 0] = r / denom;
			dstMem[index + 1] = g / denom;
			dstMem[index + 2] = b / denom;
			dstMem[index + 3] = a / denom;
		}
	}

	return bm;
}


void CBitmap::InvertColors()
{
	if (compressed)
		return;

	uint8_t* mem = GetRawMem();

	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			const int base = ((y * xsize) + x) * 4;
			mem[base + 0] = 0xFF - mem[base + 0];
			mem[base + 1] = 0xFF - mem[base + 1];
			mem[base + 2] = 0xFF - mem[base + 2];
			// do not invert alpha
		}
	}
}


void CBitmap::InvertAlpha()
{
	if (compressed)
		return; // Don't try to invert DDS

	uint8_t* mem = GetRawMem();

	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			const int base = ((y * xsize) + x) * 4;
			mem[base + 3] = 0xFF - mem[base + 3];
		}
	}
}


void CBitmap::MakeGrayScale()
{
	if (compressed)
		return;

	uint8_t* mem = GetRawMem();

	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			const int base = ((y * xsize) + x) * 4;
			const float illum =
				(mem[base + 0] * 0.299f) +
				(mem[base + 1] * 0.587f) +
				(mem[base + 2] * 0.114f);
			const uint32_t ival = (unsigned int)(illum * (256.0f / 255.0f));
			const uint8_t  cval = (ival <= 0xFF) ? ival : 0xFF;
			mem[base + 0] = cval;
			mem[base + 1] = cval;
			mem[base + 2] = cval;
		}
	}
}

static ILubyte TintByte(ILubyte value, float tint)
{
	return Clamp(value * tint, 0.0f, 255.0f);
}


void CBitmap::Tint(const float tint[3])
{
	if (compressed)
		return;

	uint8_t* mem = GetRawMem();

	for (int y = 0; y < ysize; y++) {
		for (int x = 0; x < xsize; x++) {
			const int base = ((y * xsize) + x) * 4;

			mem[base + 0] = TintByte(mem[base + 0], tint[0]);
			mem[base + 1] = TintByte(mem[base + 1], tint[1]);
			mem[base + 2] = TintByte(mem[base + 2], tint[2]);
			// don't touch the alpha channel
		}
	}
}


void CBitmap::ReverseYAxis()
{
	if (compressed)
		return; // don't try to flip DDS

	uint8_t* tmp = texMemPool.Alloc(xsize * channels);
	uint8_t* mem = GetRawMem();

	for (int y = 0; y < (ysize / 2); ++y) {
		const int pixelLow  = (((y            ) * xsize) + 0) * channels;
		const int pixelHigh = (((ysize - 1 - y) * xsize) + 0) * channels;

		// copy the whole line
		std::copy(mem + pixelHigh, mem + pixelHigh + channels * xsize, tmp);
		std::copy(mem + pixelLow , mem + pixelLow  + channels * xsize, mem + pixelHigh);
		std::copy(tmp, tmp + channels * xsize, mem + pixelLow);
	}

	texMemPool.Free(tmp, xsize * channels);
}

