#pragma once

#include "myGL.h"

//inspired by
// https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoBackends/OGL/OGLStreamBuffer.h
// https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoBackends/OGL/OGLStreamBuffer.cpp

#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <stdexcept>


#include "System/SpringMem.h"
#include "System/SpringMath.h"
#include "System/TypeToStr.h"
#include "Rendering/GlobalRendering.h"

class IStreamBufferConcept {
public:
	enum Types : uint8_t {
		SB_BUFFERDATA    = 0,
		SB_BUFFERSUBDATA = 1,
		SB_MAPANDORPHAN  = 2,
		SB_MAPANDSYNC    = 3,
		SB_PERSISTENTMAP = 4,
		SB_PINNEDMEMAMD  = 5,
		SB_AUTODETECT    = 6,
	};

	static void PutBufferLocks();

	IStreamBufferConcept(uint32_t target_, uint32_t numElements_, const std::string& name_, const std::string_view& bufferTypeName);
	virtual ~IStreamBufferConcept() {}

	uint32_t GetAlignedByteSize(uint32_t byteSizeRaw);

	void Bind(uint32_t bindTarget = 0) const;
	void Unbind(uint32_t bindTarget = 0) const;

	void BindBufferRange(GLuint index, uint32_t bindTarget = 0) const;
	void UnbindBufferRange(GLuint index, uint32_t bindTarget = 0) const;

	uint32_t GetID() const { return id; }
	uint32_t GetTarget() const { return target; }
	uint32_t GetByteSize() const { return byteSize; }
protected:
	void CreateBuffer(uint32_t byteBufferSize, uint32_t newUsage);
	void CreateBufferStorage(uint32_t byteBufferSize, uint32_t flags);
	void DeleteBuffer();
	void QueueLockBuffer(GLsync& syncObj) const;
	void WaitBuffer(GLsync& syncObj) const;
protected:
	const std::string name;
	uint32_t target;
	uint32_t id;
	uint32_t numElements;
	uint32_t byteSize;
	uint32_t allocIdx;
	uint32_t mapElemOffet;
	uint32_t mapElemCount;
protected:
	inline static bool reportType = false;
	inline static std::vector<GLsync*> lockList = {};
	static constexpr uint32_t DEFAULT_NUM_BUFFERS = 3;
};

template<typename T>
class IStreamBuffer : public IStreamBufferConcept {
public:
	static std::unique_ptr<IStreamBuffer<T>> CreateInstance(uint32_t target, uint32_t numElems, const std::string& name = "", Types type = SB_AUTODETECT, bool resizeAble = false, bool coherent = false, uint32_t numBuffers = DEFAULT_NUM_BUFFERS);
public:
	IStreamBuffer(uint32_t target_, uint32_t numElems, const std::string& name_, const std::string_view& bufferTypeName_)
		: IStreamBufferConcept(target_, numElems, name_, bufferTypeName_)
	{}

	virtual T* Map(const T* clientPtr = nullptr, uint32_t elemOffset = 0, uint32_t elemCount = 0) {
		this->mapElemOffet = elemOffset;

		if (elemCount > 0) {
			this->mapElemCount = elemCount;
			assert((this->mapElemOffet + this->mapElemCount) <= this->numElements);
		}
		else {
			this->mapElemCount = this->numElements - this->mapElemOffet;
		}
		return nullptr;
	}

	virtual void Unmap() = 0;
	virtual void SwapBuffer() {};

	virtual bool HasClientPtr() const { return false; };
	virtual uint32_t BufferElemOffset() const { return 0; };

	virtual void Init() = 0;
	virtual void Kill(bool deleteBuffer) = 0;

	virtual void Resize(uint32_t numElems) {
		Kill(false);

		this->numElements = numElems; Init();
	}
};

//////////////////////////////////////////////////////////////////////

template<typename T>
class BufferDataImpl : public IStreamBuffer<T> {
public:
	BufferDataImpl(GLenum target, uint32_t numElems, const std::string& name_)
		: IStreamBuffer<T>(target, numElems, name_, spring::TypeToCStr<decltype(*this)>())
		, clientMem { false }
		, buffer{ nullptr }
	{
		Init();
	}
	~BufferDataImpl() override {
		Kill(true);
	}

	void Init() override {
		this->byteSize = this->GetAlignedByteSize(this->numElements * sizeof(T));;
		this->CreateBuffer(this->byteSize, GL_STREAM_DRAW);
	}

	void Kill(bool deleteBuffer) override {
		if (!clientMem && buffer != nullptr) {
			spring::FreeAlignedMemory(buffer);
			buffer = nullptr;
		}

		if (deleteBuffer) this->DeleteBuffer();
	}

	T* Map(const T* clientPtr, uint32_t elemOffset, uint32_t elemCount) override {
		IStreamBuffer<T>::Map(clientPtr, elemOffset, elemCount);

		if (clientPtr) {
			clientMem = true;

			// clientPtr is expected to point to 0th element
			buffer = const_cast<T*>(clientPtr);
		}
		else if (buffer == nullptr) {
			clientMem = false;

			buffer = static_cast<T*>(spring::AllocateAlignedMemory(this->byteSize, 256));
			assert(buffer);
		}
		return buffer + this->mapElemOffet;
	}

	void Unmap() override {
		this->Bind();
		// BufferDataImpl can only upload the whole buffer completely
		glBufferData(
			this->target,
			this->mapElemCount * sizeof(T),
			buffer,
			GL_STREAM_DRAW
		);

		this->Unbind();
	}

	bool HasClientPtr() const override { return clientMem; };
private:
	bool clientMem;
	T* buffer;
};

template<typename T>
class BufferSubDataImpl : public IStreamBuffer<T> {
public:
	BufferSubDataImpl(GLenum target, uint32_t numElems, const std::string& name_)
		: IStreamBuffer<T>(target, numElems, name_, spring::TypeToCStr<decltype(*this)>())
		, clientMem{ false }
		, buffer{ nullptr }
	{
		Init();
	}
	~BufferSubDataImpl() override {
		Kill(true);
	}

	void Init() override {
		this->byteSize = this->GetAlignedByteSize(this->numElements * sizeof(T));;
		this->CreateBuffer(this->byteSize, GL_STREAM_DRAW);
	}

	void Kill(bool deleteBuffer) override {
		if (!clientMem && buffer != nullptr) {
			spring::FreeAlignedMemory(buffer);
			buffer = nullptr;
		}
		if (deleteBuffer) this->DeleteBuffer();
	}

	T* Map(const T* clientPtr, uint32_t elemOffset, uint32_t elemCount) override {
		IStreamBuffer<T>::Map(clientPtr, elemOffset, elemCount);

		if (clientPtr) {
			clientMem = true;

			// clientPtr is expected to point to 0th element
			buffer = const_cast<T*>(clientPtr);
		}
		else if (buffer == nullptr) {
			clientMem = false;

			buffer = static_cast<T*>(spring::AllocateAlignedMemory(this->byteSize, 256));
			assert(buffer);
		}
		return buffer + this->mapElemOffet;
	}

	void Unmap() override {
		this->Bind();
		// BufferSubDataImpl can upload only updated elements
		glBufferSubData(
			this->target,
			this->mapElemOffet * sizeof(T),
			this->mapElemCount * sizeof(T),
			buffer + this->mapElemOffet
		);

		this->Unbind();
	}

	bool HasClientPtr() const override { return clientMem; };
private:
	bool clientMem;
	T* buffer;
};

template<typename T>
class MapAndOrphanImpl : public IStreamBuffer<T> {
public:
	MapAndOrphanImpl(GLenum target, uint32_t numElems, const std::string& name_)
		: IStreamBuffer<T>(target, numElems, name_, spring::TypeToCStr<decltype(*this)>())
	{
		Init();
		mapUnsyncedBit = GL_MAP_UNSYNCHRONIZED_BIT * (1 - globalRendering->haveAMD);
	}
	~MapAndOrphanImpl() override {
		Kill(true);
	}

	void Init() override {
		this->byteSize = this->GetAlignedByteSize(this->numElements * sizeof(T));;
		this->CreateBuffer(this->byteSize, GL_STREAM_DRAW);
	}

	void Kill(bool deleteBuffer) override {
		if (deleteBuffer)
			this->DeleteBuffer();
	}

	T* Map(const T* clientPtr, uint32_t elemOffset, uint32_t elemCount) override {
		IStreamBuffer<T>::Map(clientPtr, elemOffset, elemCount);

		mapInvalidateBit =
			(this->mapElemOffet + this->mapElemCount == this->numElements) ?
			GL_MAP_INVALIDATE_BUFFER_BIT :
			GL_MAP_INVALIDATE_RANGE_BIT;

		this->Bind();
		T* ptr = reinterpret_cast<T*>(glMapBufferRange(
			this->target,
			this->mapElemOffet * sizeof(T),
			this->mapElemCount * sizeof(T),
			GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | mapInvalidateBit | mapUnsyncedBit));

		assert(ptr);
		return ptr;
	}

	void Unmap() override {
		glFlushMappedBufferRange(
			this->target,
			/*this->mapElemOffet * sizeof(T)*/0, // offset is relative to the mapped range offset
			this->mapElemCount * sizeof(T)
		);

		glUnmapBuffer(this->target);
		this->Unbind();
	}
private:
	uint32_t mapUnsyncedBit;
	uint32_t mapInvalidateBit;
};

template<typename T>
class MapAndSyncImpl : public IStreamBuffer<T> {
public:
	MapAndSyncImpl(GLenum target, uint32_t numElems, uint32_t numBuffers_, const std::string& name_, bool coherent_)
		: IStreamBuffer<T>(target, numElems, name_, spring::TypeToCStr<decltype(*this)>())
		, numBuffers{ numBuffers_ }
		, coherent{ coherent_ }
	{
		Init();
	}
	~MapAndSyncImpl() override {
		Kill(true);
	}

	void Init() override {
		this->byteSize = this->GetAlignedByteSize(this->numElements * sizeof(T));
		this->CreateBufferStorage(numBuffers * this->byteSize,
			GL_MAP_WRITE_BIT | GL_CLIENT_STORAGE_BIT);

		fences.resize(numBuffers);
		for (auto& fence : fences)
			this->QueueLockBuffer(fence);
	}

	void Kill(bool deleteBuffer) override {
		for (auto& fence : fences)
			this->WaitBuffer(fence);
		glFinish(); //just in case

		this->DeleteBuffer(); //delete regardless because of immutability
	}

	T* Map(const T* clientPtr, uint32_t elemOffset, uint32_t elemCount) override {
		IStreamBuffer<T>::Map(clientPtr, elemOffset, elemCount);

		this->WaitBuffer(fences[this->allocIdx]);

		this->Bind();
		T* ptr = reinterpret_cast<T*>(glMapBufferRange(
			this->target,
			this->allocIdx * this->byteSize + this->mapElemOffet * sizeof(T),
			this->mapElemCount * sizeof(T),
			GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | (!coherent * GL_MAP_FLUSH_EXPLICIT_BIT)
		));

		assert(ptr);
		return ptr;
	}

	void Unmap() override {
		if (!coherent) {
			glFlushMappedBufferRange(
				this->target,
				/*this->mapElemOffet * sizeof(T)*/0, // offset is relative to the mapped range offset
				this->mapElemCount * sizeof(T)
			);
		}

		glUnmapBuffer(this->target);
		this->Unbind();
	}

	uint32_t BufferElemOffset() const override {
		return this->allocIdx * this->numElements;
	}

	void SwapBuffer() override {
		this->QueueLockBuffer(fences[this->allocIdx]);
		this->allocIdx = (this->allocIdx + 1) % numBuffers;
	};
private:
	uint32_t numBuffers;
	const bool coherent;
	std::vector<GLsync> fences;
};

template<typename T>
class PersistentMapImpl : public IStreamBuffer<T> {
public:
	PersistentMapImpl(GLenum target, uint32_t numElems, uint32_t numBuffers_, const std::string& name_, bool coherent_)
		: IStreamBuffer<T>(target, numElems, name_, spring::TypeToCStr<decltype(*this)>())
		, numBuffers{ numBuffers_ }
		, ptrBase{ nullptr }
		, coherent{ coherent_ }
	{
		Init();
	}
	~PersistentMapImpl() override {
		Kill(true);
	}

	void Init() override {
		this->byteSize = this->GetAlignedByteSize(this->numElements * sizeof(T));
		this->CreateBufferStorage(numBuffers * this->byteSize,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | (coherent * GL_MAP_COHERENT_BIT));

		this->Bind();
		ptrBase = reinterpret_cast<T*>(glMapBufferRange(
			this->target,
			0,
			numBuffers * this->byteSize,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | mix(GL_MAP_FLUSH_EXPLICIT_BIT, GL_MAP_COHERENT_BIT, coherent)
		));
		assert(ptrBase);
		this->Unbind();

		fences.resize(numBuffers);
		for (auto& fence : fences)
			this->QueueLockBuffer(fence);
	}

	void Kill(bool deleteBuffer) override {
		for (auto& fence : fences)
			this->WaitBuffer(fence);
		glFinish(); //just in case

		glUnmapBuffer(this->target);

		this->DeleteBuffer(); //delete regardless because of immutability
		ptrBase = nullptr;
	}

	T* Map(const T* clientPtr, uint32_t elemOffset, uint32_t elemCount) override {
		IStreamBuffer<T>::Map(clientPtr, elemOffset, elemCount);
		assert(ptrBase);

		this->WaitBuffer(fences[this->allocIdx]);

		T* ptr = ptrBase + this->allocIdx * this->numElements + this->mapElemOffet;

		return ptr;
	}

	void Unmap() override {
		if (!coherent) {
			this->Bind(); //needed for glFlushMappedBufferRange()
			glFlushMappedBufferRange(
				this->target,
				(this->allocIdx * this->byteSize) + this->mapElemOffet * sizeof(T),
				this->mapElemCount * sizeof(T)
			);
			this->Unbind(); //needed for glFlushMappedBufferRange()
		}
	}

	uint32_t BufferElemOffset() const override {
		return this->allocIdx * this->numElements;
	}

	void SwapBuffer() override {
		this->QueueLockBuffer(fences[this->allocIdx]);
		this->allocIdx = (this->allocIdx + 1) % numBuffers;
	};
private:
	uint32_t numBuffers;
	T* ptrBase;
	const bool coherent;
	std::vector<GLsync> fences;
};

template<typename T>
class PinnedMemoryAMDImpl : public IStreamBuffer<T> {
public:
	PinnedMemoryAMDImpl(GLenum target, uint32_t numElems, uint32_t numBuffers_, const std::string& name_)
		: IStreamBuffer<T>(target, numElems, name_, spring::TypeToCStr<decltype(*this)>())
		, numBuffers{ numBuffers_ }
	{
		Init();
	}
	~PinnedMemoryAMDImpl() override {
		Kill(true);
	}

	void Init() override {
		this->byteSize = this->GetAlignedByteSize(this->numElements * sizeof(T));
		uint32_t bufferSize = AlignUp(numBuffers * this->byteSize, ALIGN_PINNED_MEMORY_SIZE);
		ptrBase = reinterpret_cast<T*>(spring::AllocateAlignedMemory(bufferSize, ALIGN_PINNED_MEMORY_SIZE));

		glGenBuffers(1, &this->id);

		this->Bind(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD);
		glBufferData(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, bufferSize, ptrBase, GL_STREAM_COPY);
		this->Unbind(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD);

		this->Bind();

		fences.resize(numBuffers);
		for (auto& fence : fences)
			IStreamBufferConcept::QueueLockBuffer(fence);
	}
	void Kill(bool deleteBuffer) override {
		for (auto& fence : fences)
			this->WaitBuffer(fence);

		this->Unbind();
		glFinish();

		this->DeleteBuffer();
		spring::FreeAlignedMemory(ptrBase);
		ptrBase = nullptr;
	}

	T* Map(const T* clientPtr, uint32_t elemOffset, uint32_t elemCount) override {
		IStreamBuffer<T>::Map(clientPtr, elemOffset, elemCount);
		assert(ptrBase);

		this->WaitBuffer(fences[this->allocIdx]);

		T* ptr = ptrBase + this->allocIdx * this->numElements + this->mapElemOffet;
		return ptr;
	}

	void Unmap() override {}

	uint32_t BufferElemOffset() const override {
		return this->allocIdx * this->numElements;
	}

	void SwapBuffer() override {
		this->QueueLockBuffer(fences[this->allocIdx]);
		this->allocIdx = (this->allocIdx + 1) % numBuffers;
	};
private:
	uint32_t numBuffers;
	T* ptrBase;
	std::vector<GLsync> fences;

	static constexpr uint32_t ALIGN_PINNED_MEMORY_SIZE = 4096;
};

//////////////////////////////////////////////////////////////////////

template<typename T>
inline std::unique_ptr<IStreamBuffer<T>> IStreamBuffer<T>::CreateInstance(uint32_t target, uint32_t numElems, const std::string& name, Types type, bool resizeAble, bool coherent, uint32_t numBuffers)
{
	IStreamBufferConcept::reportType = (type == SB_AUTODETECT);

	switch (type) {
	case SB_BUFFERDATA:
		return std::make_unique<BufferDataImpl<T>>(target, numElems, name);
	case SB_BUFFERSUBDATA:
		return std::make_unique<BufferSubDataImpl<T>>(target, numElems, name);
	case SB_MAPANDORPHAN:
		return std::make_unique<MapAndOrphanImpl<T>>(target, numElems, name);
	case SB_MAPANDSYNC:
		return std::make_unique<MapAndSyncImpl<T>>(target, numElems, numBuffers, name, coherent);
	case SB_PERSISTENTMAP:
		return std::make_unique<PersistentMapImpl<T>>(target, numElems, numBuffers, name, coherent);
	case SB_PINNEDMEMAMD:
		return std::make_unique<PinnedMemoryAMDImpl<T>>(target, numElems, numBuffers, name);
	default: {} break;
	}

	if (resizeAble || target == GL_UNIFORM_BUFFER) {
		return CreateInstance(target, numElems, name, SB_BUFFERSUBDATA, numBuffers); //almost certainly best
	}

	if (GLEW_ARB_sync) {
		if (globalRendering->haveAMD)
			return CreateInstance(target, numElems, name, SB_PINNEDMEMAMD, numBuffers);

		if (GLEW_ARB_buffer_storage) //core in OpenGL 4.4 or extension
			return CreateInstance(target, numElems, name, SB_PERSISTENTMAP, numBuffers);

		return CreateInstance(target, numElems, name, SB_MAPANDSYNC, numBuffers);
	}

	//seems like sensible default
	return CreateInstance(target, numElems, name, SB_BUFFERSUBDATA, numBuffers);
}