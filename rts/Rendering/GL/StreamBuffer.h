#pragma once

#include "myGL.h"

//inspired by
// https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoBackends/OGL/OGLStreamBuffer.h
// https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoBackends/OGL/OGLStreamBuffer.cpp

#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <tuple>

#include "System/SpringMem.h"
#include "System/SpringMath.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GlobalRenderingInfo.h"


class IStreamBufferConcept {
public:
	enum Types : uint8_t {
		SB_BUFFERDATA    = 0,
		SB_BUFFERSUBDATA = 1,
		SB_MAPANDORPHAN  = 2,
		SB_MAPANDSYNC    = 3,
		SB_PERSISTENTMAP = 4,
		SB_PINNEDMEMAMD  = 5,
		SB_COUNT         = 6,
	};

	static void PutBufferLocks();

	IStreamBufferConcept(uint32_t target_, uint32_t byteSizeRaw, const std::string& name_);
	virtual ~IStreamBufferConcept() {
		DeleteBuffer();
	}

	void Bind(uint32_t bindTarget = 0) const;
	void Unbind(uint32_t bindTarget = 0) const;

	uint32_t GetID() const { return id; }
	uint32_t GetTarget() const { return target; }
	uint32_t GetByteSize() const { return byteSize; }
protected:
	void CreateBuffer(uint32_t byteBufferSize, uint32_t newUsage);
	void CreateBufferStorage(uint32_t byteBufferSize, uint32_t flags);
	void DeleteBuffer();
	static void LockBuffer(GLsync& syncObj);
	void WaitBuffer(GLsync& syncObj) const;
protected:
	const std::string name;
	uint32_t id = 0;
	uint32_t target = 0;
	uint32_t byteSize;
protected:
	inline static std::vector<GLsync*> lockList = {};
	static constexpr uint32_t DEFAULT_NUM_BUFFERS = 3;
};

template<typename T>
class IStreamBuffer : public IStreamBufferConcept {
public:
	static std::unique_ptr<IStreamBuffer<T>> CreateInstance(uint32_t target, uint32_t numElems, const std::string& name = "", Types type = SB_COUNT, bool coherent = false, uint32_t numBuffers = DEFAULT_NUM_BUFFERS);
public:
	IStreamBuffer(uint32_t target_, uint32_t numElems, const std::string& name_)
		: IStreamBufferConcept(target_, numElems * sizeof(T), name_)
	{}

	virtual std::pair<T*, uint32_t> Map() = 0;
	virtual void Unmap(uint32_t updatedElems) = 0;
};

//////////////////////////////////////////////////////////////////////

template<typename T>
class BufferDataImpl : public IStreamBuffer<T> {
public:
	BufferDataImpl() = default;
	BufferDataImpl(GLenum target, uint32_t numElems, const std::string& name_ = "")
		: IStreamBuffer<T>(target, numElems, name_)
	{
		buffer = static_cast<T*>(std::malloc(this->byteSize));

		this->CreateBuffer(this->byteSize, GL_STREAM_DRAW);
	}
	~BufferDataImpl() override {
		std::free(buffer);
	}

	std::pair<T*, uint32_t> Map() override {
		assert(buffer);
		return std::make_pair(buffer, 0);
	}

	void Unmap(uint32_t updatedElems) override {
		assert(updatedElems * sizeof(T) <= byteSize);

		this->Bind();
		glBufferData(this->target, updatedElems * sizeof(T), buffer, GL_STREAM_DRAW);
		this->Unbind();
	}
private:
	T* buffer;
};

template<typename T>
class BufferSubDataImpl : public IStreamBuffer<T> {
public:
	BufferSubDataImpl() = default;
	BufferSubDataImpl(GLenum target, uint32_t numElems, const std::string& name_ = "")
		: IStreamBuffer<T>(target, numElems, name_)
	{
		buffer = static_cast<T*>(std::malloc(this->byteSize));

		this->CreateBuffer(this->byteSize, GL_STREAM_DRAW);
	}
	~BufferSubDataImpl() override {
		std::free(buffer);
	}

	std::pair<T*, uint32_t> Map() override {
		assert(buffer);
		return std::make_pair(buffer, 0);
	}

	void Unmap(uint32_t updatedElems) override {
		assert(updatedElems * sizeof(T) <= byteSize);

		this->Bind();
		glBufferSubData(this->target, 0, updatedElems * sizeof(T), buffer);
		this->Unbind();
	}

private:
	T* buffer;
};

template<typename T>
class MapAndOrphanImpl : public IStreamBuffer<T> {
public:
	MapAndOrphanImpl() = default;
	MapAndOrphanImpl(GLenum target, uint32_t numElems, const std::string& name_ = "")
		: IStreamBuffer<T>(target, numElems, name_)
	{
		this->CreateBuffer(this->byteSize, GL_STREAM_DRAW);
		mapUnsyncedBit = GL_MAP_UNSYNCHRONIZED_BIT * (1 - globalRendering->haveAMD);
	}

	std::pair<T*, uint32_t> Map() override {
		this->Bind();
		T* ptr = reinterpret_cast<T*>(glMapBufferRange(this->target, 0, this->byteSize, GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | mapUnsyncedBit));
		assert(ptr);
		return std::make_pair(
			ptr,
			0
		);
	}

	void Unmap(uint32_t updatedElems) override {
		assert(updatedElems * sizeof(T) <= byteSize);

		glFlushMappedBufferRange(this->target, 0, updatedElems * sizeof(T));
		glUnmapBuffer(this->target);
		this->Unbind();
	}

private:
	uint32_t mapUnsyncedBit;
};

template<typename T>
class MapAndSyncImpl : public IStreamBuffer<T> {
public:
	MapAndSyncImpl() = default;
	MapAndSyncImpl(GLenum target, uint32_t numElems, uint32_t numBuffers, const std::string& name_ = "")
		: IStreamBuffer<T>(target, numElems, name_)
		, allocIdx{ 0 }
	{
		this->CreateBuffer(numBuffers * this->byteSize, GL_STREAM_DRAW);

		fences.resize(numBuffers);
		for (auto& fence : fences)
			IStreamBufferConcept::LockBuffer(fence);
	}
	~MapAndSyncImpl() override {
		for (auto& fence : fences)
			glDeleteSync(fence);
	}

	std::pair<T*, uint32_t> Map() override {
		this->WaitBuffer(fences[allocIdx]);

		this->Bind();
		T* ptr = reinterpret_cast<T*>(glMapBufferRange(this->target, allocIdx * this->byteSize, this->byteSize, GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_UNSYNCHRONIZED_BIT));
		assert(ptr);
		return std::make_pair(
			ptr,
			allocIdx * this->byteSize
		);
	}

	void Unmap(uint32_t updatedElems) override {
		assert(updatedElems * sizeof(T) <= byteSize);

		glFlushMappedBufferRange(this->target, 0, updatedElems * sizeof(T));
		glUnmapBuffer(this->target);
		this->Unbind();

		IStreamBufferConcept::LockBuffer(fences[allocIdx]);
		allocIdx = (allocIdx + 1) % fences.size();
	}

private:
	std::vector<GLsync> fences;
	uint32_t allocIdx;
};

template<typename T>
class PersistentMapImpl : public IStreamBuffer<T> {
public:
	PersistentMapImpl() = default;
	PersistentMapImpl(GLenum target, uint32_t numElems, uint32_t numBuffers, const std::string& name_ = "", bool coherent_ = true)
		: IStreamBuffer<T>(target, numElems, name_)
		, coherent{ coherent_ }
		, allocIdx{ 0 }
	{

		this->CreateBufferStorage(numBuffers* this->byteSize,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT | (coherent * GL_MAP_COHERENT_BIT));

		this->Bind();
		ptrBase = reinterpret_cast<T*>(glMapBufferRange(this->target, allocIdx * this->byteSize, this->byteSize,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | (coherent ? GL_MAP_COHERENT_BIT : GL_MAP_FLUSH_EXPLICIT_BIT)));
		this->Unbind(); //binding is not needed for mapping to stay valid

		fences.resize(numBuffers);
		for (auto& fence : fences)
			IStreamBufferConcept::LockBuffer(fence);
	}
	~PersistentMapImpl() override {
		for (auto& fence : fences)
			glDeleteSync(fence);

		glUnmapBuffer(this->target);
		ptrBase = nullptr;
	}

	std::pair<T*, uint32_t> Map() override {
		assert(ptrBase);
		this->WaitBuffer(fences[allocIdx]);

		T* ptr = reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(ptrBase) + allocIdx * this->byteSize);
		return std::make_pair(
			ptr,
			allocIdx * this->byteSize
		);
	}

	void Unmap(uint32_t updatedElems) override {
		assert(updatedElems * sizeof(T) <= this->byteSize);

		if (!coherent) {
			this->Bind();
			glFlushMappedBufferRange(this->target, 0, updatedElems * sizeof(T));
			this->Unbind();
		}

		IStreamBufferConcept::LockBuffer(fences[allocIdx]);
		allocIdx = (allocIdx + 1) % fences.size();
	}

private:
	const bool coherent;
	T* ptrBase;
	std::vector<GLsync> fences;
	uint32_t allocIdx;
};

template<typename T>
class PinnedMemoryAMDImpl : public IStreamBuffer<T> {
public:
	PinnedMemoryAMDImpl() = default;
	PinnedMemoryAMDImpl(GLenum target, uint32_t numElems, uint32_t numBuffers, const std::string& name_ = "")
		: IStreamBuffer<T>(target, numElems, name_)
		, allocIdx{ 0 }
	{
		uint32_t bufferSize = AlignUp(numBuffers * this->byteSize, ALIGN_PINNED_MEMORY_SIZE);
		ptrBase = reinterpret_cast<T*>(spring::AllocateAlignedMemory(bufferSize, ALIGN_PINNED_MEMORY_SIZE));

		glGenBuffers(1, &this->id);

		this->Bind(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD);
		glBufferData(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, bufferSize, ptrBase, GL_STREAM_COPY);
		this->Unbind(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD);

		this->Bind();

		fences.resize(numBuffers);
		for (auto& fence : fences)
			IStreamBufferConcept::LockBuffer(fence);
	}
	~PinnedMemoryAMDImpl() override {
		for (auto& fence : fences)
			glDeleteSync(fence);

		this->Unbind();
		glFinish();
		spring::FreeAlignedMemory(ptrBase);
		ptrBase = nullptr;
	}

	std::pair<T*, uint32_t> Map() override {
		assert(ptrBase);
		this->WaitBuffer(fences[allocIdx]);

		T* ptr = reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(ptrBase) + allocIdx * this->byteSize);
		return std::make_pair(
			ptr,
			allocIdx * this->byteSize
		);
	}

	void Unmap(uint32_t updatedElems) override {
		assert(updatedElems * sizeof(T) <= this->byteSize);

		IStreamBufferConcept::LockBuffer(fences[allocIdx]);
		allocIdx = (allocIdx + 1) % fences.size();
	}

private:
	T* ptrBase;
	std::vector<GLsync> fences;
	uint32_t allocIdx;

	static constexpr uint32_t ALIGN_PINNED_MEMORY_SIZE = 4096;
};

//////////////////////////////////////////////////////////////////////

template<typename T>
inline std::unique_ptr<IStreamBuffer<T>> IStreamBuffer<T>::CreateInstance(uint32_t target, uint32_t numElems, const std::string& name, Types type, bool coherent, uint32_t numBuffers)
{
	switch (type) {
	case SB_BUFFERDATA:
		return std::make_unique<BufferDataImpl<T>>(target, numElems, name);
	case SB_BUFFERSUBDATA:
		return std::make_unique<BufferSubDataImpl<T>>(target, numElems, name);
	case SB_MAPANDORPHAN:
		return std::make_unique<MapAndOrphanImpl<T>>(target, numElems, name);
	case SB_MAPANDSYNC:
		return std::make_unique<MapAndSyncImpl<T>>(target, numElems, numBuffers, name);
	case SB_PERSISTENTMAP:
		return std::make_unique<PersistentMapImpl<T>>(target, numElems, numBuffers, name, coherent);
	case SB_PINNEDMEMAMD:
		return std::make_unique<PinnedMemoryAMDImpl<T>>(target, numElems, numBuffers, name);
	default: {} break;
	}

	/*
	// synced primitives are slightly slower on my setup (NV RTX 3060 + Windows 10)
	const uint32_t ver = globalRenderingInfo.glContextVersion.x * 10 + globalRenderingInfo.glContextVersion.y;
	if (ver >= 44) //core in OpenGL 4.4
		return CreateInstance(target, byteSize, name, SB_PERSISTENTMAP, numBuffers);

	if (GLEW_ARB_buffer_storage) //extension
		return CreateInstance(target, byteSize, name, SB_PERSISTENTMAP, numBuffers);

	if (GLEW_AMD_pinned_memory)
		return CreateInstance(target, byteSize, name, SB_PINNEDMEMAMD, numBuffers);
	*/
	//seems like sensible default
	return CreateInstance(target, numElems, name, SB_BUFFERSUBDATA, numBuffers);

	// another sensible default, same perf as the above
	//return CreateInstance(target, byteSize, name, SB_MAPANDORPHAN, numBuffers);
}