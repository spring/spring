/// @ref ext_scalar_float_sized
/// @file glm/ext/scalar_float_sized.hpp
///
/// @defgroup ext_scalar_float_sized GLM_EXT_scalar_float_sized
/// @ingroup ext
///
/// Exposes sized float scalar types.
///
/// Include <glm/ext/scalar_float_sized.hpp> to use the features of this extension.
///
/// @see ext_scalar_int_sized
/// @see ext_scalar_uint_sized

#pragma once

#include "../detail/setup.hpp"

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_EXT_scalar_float_sized extension included")
#endif

namespace glm
{
	/// @addtogroup ext_scalar_float_sized
	/// @{

	/// Single precision floating-point numbers.
	typedef float			float32;


#	ifndef GLM_FORCE_SINGLE_ONLY

	/// Double precision floating-point numbers.
	typedef double			float64;

#	endif//GLM_FORCE_SINGLE_ONLY

	/// @}
}//namespace glm
