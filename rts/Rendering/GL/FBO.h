/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FBO_H
#define FBO_H

#include <map>
#include <vector>

#include "myGL.h"

// TODO: add multisample buffers

/**
 * @brief FBO
 *
 * Framebuffer Object class (EXT_framebuffer_object).
 */
class FBO
{
public:
	/**
	 * @brief IsSupported
	 *
	 * if FrameBuffers are supported by the current platform
	 */
	static bool IsSupported();

	/**
	 * @brief Constructor
	 */
	FBO();

	/**
	 * @brief Destructor
	 */
	~FBO();

	/**
	 * @brief fboId
	 *
	 * GLuint pointing to the current framebuffer
	 */
	GLuint fboId;

	/**
	 * @brief reloadOnAltTab
	 *
	 * bool save all attachments in system RAM and reloaded them on OpenGL-Context lost (alt-tab) (default: false)
	 */
	bool reloadOnAltTab;

	/**
	 * @brief check FBO status
	 */
	bool CheckStatus(std::string name);

	/**
	 * @brief get FBO status
	 */
	GLenum GetStatus();

	/**
	 * @return GL_MAX_SAMPLES or 0 if multi-sampling not supported
	 */
	static GLsizei GetMaxSamples();

	/**
	 * @brief IsValid
	 * @return whether a valid framebuffer exists
	 */
	bool IsValid() const;

	/**
	 * @brief AttachTexture
	 * @param texTarget texture target (GL_TEXTURE_2D etc.)
	 * @param texId texture to attach
	 * @param attachment (GL_COLOR_ATTACHMENT0_EXT etc.)
	 * @param mipLevel miplevel to attach
	 * @param zSlice z offset (3d textures only)
	 */
	void AttachTexture(const GLuint texId, const GLenum texTarget = GL_TEXTURE_2D, const GLenum attachment = GL_COLOR_ATTACHMENT0_EXT, const int mipLevel = 0, const int zSlice = 0);

	/**
	 * @brief AttachRenderBuffer
	 * @param rboId RenderBuffer to attach
	 * @param attachment
	 */
	void AttachRenderBuffer(const GLuint rboId, const GLenum attachment = GL_COLOR_ATTACHMENT0_EXT);

	/**
	 * @brief Creates a RenderBufferObject and attachs it to the FBO (it is also auto destructed)
	 * @param attachment
	 * @param format
	 * @param width
	 * @param height
	 */
	void CreateRenderBuffer(const GLenum attachment, const GLenum format, const GLsizei width, const GLsizei height);

	/**
	 * @brief Creates a multisampled RenderBufferObject and attachs it to the FBO (it is also auto destructed)
	 * @param attachment
	 * @param format
	 * @param width
	 * @param height
	 * @param samples
	 */
	void CreateRenderBufferMultisample(const GLenum attachment, const GLenum format, const GLsizei width, const GLsizei height, GLsizei samples);

	/**
	 * @brief Detach
	 * @param attachment
	 */
	void Detach(const GLenum attachment);

	/**
	 * @brief DetachAll
	 */
	void DetachAll();

	/**
	 * @brief Bind
	 */
	void Bind();

	/**
	 * @brief Unbind
	 */
	static void Unbind();



	/**
	 * @brief GLContextLost (post atl-tab)
	 */
	static void GLContextLost();

	/**
	 * @brief GLContextReinit (pre atl-tab)
	 */
	static void GLContextReinit();


private:
	bool valid;

	/**
	 * @brief rbos
	 *
	 * List with all Renderbuffer Objects that should be destructed with the FBO
	 */
	std::vector<GLuint> myRBOs;


	struct TexData {
		GLuint id;
		unsigned char* pixels;
		GLsizei xsize,ysize,zsize;
		GLenum target,format,type;
	};

	static std::vector<FBO*> fboList;
	static std::map<GLuint, TexData*> texBuf;

	static GLint maxAttachments;
	static GLsizei maxSamples;

	/**
	 * @brief DownloadAttachment
	 *
	 * copies the attachment content to sysram
	 */
	static void DownloadAttachment(const GLenum attachment);

	/**
	 * @brief GetTextureTargetByID
	 *
	 * detects the textureTarget just by the textureName/ID
	 */
	static GLenum GetTextureTargetByID(const GLuint id, const unsigned int i = 0);
};

#endif /* FBO_H */
