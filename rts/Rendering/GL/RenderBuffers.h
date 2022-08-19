#pragma once

#include "StreamBuffer.h"
#include "VertexArrayTypes.h"
#include "VAO.h"

#include "System/TypeToStr.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"

#include "fmt/format.h"
#include "lib/fmt/printf.h"

#include <string>
#include <sstream>
#include <memory>
#include <array>
#include <vector>
#include <iterator>
#include <algorithm>
#include <type_traits>

template <typename T>
class TypedRenderBuffer;

class RenderBuffer {
public:
	static void InitStatic();
	static void KillStatic();

	RenderBuffer()
		:initCapacity{ 0, 0 }
	{}
	RenderBuffer(std::array<size_t, 2> c)
		:initCapacity{ c }
	{}

	virtual ~RenderBuffer() {}

	template <typename T>
	static TypedRenderBuffer<T>& GetTypedRenderBuffer();

	static void SwapStandardRenderBuffers() {
		for (const auto& trb : typedRenderBuffers) {
			if (trb) {
				trb->SwapBuffer();
			}
		}
	}

	virtual void SwapBuffer() = 0;
	virtual const char* GetBufferName() const = 0;
	virtual std::array<size_t, 2> GetBuffersCapacity() const = 0;

	virtual size_t SumElems() const = 0;
	virtual size_t SumIndcs() const = 0;
	virtual size_t NumSubmits(bool indexed) const = 0;

	std::array<size_t, 2> GetSubmitNum() const { return numSubmits; }
	std::array<size_t, 2> GetMaxSize()   const { return maxSize;    }
	std::array<size_t, 2> GetInitialCapacity()   const { return initCapacity; }
protected:
	// [0] := non-indexed, [1] := indexed
	std::array<size_t, 2> numSubmits = { 0, 0 };

	// [0] := vertex, [1] := index
	std::array<size_t, 2> maxSize = { 0, 0 };

	// [0] := vertex, [1] := index
	std::array<size_t, 2> initCapacity = { 0, 0 };
private:
	static std::array<std::unique_ptr<RenderBuffer>, 11> typedRenderBuffers;
public:
	static auto GetAllRenderBuffers() -> const decltype(typedRenderBuffers)& { return typedRenderBuffers; };
};

template <typename T>
class RenderBufferShader {
public:
	static Shader::IProgramObject& GetShader() {

		Shader::IProgramObject* shader = shaderHandler->GetProgramObject(poClass, typeName);

		if (shader) {
			if (!shader->IsReloadRequested())
				return *shader;
			else {
				shaderHandler->ReleaseProgramObject(poClass, typeName);
				shader = nullptr;
			}
		}

		if (!shader)
			shader = shaderHandler->CreateProgramObject(poClass, typeName);

#ifndef HEADLESS
		assert(shader);
#endif

		std::string vertSrc = GetShaderTemplate("GLSL/RenderBufferVertTemplate.glsl");
		std::string fragSrc = GetShaderTemplate("GLSL/RenderBufferFragTemplate.glsl");

		std::string vsInputs;
		std::string varyingsData;
		std::string vsAssignment;
		std::string vsPosVertex;

		std::string vsHeader;
		std::string fsHeader;

		GetShaderHeaders(vsHeader, fsHeader);
		GetAttributesStrings(vsInputs, varyingsData, vsAssignment, vsPosVertex);

		static const char* fmtString = "%s Data {%s%s};";
		const std::string varyingsVS = (varyingsData.empty()) ? "" : fmt::sprintf(fmtString, "out", nl, varyingsData);
		const std::string varyingsFS = (varyingsData.empty()) ? "" : fmt::sprintf(fmtString, "in", nl, varyingsData);

		vertSrc = fmt::sprintf(vertSrc,
			vsHeader,
			vsInputs,
			varyingsVS,
			vsAssignment,
			vsPosVertex
		);

		const std::string fragOutput = GetFragOutput();
		fragSrc = fmt::sprintf(fragSrc,
			fsHeader,
			varyingsFS,
			fragOutput
		);

		shader->AttachShaderObject(shaderHandler->CreateShaderObject(vertSrc, "", GL_VERTEX_SHADER));
		shader->AttachShaderObject(shaderHandler->CreateShaderObject(fragSrc, "", GL_FRAGMENT_SHADER));

		if (!globalRendering->supportExplicitAttribLoc) {
			for (const AttributeDef& ad : T::attributeDefs) {
				shader->BindAttribLocation(fmt::format("a{}", ad.name), ad.index);
			}
		}

		shader->Link();

		shader->Enable();
		shader->Disable();

		shader->Validate();
#ifndef HEADLESS
		assert(shader->IsValid());
#endif
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

	static void GetShaderHeaders(std::string& vsHeader, std::string& fsHeader) {
		if (globalRendering->supportExplicitAttribLoc) {
			vsHeader = fmt::format("{}{}{}", "#version 150 compatibility", nl, "#extension GL_ARB_explicit_attrib_location : require");
		}
		else {
			vsHeader = "#version 150 compatibility";
		}

		fsHeader = "#version 150";
	}

	static void GetAttributesStrings(std::string& vsInputs, std::string& varyings, std::string& vsAssignment, std::string& vsPosVertex) {
		static constexpr const char* vsInputsFmtYLoc  = "layout(location = {indx}) in {type} a{name};";
		static constexpr const char* vsInputsFmtNLoc = "in {type} a{name};";
		static constexpr const char* varyingsFmt     = "\t{type} v{name};";
		static constexpr const char* vsAssignmentFmt = "\tv{name} = a{name};";

		assert(T::attributeDefs[0].index == 0);
		assert(T::attributeDefs[0].name == "pos");
		assert(T::attributeDefs[0].count == 2 || T::attributeDefs[0].count == 3);

		vsPosVertex = (T::attributeDefs[0].count == 2) ? "vec4(apos, 0.0, 1.0)" : "vec4(apos, 1.0)";

		std::vector<std::string> vsInputsVec;
		std::vector<std::string> varyingsVec;
		std::vector<std::string> vsAssignmentVec;

		for (const AttributeDef& ad : T::attributeDefs) {
			if (globalRendering->supportExplicitAttribLoc)
				vsInputsVec.emplace_back(fmt::format(vsInputsFmtYLoc,
					fmt::arg("indx", ad.index),
					fmt::arg("type", TypeToString(ad)),
					fmt::arg("name", ad.name)
				));
			else
				vsInputsVec.emplace_back(fmt::format(vsInputsFmtNLoc,
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
	static constexpr const char* poClass = "[RenderBufferShader]";
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
	return "\toutColor = texture(tex, vuv);";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_TN>::GetFragOutput()
{
	return "\toutColor = texture(tex, vuv);";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_TC>::GetFragOutput()
{
	return "\toutColor = vcolor * texture(tex, vuv);";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_TNT>::GetFragOutput()
{
	return "\toutColor = texture(tex, vuv);";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_2D0>::GetFragOutput()
{
	return "\toutColor = vec4(1.0);";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_2DC>::GetFragOutput()
{
	return "\toutColor = vcolor;";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_2DT>::GetFragOutput()
{
	return "\toutColor = texture(tex, vuv);";
}

template<>
inline const std::string RenderBufferShader<VA_TYPE_2DTC>::GetFragOutput()
{
	return "\toutColor = vcolor * texture(tex, vuv);";
}

template <typename T>
class TypedRenderBuffer : public RenderBuffer {
public:
	using VertType = T;
	using IndcType = uint32_t;

	TypedRenderBuffer<T>()
		: RenderBuffer()
		, vertCount0{ 0 }
		, elemCount0{ 0 }
		, bufferType{ bufferTypeDefault }
	{}
	TypedRenderBuffer<T>(size_t vertCount0_, size_t elemCount0_, IStreamBufferConcept::Types bufferType_ = bufferTypeDefault)
		: RenderBuffer({ vertCount0_, elemCount0_ })
		, vertCount0 { vertCount0_ }
		, elemCount0 { elemCount0_ }
		, bufferType { bufferType_ }
	{
		verts.reserve(vertCount0);
		indcs.reserve(elemCount0);
	}

	~TypedRenderBuffer<T>() override {
		verts = {};
		indcs = {};
		vbo = {};
		ebo = {};
	}

	void Clear() {
		maxSize = std::max(maxSize, { verts.size(), indcs.size() });

		// clear
		verts.clear();
		indcs.clear();

		vboStartIndex = 0;
		eboStartIndex = 0;

		vboUploadIndex = 0;
		eboUploadIndex = 0;

		numSubmits = { 0, 0 };
	}

	void SwapBuffer() override {
		if (vbo)
			vbo->SwapBuffer();
		if (ebo)
			ebo->SwapBuffer();

		Clear();
  }

	const char* GetBufferName() const override {
		return vboTypeName;
	}

	std::array<size_t, 2> GetBuffersCapacity() const override {
		return std::array{ verts.capacity(), indcs.capacity() };
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
	void AddVertices(const std::vector<VertType>& vertices) {
		verts.insert(verts.end(), vertices.begin(), vertices.end());
	}
	void AddVertices(std::initializer_list<VertType>&& vertices) {
		verts.insert(verts.end(), vertices.begin(), vertices.end());
	}
	//106.0 compat
	void SafeAppend(VertType&& v) { AddVertex(std::forward<VertType&&>(v)); }

	void UpdateVertex(VertType&& v, size_t at) {
		assert(at < verts.size());
		verts.emplace(at, v);
	}
	void UpdateVertices(const std::vector<VertType>& vs, size_t at) {
		size_t cnt = 0;
		for (auto&& v : vs) {
			assert(cnt + at < verts.size());
			verts.emplace(cnt + at, v);
			++cnt;
		}
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
	void AddQuadTriangles(const std::vector<VertType>& vs) {
		if (vs.empty())
			return;

		assert(vs.size() % 4 == 0);
		for (size_t i = 0; i < vs.size(); i += 4) {
			AddQuadTrianglesImpl(vs[i + 0], vs[i + 1], vs[i + 2], vs[i + 3]);
		}
	}
	template<std::size_t N>
	void AddQuadTriangles(const VertType(&vs)[N]) {
		static_assert(N == 4);
		AddQuadTrianglesImpl(vs[0], vs[1], vs[2], vs[3]);
	}
	void AddQuadTriangles(VertType&& tl, VertType&& tr, VertType&& br, VertType&& bl) { AddQuadTrianglesImpl(tl, tr, br, bl); }
	void AddQuadTriangles(const VertType& tl, const VertType& tr, const VertType& br, const VertType& bl) { AddQuadTrianglesImpl(std::move(tl), std::move(tr), std::move(br), std::move(bl)); }

	// render with DrawElements(GL_LINES)
	void AddQuadLines(const std::vector<VertType>& vs) {
		if (vs.empty())
			return;

		assert(vs.size() % 4 == 0);
		for (size_t i = 0; i < vs.size(); i += 4) {
			AddQuadLinesImpl(vs[i + 0], vs[i + 1], vs[i + 2], vs[i + 3]);
		}
	}
	template<std::size_t N>
	void AddQuadLines(const VertType(&vs)[N]) {
		static_assert(N == 4);
		AddQuadLinesImpl(vs[0], vs[1], vs[2], vs[3]);
	}
	void AddQuadLines(VertType&& tl, VertType&& tr, VertType&& br, VertType&& bl) { AddQuadLinesImpl(tl, tr, br, bl); }
	void AddQuadLines(const VertType& tl, const VertType& tr, const VertType& br, const VertType& bl) { AddQuadLinesImpl(std::move(tl), std::move(tr), std::move(br), std::move(bl)); }

	// render with DrawElements(GL_TRIANGLES)
	template<std::size_t N>
	void MakeQuadsTriangles(const VertType(&vs)[N], int xDiv, int yDiv) {
		static_assert(N == 4);
		MakeQuadsTrianglesImpl(vs[0], vs[1], vs[2], vs[3], xDiv, yDiv);
	}
	void MakeQuadsTriangles(VertType&& tl, VertType&& tr, VertType&& br, VertType&& bl, int xDiv, int yDiv) { MakeQuadsTrianglesImpl(tl, tr, br, bl, xDiv, yDiv); }
	void MakeQuadsTriangles(const VertType& tl, const VertType& tr, const VertType& br, const VertType& bl, int xDiv, int yDiv) { MakeQuadsTrianglesImpl(std::move(tl), std::move(tr), std::move(br), std::move(bl), xDiv, yDiv); }

	// render with DrawArrays(GL_LINE_LOOP)
	template<std::size_t N>
	void MakeQuadsLines(const VertType(&vs)[N], int xDiv, int yDiv) {
		static_assert(N == 4);
		MakeQuadsLinesImpl(vs[0], vs[1], vs[2], vs[3], xDiv, yDiv);
	}
	void MakeQuadsLines(VertType&& tl, VertType&& tr, VertType&& br, VertType&& bl, int xDiv, int yDiv) { MakeQuadsLinesImpl(tl, tr, br, bl, xDiv, yDiv); }
	void MakeQuadsLines(const VertType& tl, const VertType& tr, const VertType& br, const VertType& bl, int xDiv, int yDiv) { MakeQuadsLinesImpl(std::move(tl), std::move(tr), std::move(br), std::move(bl), xDiv, yDiv); }


	void UploadVBO();
	void UploadEBO();

	void DrawArrays(uint32_t mode, bool rewind = true);
	void DrawElements(uint32_t mode, bool rewind = true);

	bool ShouldSubmit(bool indexed) const {
		if (indexed)
			return ((indcs.size() - eboUploadIndex) > 0 || (indcs.size() - eboStartIndex) > 0);
		else
			return ((verts.size() - vboUploadIndex) > 0 || (verts.size() - vboStartIndex) > 0);
	}
	bool ShouldSubmit() const {
		return ShouldSubmit(false) || ShouldSubmit(true);
	}

	//develop compat
	void Submit(uint32_t mode) {
		DrawElements(mode);
		DrawArrays(mode);
	}

	size_t SumElems() const override { return verts.size(); }
	size_t SumIndcs() const override { return indcs.size(); }

	const std::vector<VertType>& GetElems() const { return verts; }
	      std::vector<VertType>& GetElems()       { return verts; }
	const std::vector<IndcType>& GetIndcs() const { return indcs; }
	      std::vector<IndcType>& GetIndcs()       { return indcs; }

	size_t NumSubmits(bool indexed) const override { return numSubmits[indexed]; }

	//check everything is uploaded and submitted
	bool AssertSubmission() const {
		return
			(indcs.size() - eboUploadIndex == 0) &&
			(verts.size() - vboUploadIndex == 0) &&
			(indcs.size() - eboStartIndex  == 0) &&
			(verts.size() - vboStartIndex  == 0);
	}

	static Shader::IProgramObject& GetShader() { return shader.GetShader(); }
private:
	template<
		typename TT = VertType,
		typename = typename std::enable_if_t<std::is_same_v<VertType, typename std::decay_t<T>>>
	>
	void AddQuadTrianglesImpl(TT&& tl, TT&& tr, TT&& br, TT&& bl) {
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

	template<
		typename TT = VertType,
		typename = typename std::enable_if_t<std::is_same_v<VertType, typename std::decay_t<T>>>
	>
	void AddQuadLinesImpl(TT&& tl, TT&& tr, TT&& br, TT&& bl) {
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


	template<
		typename TT = VertType,
		typename = typename std::enable_if_t<std::is_same_v<VertType, typename std::decay_t<T>>>
	>
	void MakeQuadsTrianglesImpl(TT&& tl, TT&& tr, TT&& br, TT&& bl, int xDiv, int yDiv) {
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

	template<
		typename TT = VertType,
		typename = typename std::enable_if_t<std::is_same_v<VertType, typename std::decay_t<T>>>
	>
	void MakeQuadsLinesImpl(TT&& tl, TT&& tr, TT&& br, TT&& bl, int xDiv, int yDiv) {
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

	size_t vertsCount = (verts.size() - vboStartIndex);
	if (vertsCount <= 0)
		return;
#ifndef HEADLESS
	assert(vao.GetIdRaw() > 0);
#endif
	vao.Bind();
	glDrawArrays(mode, vbo->BufferElemOffset() + vboStartIndex, vertsCount);
	vao.Unbind();

	if (rewind)
		vboStartIndex += vertsCount;

	numSubmits[0] += 1;
}

template<typename T>
inline void TypedRenderBuffer<T>::DrawElements(uint32_t mode, bool rewind)
{
	UploadVBO();
	UploadEBO();

	size_t indcsCount = (indcs.size() - eboStartIndex);
	size_t vertsCount = (verts.size() - vboStartIndex);
	if (indcsCount <= 0)
		return;

	#define BUFFER_OFFSET(T, n) (reinterpret_cast<void*>(sizeof(T) * (n)))
#ifndef HEADLESS
	assert(vao.GetIdRaw() > 0);
#endif
	vao.Bind();
	glDrawElements(mode, indcsCount, GL_UNSIGNED_INT, BUFFER_OFFSET(uint32_t, ebo->BufferElemOffset() + eboStartIndex));
	vao.Unbind();
	#undef BUFFER_OFFSET

	if (rewind) {
		eboStartIndex += indcsCount;
		vboStartIndex += vertsCount;
	}

	vboStartIndex = verts.size();

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
GET_TYPED_RENDER_BUFFER(VA_TYPE_2D0 , 7)
GET_TYPED_RENDER_BUFFER(VA_TYPE_2DC , 8)
GET_TYPED_RENDER_BUFFER(VA_TYPE_2DT , 9)
GET_TYPED_RENDER_BUFFER(VA_TYPE_2DTC, 10)

#undef GET_TYPED_RENDER_BUFFER