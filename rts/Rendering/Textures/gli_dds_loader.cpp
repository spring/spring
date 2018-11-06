#ifndef BITMAP_NO_OPENGL
#include <cassert>

#include <lib/gli/inc/gl.hpp>
#include <lib/gli/inc/load_dds.hpp>
#include <lib/gli/inc/texture.hpp>

#include "gli_dds_loader.hpp"
#include "Rendering/GL/myGL.h"
#include "System/FileSystem/FileHandler.h"


static void handle_dds_mipmaps(GLenum target, int num_mipmaps, bool gen_mipmaps)
{
	if (num_mipmaps > 0) {
		// use included mipmaps
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		return;
	}

	if (gen_mipmaps) {
		// create mipmaps at runtime
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glGenerateMipmap(target);
		return;
	}

	// no mipmaps
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}


gli::texture spring::load_dds_image(const char* filename)
{
	CFileHandler file(filename);
	std::vector<uint8_t> buffer;

	if (!file.FileExists())
		return {};

	if (!file.IsBuffered()) {
		buffer.resize(file.FileSize(), 0);
		file.Read(buffer.data(), file.FileSize());
	} else {
		// steal if file was loaded from VFS
		buffer = std::move(file.GetBuffer());
	}

	return (gli::load_dds(reinterpret_cast<const char*>(buffer.data()), buffer.size()));
}

unsigned int spring::create_dds_texture(
	const gli::texture& dds_tex,
	unsigned int tex_id,
	float anisoLvl,
	float lodBias,
	bool mipmaps
) {
	if (dds_tex.empty())
		return 0;

	const gli::gl gl_trans = {gli::gl::PROFILE_GL33};
	const gli::gl::format gl_format = gl_trans.translate(dds_tex.format(), dds_tex.swizzles());

	GLenum gl_target = gl_trans.translate(dds_tex.target());

	if (tex_id == 0)
		glGenTextures(1, &tex_id);

	glBindTexture(gl_target, tex_id);
	glTexParameteri(gl_target, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(gl_target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(dds_tex.levels() - 1));
	glTexParameteri(gl_target, GL_TEXTURE_SWIZZLE_R, gl_format.Swizzles[0]);
	glTexParameteri(gl_target, GL_TEXTURE_SWIZZLE_G, gl_format.Swizzles[1]);
	glTexParameteri(gl_target, GL_TEXTURE_SWIZZLE_B, gl_format.Swizzles[2]);
	glTexParameteri(gl_target, GL_TEXTURE_SWIZZLE_A, gl_format.Swizzles[3]);

	if (lodBias != 0.0f)
		glTexParameterf(gl_target, GL_TEXTURE_LOD_BIAS, lodBias);
	if (anisoLvl > 0.0f)
		glTexParameterf(gl_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisoLvl);


	const glm::tvec3<GLsizei> tex_size = dds_tex.extent();
	const GLsizei numFaces = static_cast<GLsizei>(dds_tex.layers() * dds_tex.faces());

	switch (dds_tex.target()) {
		case gli::TARGET_1D: {
			glTexStorage1D(gl_target, static_cast<GLint>(dds_tex.levels()), gl_format.Internal, tex_size.x);
		} break;

		case gli::TARGET_1D_ARRAY:
		case gli::TARGET_2D:
		case gli::TARGET_CUBE: {
			glTexStorage2D(
				gl_target,
				static_cast<GLint>(dds_tex.levels()),
				gl_format.Internal,
				tex_size.x,
				(dds_tex.target() == gli::TARGET_2D)? tex_size.y : numFaces
			);
		} break;

		case gli::TARGET_2D_ARRAY:
		case gli::TARGET_3D:
		case gli::TARGET_CUBE_ARRAY: {
			glTexStorage3D(
				gl_target,
				static_cast<GLint>(dds_tex.levels()),
				gl_format.Internal,
				tex_size.x,
				tex_size.y,
				(dds_tex.target() == gli::TARGET_3D)? tex_size.z : numFaces
			);
		} break;

		default: {
			assert(false);
		} break;
	}


	for (size_t tex_layer = 0; tex_layer < dds_tex.layers(); ++tex_layer) {
		for (size_t tex_face = 0; tex_face < dds_tex.faces(); ++tex_face) {
			for (size_t tex_level = 0; tex_level < dds_tex.levels(); ++tex_level) {
				const GLsizei gl_tex_layer = static_cast<GLsizei>(tex_layer);
				const glm::tvec3<GLsizei> tex_level_size = dds_tex.extent(tex_level);

				if (gli::is_target_cube(dds_tex.target()))
					gl_target = static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + tex_face);

				switch (dds_tex.target()) {
					case gli::TARGET_1D: {
						if (gli::is_compressed(dds_tex.format())) {
							glCompressedTexSubImage1D(
								gl_target,
								static_cast<GLint>(tex_level),
								0,
								tex_level_size.x,
								gl_format.Internal,
								static_cast<GLsizei>(dds_tex.size(tex_level)),
								dds_tex.data(tex_layer, tex_face, tex_level)
							);
						} else {
							glTexSubImage1D(
								gl_target,
								static_cast<GLint>(tex_level),
								0,
								tex_level_size.x,
								gl_format.External,
								gl_format.Type,
								dds_tex.data(tex_layer, tex_face, tex_level)
							);
						}
					} break;

					case gli::TARGET_1D_ARRAY:
					case gli::TARGET_2D:
					case gli::TARGET_CUBE: {
						if (gli::is_compressed(dds_tex.format())) {
							glCompressedTexSubImage2D(
								gl_target,
								static_cast<GLint>(tex_level),
								0, 0,
								tex_level_size.x,
								(dds_tex.target() == gli::TARGET_1D_ARRAY)? gl_tex_layer : tex_level_size.y,
								gl_format.Internal,
								static_cast<GLsizei>(dds_tex.size(tex_level)),
								dds_tex.data(tex_layer, tex_face, tex_level)
							);
						} else {
							glTexSubImage2D(
								gl_target,
								static_cast<GLint>(tex_level),
								0, 0,
								tex_level_size.x,
								(dds_tex.target() == gli::TARGET_1D_ARRAY)? gl_tex_layer : tex_level_size.y,
								gl_format.External,
								gl_format.Type,
								dds_tex.data(tex_layer, tex_face, tex_level)
							);
						}
					} break;

					case gli::TARGET_2D_ARRAY:
					case gli::TARGET_3D:
					case gli::TARGET_CUBE_ARRAY: {
						if (gli::is_compressed(dds_tex.format())) {
							glCompressedTexSubImage3D(
								gl_target,
								static_cast<GLint>(tex_level),
								0, 0, 0,
								tex_level_size.x,
								tex_level_size.y,
								(dds_tex.target() == gli::TARGET_3D)? tex_level_size.z : gl_tex_layer,
								gl_format.Internal,
								static_cast<GLsizei>(dds_tex.size(tex_level)),
								dds_tex.data(tex_layer, tex_face, tex_level)
							);
						} else {
							glTexSubImage3D(
								gl_target,
								static_cast<GLint>(tex_level),
								0, 0, 0,
								tex_level_size.x,
								tex_level_size.y,
								(dds_tex.target() == gli::TARGET_3D)? tex_level_size.z : gl_tex_layer,
								gl_format.External,
								gl_format.Type,
								dds_tex.data(tex_layer, tex_face, tex_level)
							);
						}
					} break;

					default: {
						assert(false);
					} break;
				}
			}
		}
	}

	handle_dds_mipmaps(gl_target, dds_tex.levels(), mipmaps);
	return tex_id;
}

#else
#include <lib/gli/inc/texture.hpp>

gli::texture spring::load_dds_image(const char* filename) { return {}; }

unsigned int spring::create_dds_texture(
	const gli::texture& dds_tex,
	unsigned int tex_id,
	float anisoLvl,
	float lodBias,
	bool mipmaps
) {
	return 0;
}
#endif

