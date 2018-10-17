/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLEW_STUB_H_
#define _GLEW_STUB_H_

#undef GL_GLEXT_LEGACY
#define GL_GLEXT_PROTOTYPES
#ifdef _WIN32
# define _GDI32_
# ifdef _DLL
#  undef _DLL
# endif
# include <windows.h>
#endif

#if defined(__APPLE__)
	#include <OpenGL/gl3.h>
	#include <OpenGL/gl3ext.h>
#else
	#include <GL/gl.h>
	#include <GL/glext.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define GLEW_GET_FUN(x) x

#define GLEW_VERSION 1
#define GLEW_VERSION_1_4 GL_TRUE
#define GLEW_VERSION_2_0 GL_FALSE
#define GLEW_VERSION_3_0 GL_FALSE

#define GLEW_ARB_vertex_program2 GL_FALSE
#define GLEW_ARB_depth_clamp GL_FALSE
#define GLEW_EXT_framebuffer_blit GL_FALSE
#define GLEW_EXT_framebuffer_object GL_FALSE
#define GLEW_EXT_stencil_two_side GL_TRUE
#define GLEW_ARB_draw_buffers GL_FALSE
#define GLEW_EXT_pixel_buffer_object GL_FALSE
#define GLEW_ARB_map_buffer_range GL_FALSE
#define GLEW_EXT_texture_filter_anisotropic GL_FALSE
#define GLEW_ARB_texture_float GL_FALSE
#define GLEW_ARB_texture_non_power_of_two GL_TRUE
#define GLEW_ARB_texture_env_combine GL_TRUE
#define GLEW_ARB_texture_rectangle GL_TRUE
#define GLEW_ARB_texture_compression GL_TRUE
#define GLEW_ARB_texture_env_dot3 GL_FALSE
#define GLEW_EXT_texture_edge_clamp GL_FALSE
#define GLEW_ARB_texture_border_clamp GL_TRUE
#define GLEW_EXT_texture_rectangle GL_TRUE
#define GLEW_ARB_texture_query_lod GL_TRUE
#define GLEW_ARB_multisample GL_FALSE
#define GLEW_ARB_multitexture GL_TRUE
#define GLEW_ARB_depth_texture GL_TRUE
#define GLEW_ARB_vertex_buffer_object GL_FALSE
#define GLEW_ARB_vertex_shader GL_FALSE
#define GLEW_ARB_vertex_program GL_FALSE
#define GLEW_ARB_shader_objects GL_FALSE
#define GLEW_ARB_shading_language_100 GL_FALSE
#define GLEW_ARB_fragment_shader GL_FALSE
#define GLEW_ARB_fragment_program GL_FALSE
#define GLEW_ARB_shadow GL_FALSE
#define GLEW_ARB_shadow_ambient GL_FALSE
#define GLEW_ARB_imaging GL_FALSE
#define GLEW_ARB_occlusion_query GL_FALSE
#define GLEW_ARB_geometry_shader4 GL_FALSE
#define GLEW_ARB_transform_feedback_instanced GL_FALSE
#define GLEW_ARB_uniform_buffer_object GL_FALSE
#define GLEW_ARB_transform_feedback3 GL_FALSE
#define GLEW_EXT_blend_equation_separate GL_FALSE
#define GLEW_EXT_blend_func_separate GL_FALSE
#define GLEW_ARB_framebuffer_object GL_FALSE

#define GLXEW_SGI_video_sync GL_FALSE

GLenum glewInit();

const GLubyte* glewGetString(GLenum name);

GLboolean glewIsSupported(const char* name);
GLboolean glewIsExtensionSupported(const char* name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _GLEW_STUB_H_

