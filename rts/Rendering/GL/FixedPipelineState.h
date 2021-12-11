#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <stack>
#include <vector>
#include <tuple>
#include <cstring>
#include <sstream>

#include "System/Log/ILog.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/Shader.h"

namespace GL {
	template<typename GLgetFunc, typename GLparamType, typename ReturnType>
	static ReturnType glGetT(GLgetFunc glGetFunc, GLenum param) {
		ReturnType ret;

		constexpr size_t arrSize = sizeof(ret);
		static std::array<uint8_t, arrSize> arr;
		arr = {0};

		glGetError();
		glGetFunc(param, reinterpret_cast<GLparamType*>(arr.data()));
		assert(glGetError() == GL_NO_ERROR);
		memcpy(reinterpret_cast<GLparamType*>(&ret), arr.data(), arrSize);

		return ret;
	}

	template<typename ReturnType = int>
	static ReturnType glGetIntT(GLenum param) {
		return (glGetT<decltype(&glGetIntegerv), GLint, ReturnType>(glGetIntegerv, param));
	}

	template<typename ReturnType = float>
	static ReturnType glGetFloatT(GLenum param) {
		return (glGetT<decltype(&glGetFloatv), GLfloat, ReturnType>(glGetFloatv, param));
	};

	class NamedSingleState {
	public:
		template<typename F, typename Args> NamedSingleState(F func, const Args& args)
			: object{ std::move(new ObjectModel<F, Args>(func, args)) }
		{}

		NamedSingleState() = delete;

		bool operator !=(const NamedSingleState& rhs) const {
			return !object->isSame(*rhs.object);
		}
		bool operator ==(const NamedSingleState& rhs) const {
			return object->isSame(*rhs.object);
		}

		void apply() const {
			object->apply();
		}

	private:
		struct ObjectConcept {
			virtual ~ObjectConcept() = default;
			virtual void apply() const = 0;
			virtual const std::type_info& typeInfo() const = 0;
			virtual bool isSame(const ObjectConcept& rhs) const = 0;
		};

		template<typename F, typename T> struct ObjectModel : ObjectConcept {
			ObjectModel(F func_, const T& args_) :
				func( func_ ), args( args_ )

			{}

			void apply() const override {
				std::apply(func, args);
			}
			const std::type_info& typeInfo() const override {
				return typeid(args);
			}
			bool isSame(const ObjectConcept& rhs) const override {
				if (typeInfo() != rhs.typeInfo())
					return false;

				return args == static_cast<const ObjectModel&>(rhs).args;
			}
		private:
			const F& func;
			const T& args;
		};

		std::shared_ptr<ObjectConcept> object;
	};

	#define DEBUG_PIPELINE_STATE 1
	class FixedPipelineState {
	private:
		//template <typename Result, typename ...Args> using vararg_function = std::function< Result(Args...) >;
		using onBindUnbindFunc = std::function<void()>;
	public:
		FixedPipelineState();
		FixedPipelineState(FixedPipelineState&& rh) = default; //move
		FixedPipelineState(const FixedPipelineState& rh) = default; //copy

		FixedPipelineState& PolygonMode(GLenum mode) { return CommonNamedState(__func__, glPolygonMode, GL_FRONT_AND_BACK, mode); }

		FixedPipelineState& PolygonOffset(GLfloat factor, GLfloat units) { return CommonNamedState(__func__, glPolygonOffset, factor, units); }
		FixedPipelineState& PolygonOffsetFill(bool b)  { return CommonBinaryState(GL_POLYGON_OFFSET_FILL , b); }
		FixedPipelineState& PolygonOffsetLine(bool b)  { return CommonBinaryState(GL_POLYGON_OFFSET_LINE , b); }
		FixedPipelineState& PolygonOffsetPoint(bool b) { return CommonBinaryState(GL_POLYGON_OFFSET_POINT, b); }

		FixedPipelineState& FrontFace(GLenum mode) { return CommonNamedState(__func__, glFrontFace, mode); }
		FixedPipelineState& Culling(bool b) { return CommonBinaryState(GL_CULL_FACE_MODE, b); }
		FixedPipelineState& CullFace(GLenum mode) { return CommonNamedState(__func__, glCullFace, mode); }

		FixedPipelineState& DepthMask(bool b) { return CommonNamedState(__func__, glDepthMask, b); }
		FixedPipelineState& DepthRange(GLfloat n, GLfloat f) { return CommonNamedState(__func__, glDepthRangef, n, f); }
		FixedPipelineState& DepthClamp(bool b) { return CommonBinaryState(GL_DEPTH_CLAMP, b); }
		FixedPipelineState& DepthTest(bool b) { return CommonBinaryState(GL_DEPTH_TEST, b); }
		FixedPipelineState& DepthFunc(GLenum func) { return CommonNamedState(__func__, glDepthFunc, func); }

		// compat profile only
		FixedPipelineState& AlphaTest(bool b) { return CommonBinaryState(GL_ALPHA_TEST, b); }
		FixedPipelineState& AlphaFunc(GLenum func, GLfloat ref) { return CommonNamedState(__func__, glAlphaFunc, func, ref); }

		// TODO : expand
		FixedPipelineState& Blending(bool b) { return CommonBinaryState(GL_BLEND, b); }
		FixedPipelineState& BlendFunc(GLenum sfactor, GLenum dfactor) { return CommonNamedState(__func__, glBlendFunc, sfactor, dfactor); }
		FixedPipelineState& BlendColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { return CommonNamedState(__func__, glBlendColor, r, g, b, a); }

		FixedPipelineState& StencilTest(bool b) { return CommonBinaryState(GL_STENCIL_TEST, b); }

		FixedPipelineState& ColorMask(bool r, bool g, bool b, bool a) { return CommonNamedState(__func__, glColorMask, r, g, b, a); }

		FixedPipelineState& Multisampling(bool b) { return CommonBinaryState(GL_MULTISAMPLE, b); }
		FixedPipelineState& AlphaToCoverage(bool b) { return CommonBinaryState(GL_SAMPLE_ALPHA_TO_COVERAGE, b); }
		FixedPipelineState& AlphaToOne(bool b) { return CommonBinaryState(GL_SAMPLE_ALPHA_TO_ONE, b); }

		FixedPipelineState& PrimitiveRestart(bool b) { return CommonBinaryState(GL_PRIMITIVE_RESTART, b); }
		FixedPipelineState& PrimitiveRestartIndex(GLuint index) { return CommonNamedState(__func__, glPrimitiveRestartIndex, index); }

		FixedPipelineState& CubemapSeamless(bool b) { return CommonBinaryState(GL_TEXTURE_CUBE_MAP_SEAMLESS, b); }

		FixedPipelineState& PointSize(bool b) { return CommonBinaryState(GL_PROGRAM_POINT_SIZE, b); }

		FixedPipelineState& ClipDistance(GLenum relClipSp, bool b) { return CommonBinaryState(GL_CLIP_DISTANCE0 + relClipSp, b); }

		FixedPipelineState& ScissorTest(bool b) { return CommonBinaryState(GL_SCISSOR_TEST, b); }
		FixedPipelineState& ScissorRect(GLint x, GLint y, GLint w, GLint h) { return CommonNamedState(__func__, glScissor, x, y, w, h); }

		FixedPipelineState& Viewport(GLint x, GLint y, GLint w, GLint h) { return CommonNamedState(__func__, glViewport, x, y, w, h); }

		FixedPipelineState& BindTexture(GLenum texRelUnit, GLenum texType, GLuint texID) { return CommonNamedState(__func__, BindTextureProxy, texRelUnit + GL_TEXTURE0, texType, texID); };
		FixedPipelineState& LastActiveTexture(GLenum texRelUnit) { lastActiveTexture = texRelUnit + GL_TEXTURE0; return *this; }

		//FixedPipelineState& BindShader(Shader::IProgramObject* po) {  }

		/// applied in the end
		FixedPipelineState& OnBind(onBindUnbindFunc func) { customOnBindUnbind.emplace_back(std::make_pair(true, func)); return *this; };
		/// applied in the beginning
		FixedPipelineState& OnUnbind(onBindUnbindFunc func) { customOnBindUnbind.emplace_back(std::make_pair(false, func)); return *this; };

		FixedPipelineState& InferState();
	public:
		FixedPipelineState& operator=(const FixedPipelineState& other) = default; //copy
		FixedPipelineState& operator=(FixedPipelineState&& other) = default; //move
	public:
		void Bind() const { BindUnbind(true); }
		void Unbind() const { BindUnbind(false); }
	private:
		void BindUnbind(const bool bind) const;

		template <typename... T>
		constexpr void DumpState(const std::tuple<T...>& tuple);
		template <typename... T>
		constexpr void HashState(const std::tuple<T...>& tuple);
	private:
		static void BindTextureProxy(GLenum texUnit, GLenum texType, GLuint texID) { glActiveTexture(texUnit); glBindTexture(texType, texID); }
	private:
		FixedPipelineState& CommonBinaryState(const GLenum state, bool b) {
			const auto argsTuple = std::make_tuple(state, static_cast<GLboolean>(b));
			const auto fullTuple = std::tuple_cat(std::make_tuple("EnableDisable"), argsTuple);

			DumpState(fullTuple);
			HashState(fullTuple);

			binaryStates[state] = b;
			return *this;
		}

		template <class F, class ...Args>
		FixedPipelineState& CommonNamedState(const char* funcName, F func, Args... args) {
			const auto argsTuple = std::make_tuple(args...);
			const auto fullTuple = std::tuple_cat(std::make_tuple(funcName), argsTuple);

			DumpState(fullTuple);
			HashState(fullTuple);

			namedStates.emplace(std::string(funcName), std::move(NamedSingleState(func, argsTuple)));
			return *this;
		}

	private:
		std::unordered_map<GLenum, bool> binaryStates;
		std::unordered_map<std::string, NamedSingleState> namedStates;

		std::vector<std::pair<bool, onBindUnbindFunc>> customOnBindUnbind;

		uint64_t stateHash = 0u;
		GLuint lastActiveTexture = ~0u;
	private:
		static std::stack<FixedPipelineState, std::vector<FixedPipelineState>> statesChain;
	};

	class ScopedState {
	public:
		ScopedState(const FixedPipelineState& state) : s{ state } { s.Bind(); }
		~ScopedState() { s.Unbind(); }
	private:
		const FixedPipelineState s;
	};


	template<typename ...T>
	inline constexpr void FixedPipelineState::DumpState(const std::tuple<T...>& tuple)
	{
	#if (DEBUG_PIPELINE_STATE == 1)
		std::ostringstream ss;
		ss << "[ ";
		std::apply([&ss](auto&&... args) {((ss << +args << ", "), ...); }, tuple);
		ss.seekp(-2, ss.cur);
		ss << " ]";

		LOG_L(L_NOTICE, "[FixedPipelineState::DumpState] %s", ss.str().c_str());
	#endif // (DEBUG_PIPELINE_STATE == 1)
	}

	template<typename ...T>
	inline constexpr void GL::FixedPipelineState::HashState(const std::tuple<T...>& tuple)
	{
		const auto lambda = [this](auto&&... args) -> uint64_t {
			return ((hashString(reinterpret_cast<const char*>(&args), sizeof(args)) * 65521) + ...);
		};
		stateHash += std::apply(lambda, tuple);
	}

}