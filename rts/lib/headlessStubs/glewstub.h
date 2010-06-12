/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLEW_STUB_H_
#define _GLEW_STUB_H_

#undef GL_GLEXT_LEGACY
#define GL_GLEXT_PROTOTYPES
#include <GL/glu.h>
#include <GL/glext.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GLEW_GET_FUN(x) x

#define GLEW_VERSION 1
#define GLEW_VERSION_1_4 GL_TRUE
#define GLEW_VERSION_2_0 GL_FALSE

#define GLEW_NV_vertex_program2 GL_FALSE
#define GLEW_NV_depth_clamp GL_FALSE
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

#define GLXEW_SGI_video_sync GL_FALSE

GLenum glewInit();

const GLubyte* glewGetString(GLenum name);

GLboolean glewIsSupported (const char* name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _GLEW_STUB_H_

