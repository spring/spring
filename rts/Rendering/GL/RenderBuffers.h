#pragma once

#include "StreamBuffer.h"
#include "VertexArrayTypes.h"
#include "VAO.h"

#include "System/TypeToStr.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "Rendering/Shaders/Shader.h"

#include "fmt/format.h"
#include "lib/fmt/printf.h"

#include <string>
#include <sstream>
#include <memory>
#include <array>
#include <vector>
#include <iterator>
#include <algorithm>

template <typename T>
class TypedRenderBuffer;

class RenderBuffer {
public:
	template <typename T>
	static TypedRenderBuffer<T>& GetTypedRenderBuffer();

	virtual void SwapBuffer() = 0;

	static void SwapStandardRenderBuffers() {
		for (const auto& trb : typedRenderBuffers) {
			if (trb) {
				trb->SwapBuffer();
			}
		}
	}
private:
	static std::array<std::unique_ptr<RenderBuffer>, 11> typedRenderBuffers;
};

template <typename T>
class RenderBufferShader {
public:
	static Shader::IProgramObject& GetShader() {
		if (shader) {
			if (!shader->IsReloadRequested())
				return *(shader.get());
			else {
				shader->Release();
				shader = nullptr;
			}
		}

		if (!shader) {
			// can't use shaderHandler here because it invalidates the objects on reload
			// but shaders are expected to be available all the time
			shader = std::make_unique<Shader::GLSLProgramObject>(typeName);
		}

		std::string vertSrc = GetShaderTemplate("GLSL/RenderBufferVertTemplate.glsl");
		std::string fragSrc = GetShaderTemplate("GLSL/RenderBufferFragTemplate.glsl");

		std::string vsInputs;
		std::string varyingsData;
		std::string vsAssignment;
		std::string vsPosVertex;

		GetAttributesStrings(vsInputs, varyingsData, vsAssignment, vsPosVertex);

		static const char* fmtString = "%s Data {%s%s};";
		const std::string varyingsVS = (varyingsData.empty()) ? "" : fmt::sprintf(fmtString, "out", nl, varyingsData);
		const std::string varyingsFS = (varyingsData.empty()) ? "" : fmt::sprintf(fmtString, "in" , nl, varyingsData);

		vertSrc = fmt::sprintf(vertSrc,
			vsInputs,
			varyingsVS,
			vsAssignment,
			vsPosVertex
		);

		const std::string fragOutput = GetFragOutput();
		fragSrc = fmt::sprintf(fragSrc,
			varyingsFS,
			fragOutput
		);

		shader->AttachShaderObject(new Shader::GLSLShaderObject(GL_VERTEX_SHADER  , vertSrc));
		shader->AttachShaderObject(new Shader::GLSLShaderObject(GL_FRAGMENT_SHADER, fragSrc));

		shader->Link();

		shader->Enable();
		shader->Disable();

		shader->Validate();
		assert(shader->IsValid());
		shader->SetReloadComplete();

		return *shader;
	}
private:
	static const std::string TypeToString(const AttributeDef& ad) {
		static const char* fmtString = "{type}{count}";

		std::string type;

		switch (ad.type)
		{
		case GL_FLOAT: {
			type = (ad.count == 1) ? "float" : "vec";
		} break;
		case GL_UNSIGNED_BYTE: {
			if (ad.normalize)
				type = (ad.count == 1) ? "float" : "vec";
			else
				type = (ad.count == 1) ? "uint" : "uvec";
		} break;
		default:
			assert(false);
			break;
		}

		const std::string count = (ad.count == 1) ? "" : std::to_string(ad.count);
		return fmt::format(fmtString, fmt::arg("type", type), fmt::arg("count", count));
	}

	static const std::string GetFragOutput();

	static void GetAttributesStrings(std::string& vsInputs, std::string& varyings, std::string& vsAssignment, std::string& vsPosVertex) {
		static const char* vsInputsFmt     = "layout(location = {indx}) in {type} a{name};";
		static const char* varyingsFmt     = "\t{type} v{name};";
		static const char* vsAssignmentFmt = "\tv{name} = a{name};";

		assert(T::attributeDefs[0].index == 0);
		assert(T::attributeDefs[0].name == "pos");
		assert(T::attributeDefs[0].count == 2 || T::attributeDefs[0].count == 3);

		vsPosVertex = (T::attributeDefs[0].count == 2) ? "vec4(apos, 0.0, 1.0)" : "vec4(apos, 1.0)";

		std::vector<std::string> vsInputsVec;
		std::vector<std::string> varyingsVec;
		std::vector<std::string> vsAssignmentVec;

		for (const AttributeDef& ad : T::attributeDefs) {
			vsInputsVec.emplace_back(fmt::format(vsInputsFmt,
				fmt::arg("indx", ad.index),
				fmt::arg("type", TypeToString(ad)),
				fmt::arg("name", ad.name)
			));

			if (ad.index == 0)
				continue;

			varyingsVec.emplace_back(fmt::format(varyingsFmt,
				fmt::arg("type", TypeToString(ad)),
				fmt::arg("name", ad.name)
			));

			vsAssignmentVec.emplace_back(fmt::format(vsAssignmentFmt,
				fmt::arg("name", ad.name)
			));
		}

		const auto joinFunc = [](const std::vector<std::string>& vec) -> const std::string {
			if (vec.empty())
				return "";

			std::ostringstream result;
			std::copy(vec.begin(), vec.end(), std::ostream_iterator<std::string>(result, nl));
			return result.str();
		};

		vsInputs = joinFunc(vsInputsVec);
		varyings = joinFunc(varyingsVec);
		vsAssignment = joinFunc(vsAssignmentVec);
	}

	static const std::string GetShaderTemplate(const std::string& relPath) {
		const std::string stPath = "shaders/" + relPath;
		std::string stSource = "";

		CFileHandler stFile(stPath);

		if (stFile.FileExists()) {
			stSource.resize(stFile.FileSize());
			stFile.Read(&stSource[0], stFile.FileSize());
		}

		return stSource;
	}
private:
	inline static std::unique_ptr<Shader::IProgramObject> shader = nullptr;

	static constexpr const char* typeName = spring::TypeToCStr<T>();
	static constexpr const char* nl = "\r\n";
};


//member template specializations
template<>
inline const std::string RenderBufferShader<VA_TYPE_0>::GetFragOutput()
{
	return "\toutColor = vec4(1.0);";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_C>::GetFragOutput()
{
	return "\toutColor = vcolor;";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_N>::GetFragOutput()
{
	return "\toutColor = vec4(1.0);";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_T>::GetFragOutput()
{
	return "\toutColor = texture(tex, uv);";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_TN>::GetFragOutput()
{
	return "\toutColor = texture(tex, uv);";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_TC>::GetFragOutput()
{
	return "\toutColor = vcolor * texture(tex, uv);";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_TNT>::GetFragOutput()
{
	return "\toutColor = texture(tex, uv);";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_2d0>::GetFragOutput()
{
	return "\toutColor = vec4(1.0);";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_2dC>::GetFragOutput()
{
	return "\toutColor = vcolor;";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_2dT>::GetFragOutput()
{
	return "\toutColor = texture(tex, uv);";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_2dTC>::GetFragOutput()
{
	return "\toutColor = vcolor * texture(tex, uv);";
}

template <typename T>
class TypedRenderBuffer : public RenderBuffer {
public:
	using VertType = T;
	using IndcType = uint32_t;

	TypedRenderBuffer<T>()
		: vertCount0{ 0 }
		, elemCount0{ 0 }
		, bufferType{ bufferTypeDefault }
	{}
	TypedRenderBuffer<T>(size_t vertCount0_, size_t elemCount0_, IStreamBufferConcept::Types bufferType_ = bufferTypeDefault)
		: vertCount0 { vertCount0_ }
		, elemCount0 { elemCount0_ }
		, bufferType { bufferType_ }
	{
		verts.reserve(vertCount0);
		indcs.reserve(elemCount0);
	}

	void SwapBuffer() override {
		if (vbo)
			vbo->SwapBuffer();
		if (ebo)
			ebo->SwapBuffer();

		// clear
		verts.clear();
		indcs.clear();

		vboStartIndex = 0;
		eboStartIndex = 0;

		vboUploadIndex = 0;
		eboUploadIndex = 0;

		numSubmits = { 0, 0 };
	}

	TypedRenderBuffer<T>(const TypedRenderBuffer<T>& trdb) = delete;
	TypedRenderBuffer<T>(TypedRenderBuffer<T>&& trdb) { *this = std::move(trdb); }

	TypedRenderBuffer<T>& operator = (const TypedRenderBuffer<T>& rhs) = delete;
	TypedRenderBuffer<T>& operator = (TypedRenderBuffer<T>&& rhs) {
		vertCount0 = rhs.vertCount0;
		elemCount0 = rhs.elemCount0;
		bufferType = rhs.bufferType;

		std::swap(vbo, rhs.vbo);
		std::swap(ebo, rhs.ebo);

		std::swap(vao, rhs.vao);

		std::swap(verts, rhs.verts);
		std::swap(indcs, rhs.indcs);

		vboStartIndex = rhs.vboStartIndex;
		eboStartIndex = rhs.eboStartIndex;

		vboUploadIndex = rhs.vboUploadIndex;
		eboUploadIndex = rhs.eboUploadIndex;

		std::swap(numSubmits, rhs.numSubmits);

		return *this;
	}

	void AddVertex(VertType&& v) {
		verts.emplace_back(v);
	}
	//develop compat
	void SafeAppend(VertType&& v) { AddVertex(std::forward<VertType&&>(v)); }

	void AddVertices(std::initializer_list<VertType>&& vs) {
		for (auto&& v : vs) {
			verts.emplace_back(v);
		}
	}

	void UpdateVertex(VertType&& v, size_t at) {
		assert(at < verts.size());
		verts.emplace(at, v);
	}
	void UpdateVertices(std::initializer_list<VertType>&& vs, size_t at) {
		size_t cnt = 0;
		for (auto&& v : vs) {
			assert(cnt + at < verts.size());
			verts.emplace(cnt + at, v);
			++cnt;
		}
	}

	// render with DrawElements(GL_TRIANGLES)
	void AddQuadTriangles(const VertType* vs) { AddQuadTriangles(vs + 0, vs + 1, vs + 2, vs + 3, vs + 4); }
	void AddQuadTriangles(VertType&& tl, VertType&& tr, VertType&& br, VertType&& bl) {
		const IndcType baseIndex = static_cast<IndcType>(verts.size());

		verts.emplace_back(tl); //0
		verts.emplace_back(tr); //1
		verts.emplace_back(br); //2
		verts.emplace_back(bl); //3

		//triangle 1 {tl, tr, bl}
		indcs.emplace_back(baseIndex + 3);
		indcs.emplace_back(baseIndex + 0);
		indcs.emplace_back(baseIndex + 1);

		//triangle 2 {bl, tr, br}
		indcs.emplace_back(baseIndex + 3);
		indcs.emplace_back(baseIndex + 1);
		indcs.emplace_back(baseIndex + 2);
	}

	// render with DrawElements(GL_LINES)
	void AddQuadLines(const VertType* vs) { AddQuadLines(vs + 0, vs + 1, vs + 2, vs + 3, vs + 4); }
	void AddQuadLines(VertType&& tl, VertType&& tr, VertType&& br, VertType&& bl) {
		const IndcType baseIndex = static_cast<IndcType>(verts.size());

		verts.emplace_back(tl); //0
		verts.emplace_back(tr); //1
		verts.emplace_back(br); //2
		verts.emplace_back(bl); //3

		indcs.emplace_back(baseIndex + 0);
		indcs.emplace_back(baseIndex + 1);
		indcs.emplace_back(baseIndex + 1);
		indcs.emplace_back(baseIndex + 2);
		indcs.emplace_back(baseIndex + 2);
		indcs.emplace_back(baseIndex + 3);
		indcs.emplace_back(baseIndex + 3);
		indcs.emplace_back(baseIndex + 0);
	}

	// render with DrawElements(GL_TRIANGLES)
	void MakeQuadsTriangles(const VertType* vs, int xDiv, int yDiv) { MakeQuadsTriangles(vs + 0, vs + 1, vs + 2, vs + 3, vs + 4, xDiv, yDiv); }
	void MakeQuadsTriangles(VertType&& tl, VertType&& tr, VertType&& br, VertType&& bl, int xDiv, int yDiv) {
		const IndcType baseIndex = static_cast<IndcType>(verts.size_t());
		float ratio;

		for (int y = 0; y < yDiv; ++y) {
			ratio = static_cast<float>(y) / static_cast<float>(yDiv);
			const VertType ml = mix(tl, bl, ratio);
			const VertType mr = mix(tr, br, ratio);
			for (int x = 0; y < xDiv; ++x) {
				ratio = static_cast<float>(x) / static_cast<float>(xDiv);

				verts.emplace_back(mix(ml, mr, ratio));
			}
		}

		for (int y = 0; y < yDiv - 1; ++y)
		for (int x = 0; y < xDiv - 1; ++x) {
			const IndcType tli = xDiv * (y + 0) + (x + 0) + baseIndex;
			const IndcType tri = xDiv * (y + 0) + (x + 1) + baseIndex;
			const IndcType bli = xDiv * (y + 1) + (x + 0) + baseIndex;
			const IndcType bri = xDiv * (y + 1) + (x + 1) + baseIndex;

			//triangle 1 {tl, tr, bl}
			indcs.emplace_back(tli);
			indcs.emplace_back(tri);
			indcs.emplace_back(bli);

			//triangle 2 {bl, tr, br}
			indcs.emplace_back(bli);
			indcs.emplace_back(tri);
			indcs.emplace_back(bri);
		}
	}

	// render with DrawArrays(GL_LINE_LOOP)
	void MakeQuadsLines(const VertType* vs, int xDiv, int yDiv) { MakeQuadsLines(vs + 0, vs + 1, vs + 2, vs + 3, vs + 4, xDiv, yDiv); }
	void MakeQuadsLines(const VertType& tl, const VertType& tr, const VertType& br, const VertType& bl, int xDiv, int yDiv) {
		float ratio;

		for (int x = 0; x < yDiv; ++x) {
			ratio = static_cast<float>(x) / static_cast<float>(xDiv);
			verts.emplace_back(mix(tl, tr, ratio));
			verts.emplace_back(mix(bl, br, ratio));
		}

		for (int y = 0; y < yDiv; ++y) {
			ratio = static_cast<float>(y) / static_cast<float>(yDiv);
			verts.emplace_back(mix(tl, bl, ratio));
			verts.emplace_back(mix(tr, br, ratio));
		}
	}

	void UploadVBO();
	void UploadEBO();

	void DrawArrays(uint32_t mode, bool rewind = true);
	void DrawElements(uint32_t mode, bool rewind = true);

	//develop compat
	void Submit(uint32_t mode) {
		if (indcs.size() - eboStartIndex > 0)
			DrawElements(mode);
		else
			DrawArrays(mode);
	}

	size_t SumElems() const { return verts.size(); }
	size_t SumIndcs() const { return indcs.size(); }
	size_t NumSubmits(bool indexed) const { return numSubmits[indexed]; }

	static Shader::IProgramObject& GetShader() { return shader.GetShader(); }
private:
	void CondInit();
	void InitVAO() const;
private:
	size_t vertCount0;
	size_t elemCount0;
	IStreamBufferConcept::Types bufferType;

	std::unique_ptr<IStreamBuffer<VertType>> vbo;
	std::unique_ptr<IStreamBuffer<IndcType>> ebo;

	VAO vao;

	std::vector<VertType> verts;
	std::vector<IndcType> indcs;

	size_t vboStartIndex = 0;
	size_t eboStartIndex = 0;

	size_t vboUploadIndex = 0;
	size_t eboUploadIndex = 0;

	// [0] := non-indexed, [1] := indexed
	std::array<size_t, 2> numSubmits = { 0, 0 };

	inline static RenderBufferShader<T> shader;

	static constexpr const char* vboTypeName = spring::TypeToCStr<VertType>();
	static constexpr IStreamBufferConcept::Types bufferTypeDefault = IStreamBufferConcept::Types::SB_BUFFERSUBDATA;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline void TypedRenderBuffer<T>::UploadVBO()
{
	size_t elemsCount = (verts.size() - vboUploadIndex);
	if (elemsCount <= 0)
		return;

	CondInit();

	if (verts.size() > vertCount0) {
		LOG_L(L_WARNING, "[TypedRenderBuffer<%s>::%s] Increase the number of elements here!", vboTypeName, __func__);
		vbo->Resize(verts.capacity());
		vertCount0 = verts.capacity();
	}

	//update on the GPU
	const VertType* clientPtr = verts.data();
	VertType* mappedPtr = vbo->Map(clientPtr, vboUploadIndex, elemsCount);

	if (!vbo->HasClientPtr())
		memcpy(mappedPtr, clientPtr + vboUploadIndex, elemsCount * sizeof(VertType));

	vbo->Unmap();
	vboUploadIndex += elemsCount;
}

template<typename T>
inline void TypedRenderBuffer<T>::UploadEBO()
{
	size_t elemsCount = (indcs.size() - eboUploadIndex);
	if (elemsCount <= 0)
		return;

	CondInit();

	if (indcs.size() > elemCount0) {
		LOG_L(L_WARNING, "[TypedRenderBuffer<%s>::%s] Increase the number of elements here!", vboTypeName, __func__);
		ebo->Resize(indcs.capacity());
		elemCount0 = indcs.capacity();
	}

	//update on the GPU
	const IndcType* clientPtr = indcs.data();
	IndcType* mappedPtr = ebo->Map(clientPtr, eboUploadIndex, elemsCount);

	if (!ebo->HasClientPtr())
		memcpy(mappedPtr, clientPtr + eboUploadIndex, elemsCount * sizeof(IndcType));

	ebo->Unmap();
	eboUploadIndex += elemsCount;

	return;
}

template<typename T>
inline void TypedRenderBuffer<T>::DrawArrays(uint32_t mode, bool rewind)
{
	UploadVBO();

	size_t elemsCount = (verts.size() - vboStartIndex);
	if (elemsCount <= 0)
		return;

	assert(vao.GetIdRaw() > 0);
	vao.Bind();
	glDrawArrays(mode, vbo->BufferElemOffset() + vboStartIndex, elemsCount);
	vao.Unbind();

	if (rewind)
		vboStartIndex += elemsCount;

	numSubmits[0] += 1;
}

template<typename T>
inline void TypedRenderBuffer<T>::DrawElements(uint32_t mode, bool rewind)
{
	UploadVBO();
	UploadEBO();

	size_t elemsCount = (indcs.size() - eboStartIndex);
	if (elemsCount <= 0)
		return;

	#define BUFFER_OFFSET(T, n) (reinterpret_cast<void*>(sizeof(T) * (n)))
	assert(vao.GetIdRaw() > 0);
	vao.Bind();
	glDrawElements(mode, elemsCount, GL_UNSIGNED_INT, BUFFER_OFFSET(uint32_t, ebo->BufferElemOffset() + eboStartIndex));
	vao.Unbind();
	#undef BUFFER_OFFSET

	if (rewind)
		eboStartIndex += elemsCount;

	numSubmits[1] += 1;
}


template<typename T>
inline void TypedRenderBuffer<T>::CondInit()
{
	if (vao.GetIdRaw() > 0)
		return;

	if (vertCount0 > 0)
		vbo = IStreamBuffer<VertType>::CreateInstance(GL_ARRAY_BUFFER        , vertCount0, std::string(vboTypeName), bufferType);

	if (elemCount0 > 0)
		ebo = IStreamBuffer<IndcType>::CreateInstance(GL_ELEMENT_ARRAY_BUFFER, elemCount0, std::string(vboTypeName), bufferType);

	InitVAO();
}

template<typename T>
inline void TypedRenderBuffer<T>::InitVAO() const
{
	assert(vbo);

	vao.Bind(); //will instantiate

	vbo->Bind();

	if (ebo)
		ebo->Bind();

	for (const AttributeDef& ad : T::attributeDefs) {
		glEnableVertexAttribArray(ad.index);
		glVertexAttribDivisor(ad.index, 0);

		//assume only float or float convertible values
		glVertexAttribPointer(ad.index, ad.count, ad.type, ad.normalize, ad.stride, ad.data);
	}

	vao.Unbind();

	vbo->Unbind();

	if (ebo)
		ebo->Unbind();

	//restore default state
	for (const AttributeDef& ad : T::attributeDefs) {
		glDisableVertexAttribArray(ad.index);
	}
}

//member template specializations

#define GET_TYPED_RENDER_BUFFER(T, idx) \
template<> \
inline TypedRenderBuffer<T>& RenderBuffer::GetTypedRenderBuffer<T>() \
{ \
	assert(dynamic_cast<TypedRenderBuffer<T>*>(typedRenderBuffers[idx].get())); \
	return *static_cast<TypedRenderBuffer<T>*>(typedRenderBuffers[idx].get()); \
}

GET_TYPED_RENDER_BUFFER(VA_TYPE_0   , 0)
GET_TYPED_RENDER_BUFFER(VA_TYPE_C   , 1)
GET_TYPED_RENDER_BUFFER(VA_TYPE_N   , 2)
GET_TYPED_RENDER_BUFFER(VA_TYPE_T   , 3)
GET_TYPED_RENDER_BUFFER(VA_TYPE_TN  , 4)
GET_TYPED_RENDER_BUFFER(VA_TYPE_TC  , 5)
GET_TYPED_RENDER_BUFFER(VA_TYPE_TNT , 6)
GET_TYPED_RENDER_BUFFER(VA_TYPE_2d0 , 7)
GET_TYPED_RENDER_BUFFER(VA_TYPE_2dC , 8)
GET_TYPED_RENDER_BUFFER(VA_TYPE_2dT , 9)
GET_TYPED_RENDER_BUFFER(VA_TYPE_2dTC, 10)

#undef GET_TYPED_RENDER_BUFFER