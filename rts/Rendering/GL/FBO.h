/**
 * @file FBO.h
 * @brief EXT_framebuffer_object
 *
 * EXT_framebuffer_object class definition
 * Copyright (C) 2008.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 *
 */
#ifndef FBO_H
#define FBO_H

#include <map>

#include "myGL.h"

//todo: add multisample buffers

/**
 * @brief FBO
 *
 * Framebuffer Object class.
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
	 * @brief IsValid
	 * @return whether a valid framebuffer exists
	 */
	bool IsValid();

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
	 * @brief Unattach
	 * @param attachment
	 */
	void Unattach(const GLenum attachment);

	/**
	 * @brief UnattachAll
	 */
	void UnattachAll();

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
	/**
	 * @brief rbos
	 *
	 * List with all Renderbuffer Objects that should be destructed with the FBO
	 */
	std::vector<GLuint> myRBOs;


	static std::vector<FBO*> fboList;
	struct TexData{
		GLuint id;
		unsigned char* pixels;
		GLsizei xsize,ysize,zsize;
		GLenum target,format,type;
	};
	static std::map<GLuint,TexData*> texBuf;

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
