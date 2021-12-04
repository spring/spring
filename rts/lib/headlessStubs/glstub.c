/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "glstub.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GLAPI
# undef GLAPI
#endif
#define GLAPI

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef GL_TRUE
#define GL_TRUE 1
#define GL_FALSE 0
#endif

// from gl.h & glext.h
GLAPI void APIENTRY glClientActiveTextureARB(GLenum texture) {}
GLAPI void APIENTRY glClientActiveTexture(GLenum texture) {}
GLAPI void APIENTRY glActiveTexture(GLenum texture) {}
GLAPI void APIENTRY glActiveTextureARB(GLenum texture) {}

GLAPI void APIENTRY glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers) {}
GLAPI void APIENTRY glBindFramebuffer(GLenum target, GLuint framebuffer) {}
GLAPI void APIENTRY glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
GLAPI void APIENTRY glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {}

GLAPI void APIENTRY glDeleteFramebuffersEXT(GLsizei n, const GLuint *framebuffers) {}
GLAPI void APIENTRY glBindFramebufferEXT(GLenum target, GLuint framebuffer) {}
GLAPI void APIENTRY glFramebufferTexture2DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
GLAPI void APIENTRY glFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {}

GLAPI void APIENTRY glDrawBuffers(GLsizei n, const GLenum *bufs) {}

GLAPI void APIENTRY glDrawBuffersARB(GLsizei n, const GLenum *bufs) {}
GLAPI void APIENTRY glDeleteBuffersARB(GLsizei n, const GLuint *buffers) {}
GLAPI void APIENTRY glGenBuffersARB(GLsizei n, GLuint *buffers) {}
GLAPI void APIENTRY glBindBufferARB(GLenum target, GLuint buffer) {}
GLAPI void APIENTRY glBufferDataARB(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage) {}
GLAPI void APIENTRY glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {}
GLAPI void APIENTRY glCopyNamedBufferSubData(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizei size) {}

GLAPI void APIENTRY glGenVertexArrays(GLsizei n, GLuint* arrays) {}
GLAPI void APIENTRY glDeleteVertexArrays(GLsizei n, const GLuint* arrays) {}
GLAPI void APIENTRY glBindVertexArray(GLuint id) {}

GLAPI void APIENTRY glEnableVertexAttribArray(GLuint index) {}
GLAPI void APIENTRY glDisableVertexAttribArray(GLuint index) {}
GLAPI void APIENTRY glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {}
GLAPI void APIENTRY glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) {}

GLAPI void APIENTRY glGenFramebuffers(GLsizei n, GLuint *framebuffers) {}
GLAPI GLenum APIENTRY glCheckFramebufferStatus(GLenum target) { return 0; }
GLAPI void APIENTRY glBlitFramebuffer (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {}

GLAPI void APIENTRY glGenFramebuffersEXT(GLsizei n, GLuint *framebuffers) {}
GLAPI GLenum APIENTRY glCheckFramebufferStatusEXT(GLenum target) { return 0; }
GLAPI void APIENTRY glBlitFramebufferEXT (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {}

GLAPI void APIENTRY glUseProgram(GLuint program) {}
GLAPI GLuint APIENTRY glCreateProgram() { return 0; }
//glCreateProgram = (PFNGLCREATEPROGRAMPROC) NULL;
GLAPI void APIENTRY glDeleteProgram(GLuint program) {}
GLAPI void APIENTRY glProgramParameteri(GLuint program, GLenum pname, GLint value) {}
GLAPI void APIENTRY glLinkProgram(GLuint program) {}
GLAPI void APIENTRY glGetProgramiv(GLuint program, GLenum pname, GLint *params) {}
GLAPI void APIENTRY glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {}

GLAPI void APIENTRY glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t) {}
GLAPI void APIENTRY glPointParameterf(GLenum pname, GLfloat param) {}
GLAPI void APIENTRY glPointParameterfv(GLenum pname, const GLfloat *params) {}
GLAPI void APIENTRY glMultiTexCoord3f(GLenum target, GLfloat s, GLfloat t, GLfloat r) {}
GLAPI void APIENTRY glFogCoordf(GLfloat coord) {}
GLAPI void APIENTRY glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {}
GLAPI void APIENTRY glBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {}
GLAPI void APIENTRY glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {}
GLAPI void APIENTRY glStencilFuncSeparate(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask) {}
GLAPI void APIENTRY glStencilMaskSeparate(GLenum face, GLuint mask) {}

GLAPI void APIENTRY glGenQueries(GLsizei n, GLuint *ids) {}
GLAPI void APIENTRY glDeleteQueries(GLsizei n, const GLuint *ids) {}
GLAPI void APIENTRY glBeginQuery(GLenum target, GLuint id) {}
GLAPI void APIENTRY glEndQuery(GLenum target) {}

GLAPI void APIENTRY glQueryCounter(GLuint id, GLenum target) {}
GLAPI void APIENTRY glGetQueryObjectiv(GLuint id, GLenum pname, GLint* params) {}
GLAPI void APIENTRY glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params) {}
GLAPI void APIENTRY glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params) {}

GLAPI GLint APIENTRY glGetUniformLocation(GLuint program, const GLchar *name) { return 0; }
GLAPI void APIENTRY glUniform1f(GLint location, GLfloat v0) {}
GLAPI void APIENTRY glUniform2f(GLint location, GLfloat v0, GLfloat v1) {}
GLAPI void APIENTRY glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {}
GLAPI void APIENTRY glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {}
GLAPI void APIENTRY glUniform1i(GLint location, GLint v0) {}
GLAPI void APIENTRY glUniform2i(GLint location, GLint v0, GLint v1) {}
GLAPI void APIENTRY glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {}
GLAPI void APIENTRY glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {}
GLAPI void APIENTRY glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {}
GLAPI void APIENTRY glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {}
GLAPI void APIENTRY glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {}
GLAPI void APIENTRY glUniform1fv(GLint location, GLsizei count, const GLfloat *value) {}
GLAPI void APIENTRY glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {}
GLAPI void APIENTRY glUniform3fv(GLint location, GLsizei count, const GLfloat *value) {}
GLAPI void APIENTRY glUniform4fv(GLint location, GLsizei count, const GLfloat *value) {}
GLAPI void APIENTRY glUniform1iv(GLint location, GLsizei count, const GLint *value) {}
GLAPI void APIENTRY glUniform2iv(GLint location, GLsizei count, const GLint *value) {}
GLAPI void APIENTRY glUniform3iv(GLint location, GLsizei count, const GLint *value) {}
GLAPI void APIENTRY glUniform4iv(GLint location, GLsizei count, const GLint *value) {}

GLAPI void APIENTRY glBindRenderbuffer(GLenum target, GLuint renderbuffer) {}
GLAPI void APIENTRY glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) {}
GLAPI void APIENTRY glGenRenderbuffers(GLsizei n, GLuint *renderbuffers) {}
GLAPI void APIENTRY glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {}
GLAPI void APIENTRY glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {}
GLAPI GLboolean APIENTRY glIsRenderbuffer(GLuint renderbuffer) { return GL_FALSE; }

//Subroutines
GLAPI void APIENTRY glUniformSubroutinesuiv (GLenum shadertype, GLsizei count, const GLuint *indices) {}
GLAPI GLuint APIENTRY glGetSubroutineIndex (GLuint program, GLenum shadertype, const GLchar *name) { return -1; }

//Tesselation
GLAPI void APIENTRY glPatchParameteri (GLenum pname, GLint value) {};
GLAPI void APIENTRY glPatchParameterfv (GLenum pname, const GLfloat *values) {};

GLAPI void APIENTRY glBindRenderbufferEXT(GLenum target, GLuint renderbuffer) {}
GLAPI void APIENTRY glDeleteRenderbuffersEXT(GLsizei n, const GLuint *renderbuffers) {}
GLAPI void APIENTRY glGenRenderbuffersEXT(GLsizei n, GLuint *renderbuffers) {}
GLAPI void APIENTRY glRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {}
GLAPI void APIENTRY glRenderbufferStorageMultisampleEXT(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {}
GLAPI GLboolean APIENTRY glIsRenderbufferEXT(GLuint renderbuffer) { return GL_FALSE; }

GLAPI void APIENTRY glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {}
GLAPI void APIENTRY glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {}
GLAPI void APIENTRY glGetQueryiv(GLenum target, GLenum pname, GLint *params) {}
GLAPI void APIENTRY glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {}

GLAPI void APIENTRY glGenerateMipmap(GLenum target) {}
GLAPI void APIENTRY glGenerateMipmapEXT(GLenum target) {}

GLAPI void APIENTRY glGetUniformfv(GLuint program, GLint location, GLfloat *params) {}
GLAPI void APIENTRY glGetUniformiv(GLuint program, GLint location, GLint *params) {}
GLAPI void APIENTRY glGetUniformuiv(GLuint program, GLint location, GLuint *params) {}
GLAPI void APIENTRY glUniform1uiv(GLint location, GLsizei count, const GLuint *value) {}
GLAPI void APIENTRY glUniform2uiv(GLint location, GLsizei count, const GLuint *value) {}
GLAPI void APIENTRY glUniform3uiv(GLint location, GLsizei count, const GLuint *value) {}
GLAPI void APIENTRY glUniform4uiv(GLint location, GLsizei count, const GLuint *value) {}
GLAPI void APIENTRY glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {}
GLAPI GLint APIENTRY glGetAttribLocation(GLuint program, const GLchar *name) { return -1; }
GLAPI void APIENTRY glBindAttribLocation(GLuint program, GLuint index, const GLchar *name) {}

// true for headless asserts
GLAPI GLboolean APIENTRY glIsShader(GLuint shader) { return GL_TRUE; }
GLAPI GLboolean APIENTRY glIsProgram(GLuint shader) { return GL_TRUE; }

GLAPI void APIENTRY glDetachShader(GLuint program, GLuint shader) {}
GLAPI void APIENTRY glAttachShader(GLuint program, GLuint shader) {}
GLAPI void APIENTRY glDeleteShader(GLuint shader) {}
GLAPI GLuint APIENTRY glCreateShader(GLenum type) { return 0; }
//glCreateShader = (PFNGLCREATESHADERPROC) NULL;
GLAPI void APIENTRY glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source) {}
GLAPI void APIENTRY glCompileShader(GLuint shader) {}
GLAPI void APIENTRY glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {}
GLAPI void APIENTRY glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {}
#ifdef __APPLE__
// MacOS buildbot GL headers are ancient, need the extra const
GLAPI void APIENTRY glShaderSource(GLuint shader, GLsizei count, const GLchar* const *string, const GLint *length) {}
#else
GLAPI void APIENTRY glShaderSource(GLuint shader, GLsizei count, const GLchar* *string, const GLint *length) {}
#endif

GLAPI void APIENTRY glUniform1fARB(GLint location, GLfloat v0) {}
GLAPI void APIENTRY glUniform2fARB(GLint location, GLfloat v0, GLfloat v1) {}
GLAPI void APIENTRY glUniform3fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {}
GLAPI void APIENTRY glUniform4fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {}
GLAPI void APIENTRY glUniform1iARB(GLint location, GLint v0) {}
GLAPI void APIENTRY glUniform2iARB(GLint location, GLint v0, GLint v1) {}
GLAPI void APIENTRY glUniform3iARB(GLint location, GLint v0, GLint v1, GLint v2) {}
GLAPI void APIENTRY glUniform4iARB(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {}
GLAPI void APIENTRY glUniform1fvARB(GLint location, GLsizei count, const GLfloat *value) {}
GLAPI void APIENTRY glUniform2fvARB(GLint location, GLsizei count, const GLfloat *value) {}
GLAPI void APIENTRY glUniform3fvARB(GLint location, GLsizei count, const GLfloat *value) {}
GLAPI void APIENTRY glUniform4fvARB(GLint location, GLsizei count, const GLfloat *value) {}
GLAPI void APIENTRY glUniform1ivARB(GLint location, GLsizei count, const GLint *value) {}
GLAPI void APIENTRY glUniform2ivARB(GLint location, GLsizei count, const GLint *value) {}
GLAPI void APIENTRY glUniform3ivARB(GLint location, GLsizei count, const GLint *value) {}
GLAPI void APIENTRY glUniform4ivARB(GLint location, GLsizei count, const GLint *value) {}
GLAPI void APIENTRY glUniformMatrix2fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {}
GLAPI void APIENTRY glUniformMatrix3fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {}
GLAPI void APIENTRY glUniformMatrix4fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {}

GLAPI void APIENTRY glDeleteObjectARB(GLhandleARB obj) {}
GLAPI GLhandleARB APIENTRY glGetHandleARB(GLenum pname) { return 0; }
GLAPI void APIENTRY glDetachObjectARB(GLhandleARB containerObj, GLhandleARB attachedObj) {}
GLAPI GLhandleARB APIENTRY glCreateShaderObjectARB(GLenum shaderType) { return 0; }


GLAPI void APIENTRY glCompileShaderARB(GLhandleARB shaderObj) {}
GLAPI GLhandleARB APIENTRY glCreateProgramObjectARB() { return 0; }
GLAPI void APIENTRY glAttachObjectARB(GLhandleARB containerObj, GLhandleARB obj) {}

GLAPI void APIENTRY glLinkProgramARB(GLhandleARB programObj) {}
GLAPI void APIENTRY glUseProgramObjectARB(GLhandleARB programObj) {}
GLAPI void APIENTRY glValidateProgramARB(GLhandleARB programObj) {}

GLAPI void APIENTRY glGetObjectParameterfvARB(GLhandleARB obj, GLenum pname, GLfloat *params) {}
GLAPI void APIENTRY glGetObjectParameterivARB(GLhandleARB obj, GLenum pname, GLint *params) {}
GLAPI void APIENTRY glGetInfoLogARB(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog) {}
GLAPI void APIENTRY glGetAttachedObjectsARB(GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj) {}

GLAPI GLint APIENTRY glGetUniformLocationARB(GLhandleARB programObj, const GLcharARB *name) { return 0; }

GLAPI void APIENTRY glBindAttribLocationARB(GLhandleARB programObj, GLuint index, const GLcharARB *name) {}
GLAPI void APIENTRY glGetActiveAttribARB(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name) {}
GLAPI GLint APIENTRY glGetAttribLocationARB(GLhandleARB programObj, const GLcharARB *name) { return 0; }

GLAPI void APIENTRY glVertexAttribPointerARB(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer) {}
GLAPI void APIENTRY glEnableVertexAttribArrayARB(GLuint index) {}
GLAPI void APIENTRY glDisableVertexAttribArrayARB(GLuint index) {}

GLAPI void APIENTRY glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data) {}
GLAPI void APIENTRY glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data) {}
GLAPI void APIENTRY glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data) {}
GLAPI void APIENTRY glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data) {}
GLAPI void APIENTRY glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data) {}
GLAPI void APIENTRY glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data) {}
GLAPI void APIENTRY glGetCompressedTexImage(GLenum target, GLint level, GLvoid *img) {}

GLAPI void APIENTRY glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {}
GLAPI void APIENTRY glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {}

GLAPI void APIENTRY glProgramStringARB(GLenum target, GLenum format, GLsizei len, const GLvoid *string) {}
GLAPI void APIENTRY glBindProgramARB(GLenum target, GLuint program) {}
GLAPI void APIENTRY glDeleteProgramsARB(GLsizei n, const GLuint *programs) {}
GLAPI void APIENTRY glGenProgramsARB(GLsizei n, GLuint *programs) {}
//glGenProgramsARB = (PFNGLGENPROGRAMSARBPROC) NULL;

GLAPI void APIENTRY glProgramEnvParameter4dARB(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {}
GLAPI void APIENTRY glProgramEnvParameter4dvARB(GLenum target, GLuint index, const GLdouble *params) {}
GLAPI void APIENTRY glProgramEnvParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {}
GLAPI void APIENTRY glProgramEnvParameter4fvARB(GLenum target, GLuint index, const GLfloat *params) {}

GLAPI void APIENTRY glProgramLocalParameter4dARB(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {}
GLAPI void APIENTRY glProgramLocalParameter4dvARB(GLenum target, GLuint index, const GLdouble *params) {}
GLAPI void APIENTRY glProgramLocalParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {}
GLAPI void APIENTRY glProgramLocalParameter4fvARB(GLenum target, GLuint index, const GLfloat *params) {}

GLAPI void APIENTRY glGetProgramEnvParameterdvARB(GLenum target, GLuint index, GLdouble *params) {}
GLAPI void APIENTRY glGetProgramEnvParameterfvARB(GLenum target, GLuint index, GLfloat *params) {}
GLAPI void APIENTRY glGetProgramLocalParameterdvARB(GLenum target, GLuint index, GLdouble *params) {}
GLAPI void APIENTRY glGetProgramLocalParameterfvARB(GLenum target, GLuint index, GLfloat *params) {}

GLAPI void APIENTRY glMultiTexCoord2iARB(GLenum target, GLint s, GLint t) {}
GLAPI void APIENTRY glMultiTexCoord2ivARB(GLenum target, const GLint *v) {}

GLAPI void APIENTRY glBindBuffer(GLenum target, GLuint buffer) {}
GLAPI void APIENTRY glDeleteBuffers(GLsizei n, const GLuint *buffers) {}
GLAPI void APIENTRY glGenBuffers(GLsizei n, GLuint *buffers) {}
GLAPI void APIENTRY glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) {}
GLAPI void APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {}

GLAPI GLvoid* APIENTRY glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) { return (GLvoid*) 0; }
GLAPI void APIENTRY glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) {}
GLAPI GLvoid* APIENTRY glMapBuffer(GLenum target, GLenum access) { return (GLvoid*) 0; }
GLAPI GLboolean APIENTRY glUnmapBuffer(GLenum target) { return GL_FALSE; }

GLAPI void APIENTRY glMultiTexCoord2i(GLenum target, GLint s, GLint t) {}
GLAPI void APIENTRY glMultiTexCoord2iv(GLenum target, const GLint *v) {}

GLAPI void APIENTRY glDeleteFencesNV(GLsizei n, const GLuint *fences) {}
GLAPI void APIENTRY glGenFencesNV(GLsizei n, GLuint *fences) {}
GLAPI GLboolean APIENTRY glIsFenceNV(GLuint fence) { return GL_FALSE; }
GLAPI GLboolean APIENTRY glTestFenceNV(GLuint fence) { return GL_FALSE; }
GLAPI void APIENTRY glGetFenceivNV(GLuint fence, GLenum pname, GLint *params) {}
GLAPI void APIENTRY glFinishFenceNV(GLuint fence) {}
GLAPI void APIENTRY glSetFenceNV(GLuint fence, GLenum condition) {}

GLAPI void APIENTRY glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params) {}
GLAPI void APIENTRY glGetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint *params) {}
GLAPI GLboolean APIENTRY glIsFramebuffer(GLuint framebuffer) { return GL_FALSE; }
GLAPI GLboolean APIENTRY glIsFramebufferEXT(GLuint framebuffer) { return GL_FALSE; }
GLAPI void APIENTRY glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level) {}
GLAPI void APIENTRY glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
GLAPI void APIENTRY glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) {}
GLAPI void APIENTRY glFramebufferTextureEXT(GLenum target, GLenum attachment, GLuint texture, GLint level) {}
GLAPI void APIENTRY glFramebufferTexture1DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
GLAPI void APIENTRY glFramebufferTexture3DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) {}
GLAPI void APIENTRY glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint *params) {}
GLAPI void APIENTRY glGetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment, GLenum pname, GLint *params) {}

GLAPI void APIENTRY glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices) {}
GLAPI void APIENTRY glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {}
GLAPI void APIENTRY glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels) {}
GLAPI void APIENTRY glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {}

GLAPI void APIENTRY glGetProgramivARB(GLenum target, GLenum pname, GLint *params) {}
GLAPI void APIENTRY glValidateProgram(GLuint program) {}

GLAPI void APIENTRY glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {}
GLAPI void APIENTRY glEnable(GLenum i) {}
GLAPI void APIENTRY glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {}
GLAPI void APIENTRY glColor3fv(const GLfloat *v) {}
GLAPI void APIENTRY glColor3ub(GLubyte red, GLubyte green, GLubyte blue) {}
GLAPI void APIENTRY glColor3ubv(const GLubyte *v) {}
GLAPI void APIENTRY glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {}
GLAPI void APIENTRY glColor4ubv(const GLubyte *v) {}

GLAPI void APIENTRY glCopyTexImage2D(GLenum target, GLint level,
                                        GLenum internalformat,
                                        GLint x, GLint y,
                                        GLsizei width, GLsizei height,
                                        GLint border) {}

GLAPI void APIENTRY glCopyTexSubImage2D(GLenum target, GLint level,
                                           GLint xoffset, GLint yoffset,
                                           GLint x, GLint y,
                                           GLsizei width, GLsizei height) {}

GLAPI void APIENTRY glDrawBuffer(GLenum mode) {}
GLAPI void APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {}
GLAPI void APIENTRY glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount) {}
GLAPI void APIENTRY glEdgeFlag(GLboolean flag) {}
GLAPI void APIENTRY glEvalCoord1f(GLfloat u) {}
GLAPI void APIENTRY glEvalCoord2f(GLfloat u, GLfloat v) {}
GLAPI void APIENTRY glEvalMesh1(GLenum mode, GLint i1, GLint i2) {}
GLAPI void APIENTRY glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) {}
GLAPI void APIENTRY glEvalPoint1(GLint i) {}
GLAPI void APIENTRY glEvalPoint2(GLint i, GLint j) {}
GLAPI void APIENTRY glFinish() {}
GLAPI void APIENTRY glFlush() {}
GLAPI void APIENTRY glFrontFace(GLenum mode) {}

GLAPI void APIENTRY glFrustum(GLdouble left, GLdouble right,
                                   GLdouble bottom, GLdouble top,
                                   GLdouble near_val, GLdouble far_val) {}

GLAPI void APIENTRY glGetFloatv(GLenum pname, GLfloat *params) {
   *params = 0;
}

GLAPI void APIENTRY glGetTexImage( GLenum target, GLint level,
                                     GLenum format, GLenum type,
                                     GLvoid *pixels ) {}

GLAPI void APIENTRY glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params) {}

GLAPI void APIENTRY glInitNames() {}

GLAPI GLboolean APIENTRY glIsEnabled (GLenum cap) { return GL_FALSE; }
GLAPI GLboolean APIENTRY glIsTexture(GLuint texture) { return GL_FALSE; }

GLAPI void APIENTRY glLightf(GLenum light, GLenum pname, GLfloat param) {}
GLAPI void APIENTRY glLightModelfv(GLenum pname, const GLfloat *params) {}
GLAPI void APIENTRY glLoadMatrixf(const GLfloat *m) {}
GLAPI void APIENTRY glLoadName(GLuint name) {}

GLAPI void APIENTRY glMap1f( GLenum target, GLfloat u1, GLfloat u2,
                               GLint stride,
                               GLint order, const GLfloat *points ) {}

GLAPI void APIENTRY glMap2f( GLenum target,
		     GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
		     GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
		     const GLfloat *points ) {}

GLAPI void APIENTRY glMapGrid1f(GLint un, GLfloat u1, GLfloat u2) {}
GLAPI void APIENTRY glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) {}

GLAPI void APIENTRY glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) {}
GLAPI void APIENTRY glNormal3fv(const GLfloat *v) {}

GLAPI void APIENTRY glPixelStorei(GLenum pname, GLint param) {}
GLAPI void APIENTRY glPushName(GLuint name) {}
GLAPI void APIENTRY glPopName() {}
GLAPI void APIENTRY glReadBuffer(GLenum mode) {}

GLAPI void APIENTRY glReadPixels(GLint x, GLint y,
                                    GLsizei width, GLsizei height,
                                    GLenum format, GLenum type,
                                    GLvoid *pixels ) {}

GLAPI void APIENTRY glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {}
GLAPI GLint APIENTRY glRenderMode(GLenum mode) { return 0; }

GLAPI void APIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {}
GLAPI void APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z) {}
GLAPI void APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {}
GLAPI void APIENTRY glSelectBuffer(GLsizei size, GLuint *buffer) {}
GLAPI void APIENTRY glTexCoord1f(GLfloat s) {}
GLAPI void APIENTRY glTexCoord2fv(const GLfloat *v) {}
GLAPI void APIENTRY glTexCoord3f(GLfloat s, GLfloat t, GLfloat r) {}
GLAPI void APIENTRY glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q) {}
GLAPI void APIENTRY glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params) {}
GLAPI void APIENTRY glTexGenf(GLenum coord, GLenum pname, GLfloat param) {}

GLAPI void APIENTRY glTexImage1D( GLenum target, GLint level,
                                    GLint internalFormat,
                                    GLsizei width, GLint border,
                                    GLenum format, GLenum type,
                                    const GLvoid *pixels) {
}

GLAPI void APIENTRY glVertex3fv( const GLfloat *v ) {}
GLAPI void APIENTRY glVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {}
GLAPI void APIENTRY glClipPlane( GLenum plane, const GLdouble *equation ) {}
GLAPI void APIENTRY glMatrixMode(GLenum mode) {}

GLAPI void APIENTRY  glGetBooleanv( GLenum pname, GLboolean *params ) { *params = 0; }

GLAPI void APIENTRY glLoadMatrixd(const GLdouble *m) {}
GLAPI void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {}
GLAPI void APIENTRY glNewList(GLuint list, GLenum mode) {}
GLAPI void APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask ) {}
GLAPI void APIENTRY glStencilMask(GLuint mask ) {}
GLAPI void APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass ) {}
GLAPI void APIENTRY glBlendEquation(GLenum mode) {}
GLAPI void APIENTRY glEnableClientState(GLenum cap) {}
GLAPI void APIENTRY glDisableClientState(GLenum cap) {}
GLAPI void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr) {}
GLAPI void APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr) {}
GLAPI void APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr) {}
GLAPI void APIENTRY glNormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr) {}
GLAPI void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count) {}
GLAPI void APIENTRY glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount) {}

GLAPI void APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param) {}

GLAPI void APIENTRY glMultMatrixd(const GLdouble *m) {}
GLAPI void APIENTRY glMultMatrixf(const GLfloat *m) {}
GLAPI void APIENTRY glTexGeni(GLenum coord, GLenum pname, GLint param) {}
GLAPI void APIENTRY glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params) {}
GLAPI void APIENTRY glLightModeli(GLenum pname, GLint param) {}
GLAPI void APIENTRY glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {}
GLAPI void APIENTRY glMaterialf(GLenum face, GLenum pname, GLfloat param) {}
GLAPI void APIENTRY glPointSize(GLfloat size) {}
GLAPI void APIENTRY glCullFace(GLenum mode) {}
GLAPI void APIENTRY glLogicOp(GLenum opcode) {}
GLAPI void APIENTRY glEndList() {}
GLAPI GLuint APIENTRY glGenLists( GLsizei range ) { return 0; }

GLAPI void APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {}
GLAPI void APIENTRY glLoadIdentity() {}
GLAPI void APIENTRY glOrtho(GLdouble left, GLdouble right,
                                 GLdouble bottom, GLdouble top,
                                 GLdouble near_val, GLdouble far_val) {}

GLAPI void APIENTRY glGenTextures(GLsizei n, GLuint *textures) {}

GLAPI void APIENTRY glBindTexture(GLenum target, GLuint texture) {}

GLAPI void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param) {}
GLAPI void APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param) {}
GLAPI void APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {}
GLAPI void APIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint* params) {}

GLAPI void APIENTRY glTexImage2D(GLenum target, GLint level,
                                    GLint internalFormat,
                                    GLsizei width, GLsizei height,
                                    GLint border, GLenum format, GLenum type,
                                    const GLvoid *pixels) {
  // printf( "glTexImage2D\n" );
}

GLAPI void APIENTRY glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {}

GLAPI void APIENTRY glClear(GLbitfield mask) {}
GLAPI void APIENTRY glTexCoord2i(GLint s, GLint t ){}
GLAPI void APIENTRY glVertex2f(GLfloat x, GLfloat y ) {}
GLAPI void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z){}
GLAPI void APIENTRY glBegin(GLenum mode) {}
GLAPI void APIENTRY glEnd() {}

GLAPI void APIENTRY glDeleteTextures(GLsizei n, const GLuint *textures) {}

GLAPI void APIENTRY glGetIntegerv(GLenum pname, GLint *params) {}
GLAPI void APIENTRY glDepthFunc(GLenum func) {}
GLAPI void APIENTRY glShadeModel(GLenum mode) {}

GLAPI void APIENTRY glHint(GLenum target, GLenum mode) {}
GLAPI void APIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param) {}

GLAPI const GLubyte * APIENTRY glGetString(GLenum name) { return (const GLubyte*) ""; }

GLAPI void APIENTRY glClearStencil(GLint s) {}
GLAPI void APIENTRY glLightfv(GLenum light, GLenum pname, const GLfloat *params ) {}

GLAPI void APIENTRY glDeleteLists( GLuint list, GLsizei range ) {}
GLAPI void APIENTRY glDisable(GLenum i) {}
GLAPI void APIENTRY glClearDepth(GLclampd depth) {}
GLAPI void APIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {}
GLAPI void APIENTRY glSecondaryColor3f(GLfloat red, GLfloat green, GLfloat blue) {}

GLAPI void APIENTRY glMultiTexCoord1f( GLenum target, GLfloat s ) {}
GLAPI void APIENTRY glMultiTexCoord2f( GLenum target, GLfloat s, GLfloat t ) {}
GLAPI void APIENTRY glMultiTexCoord4f( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {}

GLAPI GLenum APIENTRY glGetError( void ) { return 0; }

GLAPI void APIENTRY glPolygonMode(GLenum face, GLenum mode) {}
GLAPI void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor ) {}
GLAPI void APIENTRY glTranslatef( GLfloat x, GLfloat y, GLfloat z ) {}
GLAPI void APIENTRY glColor4fv( const GLfloat *v) {}
GLAPI void APIENTRY glLineStipple(GLint factor, GLushort pattern) {}
GLAPI void APIENTRY glPopAttrib() {}
GLAPI void APIENTRY glPushAttrib(GLbitfield mask) {}
GLAPI void APIENTRY glDepthMask(GLboolean flag) {}
GLAPI void APIENTRY glAlphaFunc(GLenum func, GLclampf ref) {}

GLAPI void APIENTRY glFogfv(GLenum pname, const GLfloat *params) {}
GLAPI void APIENTRY glFogf( GLenum pname, GLfloat param ) {}
GLAPI void APIENTRY glFogi( GLenum pname, GLint param ) {}
GLAPI void APIENTRY glPushMatrix() {}
GLAPI void APIENTRY glPopMatrix() {}
GLAPI void APIENTRY glCallList( GLuint list) {}

GLAPI void APIENTRY glTexSubImage2D(GLenum target, GLint level,
                                       GLint xoffset, GLint yoffset,
                                       GLsizei width, GLsizei height,
                                       GLenum format, GLenum type,
                                       const GLvoid *pixels ) {}

GLAPI void APIENTRY glTexCoord2f(GLfloat s, GLfloat t ) {}
GLAPI void APIENTRY glLineWidth(GLfloat width) {}
GLAPI void APIENTRY glColor3f(GLfloat red, GLfloat green, GLfloat blue ) {}
GLAPI void APIENTRY glPolygonOffset(GLfloat factor, GLfloat units) {}

GLAPI void APIENTRY glBeginConditionalRender(GLuint id, GLenum mode) {}
GLAPI void APIENTRY glBeginConditionalRenderNV(GLuint id, GLenum mode) {}
GLAPI void APIENTRY glEndConditionalRender() {}
GLAPI void APIENTRY glEndConditionalRenderNV() {}

GLAPI void APIENTRY glPrimitiveRestartIndex(GLuint index) {}
GLAPI void APIENTRY glPrimitiveRestartIndexNV(GLuint index) {}

GLAPI void APIENTRY glDepthRangef(GLfloat nearVal, GLfloat farVal) {}
GLAPI void APIENTRY glDepthRange(GLdouble nearVal, GLdouble farVal) {}

#ifdef __cplusplus
} // extern "C"
#endif

