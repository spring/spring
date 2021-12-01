#include "FixedPipelineState.h"

#include <array>
#include <stdexcept>
#include <sstream>

#include "System/type2.h"
#include "System/float4.h"
#include "System/StringHash.h"
#include "System/Log/ILog.h"
#include "Rendering/GlobalRendering.h"

using namespace GL;
std::stack<FixedPipelineState> FixedPipelineState::statesChain = {};

FixedPipelineState::FixedPipelineState()
{
	if (statesChain.empty()) { //default state
		InferState();
	} else {
		*this = statesChain.top(); //copy&paste previous state
	}
}


template<typename ...T>
constexpr void FixedPipelineState::DumpState(const std::tuple<T...>& tuple)
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
constexpr void GL::FixedPipelineState::HashState(const std::tuple<T...>& tuple)
{
	const auto lambda = [this](auto&&... args) -> uint64_t {
		return ((hashString(reinterpret_cast<const char*>(&args), sizeof(args)) * 65521) +  ...);
	};
	stateHash += std::apply(lambda, tuple);
}

FixedPipelineState& GL::FixedPipelineState::InferState()
{
	{
		PolygonMode(GL_FILL);
	}
	{
		PolygonOffset(glGetFloatT(GL_POLYGON_OFFSET_FACTOR), glGetFloatT(GL_POLYGON_OFFSET_UNITS));
		PolygonOffsetFill(glIsEnabled(GL_POLYGON_OFFSET_FILL));
		PolygonOffsetLine(glIsEnabled(GL_POLYGON_OFFSET_LINE));
		PolygonOffsetPoint(glIsEnabled(GL_POLYGON_OFFSET_POINT));
	}
	{
		FrontFace(glGetIntT(GL_FRONT_FACE)); // GL_CCW
		CullFace(glGetIntT(GL_CULL_FACE_MODE)); // GL_BACK
		Culling(glIsEnabled(GL_CULL_FACE));
	}
	{
		float2 nf = glGetFloatT<float2>(GL_DEPTH_RANGE);
		DepthRange(nf.x, nf.y);
		DepthClamp(glIsEnabled(GL_DEPTH_CLAMP));
		DepthTest(glIsEnabled(GL_DEPTH_TEST));
		DepthMask(glGetIntT(GL_DEPTH_WRITEMASK));
		DepthFunc(glGetIntT(GL_DEPTH_FUNC));
	}
	{
		AlphaTest(glIsEnabled(GL_ALPHA_TEST));
		AlphaFunc(glGetIntT(GL_ALPHA_TEST_FUNC), glGetFloatT(GL_ALPHA_TEST_REF));
	}
	{
		Blending(glIsEnabled(GL_BLEND));
		// Does not return proper values until glBlendFunc has been called
		// PushBlendFunc(glGetIntT(GL_BLEND_SRC_ALPHA), glGetIntT(GL_BLEND_DST_ALPHA));
		BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		float4 blendCol = glGetFloatT<float4>(GL_BLEND_COLOR);
		BlendColor(blendCol.r, blendCol.g, blendCol.b, blendCol.a);
	}
	{
		ScissorTest(glIsEnabled(GL_SCISSOR_TEST));
		auto scissorRect = glGetIntT<std::array<int32_t, 4>>(GL_SCISSOR_BOX);
		ScissorRect(scissorRect[0], scissorRect[1], scissorRect[2], scissorRect[3]);
	}
	{
		StencilTest(glIsEnabled(GL_STENCIL_TEST));
	}
	{
		auto colorMask = glGetIntT<std::array<uint32_t, 4>>(GL_COLOR_WRITEMASK);
		ColorMask(
			static_cast<bool>(colorMask[0]),
			static_cast<bool>(colorMask[1]),
			static_cast<bool>(colorMask[2]),
			static_cast<bool>(colorMask[3])
		);
	}
	{
		Multisampling(glIsEnabled(GL_MULTISAMPLE));
		Multisampling(glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE));
		Multisampling(glIsEnabled(GL_SAMPLE_ALPHA_TO_ONE));
	}
	{
		PrimitiveRestart(glIsEnabled(GL_PRIMITIVE_RESTART));
		PrimitiveRestartIndex(glGetIntT<GLuint>(GL_PRIMITIVE_RESTART_INDEX));
	}
	{
		CubemapSeamless(glIsEnabled(GL_TEXTURE_CUBE_MAP_SEAMLESS));
	}
	{
		PointSize(glIsEnabled(GL_PROGRAM_POINT_SIZE));
	}
	{
		auto maxClipDists = glGetIntT(GL_MAX_CLIP_DISTANCES);
		for (int i = 0; i < maxClipDists; ++i)
			ClipDistance(i, glIsEnabled(GL_CLIP_DISTANCE0 + i));
	}
	{
		auto viewPort = glGetIntT<std::array<int32_t, 4>>(GL_VIEWPORT);
		Viewport(viewPort[0], viewPort[1], viewPort[2], viewPort[3]);
	}
	{
		lastActiveTexture = glGetIntT(GL_ACTIVE_TEXTURE) - GL_TEXTURE0;
	}
	for (int texRelUnit = 0; texRelUnit < CGlobalRendering::MAX_TEXTURE_UNITS; ++texRelUnit)
	{
		// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTextures.xhtml
		#define MAKE_TEX_QUERY_BIND_PAIR(type) std::make_pair(GL_TEXTURE_BINDING_##type, GL_TEXTURE_##type)
		constexpr static std::array<std::pair<GLenum, GLenum>, 11> texTypes = {
			MAKE_TEX_QUERY_BIND_PAIR(1D),
			MAKE_TEX_QUERY_BIND_PAIR(2D),
			MAKE_TEX_QUERY_BIND_PAIR(3D),
			MAKE_TEX_QUERY_BIND_PAIR(1D_ARRAY),
			MAKE_TEX_QUERY_BIND_PAIR(2D_ARRAY),
			MAKE_TEX_QUERY_BIND_PAIR(RECTANGLE),
			MAKE_TEX_QUERY_BIND_PAIR(BUFFER),
			MAKE_TEX_QUERY_BIND_PAIR(CUBE_MAP),
			MAKE_TEX_QUERY_BIND_PAIR(CUBE_MAP_ARRAY),
			MAKE_TEX_QUERY_BIND_PAIR(2D_MULTISAMPLE),
			MAKE_TEX_QUERY_BIND_PAIR(2D_MULTISAMPLE_ARRAY)
		};
		#undef MAKE_TEX_QUERY_BIND_PAIR

		bool found = false;
		for (const auto [queryType, bindType] : texTypes) {
			glActiveTexture(GL_TEXTURE0 + texRelUnit);
			GLint texID = glGetIntT(queryType);
			if (texID > 0) {
				BindTexture(texRelUnit, bindType, texID);
				found = true;
				break;
			}
		}

		if (found)
			continue;

		BindTexture(texRelUnit, GL_TEXTURE_2D, 0u);
	}
	glActiveTexture(lastActiveTexture + GL_TEXTURE0); //revert just in case

	return *this;
}

void FixedPipelineState::BindUnbind(const bool bind) const
{
	if (!bind)
		statesChain.pop();

	for (const auto& [onbind, func] : customOnBindUnbind) {
		if (bind == onbind)
			func();
	}

	if (bind) {
		for (const auto& [funcName, namedState] : namedStates) {
            if (statesChain.empty()) { /*default state*/
                namedState.apply();
                continue;
            }
            const auto& prev = statesChain.top().namedStates;
            const auto& prevStateIt = prev.find(funcName);
            if (prevStateIt == prev.cend()) { /*haven't seen this state in previous states*/
                /* TODO: do something to save revert state */
                namedState.apply();
                continue;
            }
            if (prevStateIt->second != namedState) { /*previous state's args are different to this state args*/
                namedState.apply();
                continue;
            }
        }
	} else {
        for (const auto& [funcName, namedState] : namedStates) {
            if (statesChain.empty()) { /*default state*/
                continue;
            }
            const auto& prev = statesChain.top().namedStates;
            const auto& prevStateIt = prev.find(funcName);
            if (prevStateIt == prev.cend()) { /*haven't seen this state in previous states*/
                /* TODO: do something to save revert state */
                throw std::runtime_error("[FixedPipelineState::BindUnbind] Cannot revert state that has no default value");
            }
            if (prevStateIt->second != namedState) { /*previous state's args are different to this state args*/
                prevStateIt->second.apply();
                continue;
            }
        }
	}

	//now enable/disable states
	for (const auto [state, status] : binaryStates) {
		/*
		  b, s ==> e
		  1, 1 ==> 1
		  0, 1 ==> 0
		  1, 1 ==> 0
		  0, 0 ==> 1
		*/
		const bool en = !(bind ^ status);
		if (en)
			glEnable(state);
		else
			glDisable(state);
	}

	if (lastActiveTexture < CGlobalRendering::MAX_TEXTURE_UNITS)
		glActiveTexture(lastActiveTexture + GL_TEXTURE0);
	else {
		if (!bind && !statesChain.empty()) {
			const auto prevLastActiveTexture = statesChain.top().lastActiveTexture;
			if (prevLastActiveTexture < CGlobalRendering::MAX_TEXTURE_UNITS)
				glActiveTexture(prevLastActiveTexture + GL_TEXTURE0);
		}
	}

	for (const auto& [onbind, func] : customOnBindUnbind) {
		if (bind == onbind)
			func();
	}

	if (bind) {
		statesChain.push(*this);
	}
}