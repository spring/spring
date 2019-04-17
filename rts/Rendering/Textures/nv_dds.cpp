/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// This software contains source code provided by NVIDIA Corporation.
// License: http://developer.download.nvidia.com/licenses/general_license.txt

// Modified DDS reader class from NVIDIA SDK
///////////////////////////////////////////////////////////////////////////////
//
// Description:
// 
// Loads DDS images (DXTC1, DXTC3, DXTC5, RGB (888, 888X), and RGBA (8888) are
// supported) for use in OpenGL. Image is flipped when its loaded as DX images
// are stored with different coordinate system. If file has mipmaps and/or 
// cubemaps then these are loaded as well. Volume textures can be loaded as 
// well but they must be uncompressed.
//
// When multiple textures are loaded (i.e a volume or cubemap texture), 
// additional faces can be accessed using the array operator. 
//
// The mipmaps for each face are also stored in a list and can be accessed like 
// so: image.get_mipmap() (which accesses the first mipmap of the first 
// image). To get the number of mipmaps call the get_num_mipmaps function for
// a given texture.
//
// Call the is_volume() or is_cubemap() function to check that a loaded image
// is a volume or cubemap texture respectively. If a volume texture is loaded
// then the get_depth() function should return a number greater than 1. 
// Mipmapped volume textures and DXTC compressed volume textures are supported.
//
///////////////////////////////////////////////////////////////////////////////
//
// Update: 9/15/2003
//
// Added functions to create new image from a buffer of pixels. Added function
// to save current image to disk.
//
// Update: 6/11/2002
//
// Added some convenience functions to handle uploading textures to OpenGL. The
// following functions have been added:
//
//     bool upload_texture1D();
//     bool upload_texture2D(unsigned int imageIndex = 0, GLenum target = GL_TEXTURE_2D);
//     bool upload_textureRectangle();
//     bool upload_texture3D();
//     bool upload_textureCubemap();
//
// See function implementation below for instructions/comments on using each
// function.
//
// The open function has also been updated to take an optional second parameter
// specifying whether the image should be flipped on load. This defaults to 
// true.
//
///////////////////////////////////////////////////////////////////////////////
// Sample usage
///////////////////////////////////////////////////////////////////////////////
//
// Loading a compressed texture:
//
// CDDSImage image;
// GLuint texobj;
//
// image.load("compressed.dds");
// 
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_2D);
// glBindTexture(GL_TEXTURE_2D, texobj);
//
// glCompressedTexImage2D(GL_TEXTURE_2D, 0, image.get_format(), 
//     image.get_width(), image.get_height(), 0, image.get_size(), 
//     image);
//
// for (int i = 0; i < image.get_num_mipmaps(); i++)
// {
//     CSurface mipmap = image.get_mipmap(i);
//
//     glCompressedTexImage2D(GL_TEXTURE_2D, i+1, image.get_format(), 
//         mipmap.get_width(), mipmap.get_height(), 0, mipmap.get_size(), 
//         mipmap);
// } 
///////////////////////////////////////////////////////////////////////////////
// 
// Loading an uncompressed texture:
//
// CDDSImage image;
// GLuint texobj;
//
// image.load("uncompressed.dds");
//
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_2D);
// glBindTexture(GL_TEXTURE_2D, texobj);
//
// glTexImage2D(GL_TEXTURE_2D, 0, image.get_components(), image.get_width(), 
//     image.get_height(), 0, image.get_format(), GL_UNSIGNED_BYTE, image);
//
// for (int i = 0; i < image.get_num_mipmaps(); i++)
// {
//     glTexImage2D(GL_TEXTURE_2D, i+1, image.get_components(), 
//         image.get_mipmap(i).get_width(), image.get_mipmap(i).get_height(), 
//         0, image.get_format(), GL_UNSIGNED_BYTE, image.get_mipmap(i));
// }
//
///////////////////////////////////////////////////////////////////////////////
// 
// Loading an uncompressed cubemap texture:
//
// CDDSImage image;
// GLuint texobj;
// GLenum target;
// 
// image.load("cubemap.dds");
// 
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_CUBE_MAP);
// glBindTexture(GL_TEXTURE_CUBE_MAP, texobj);
// 
// for (int n = 0; n < 6; n++)
// {
//     target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + n;
// 
//     glTexImage2D(target, 0, image.get_components(), image[n].get_width(), 
//         image[n].get_height(), 0, image.get_format(), GL_UNSIGNED_BYTE, 
//         image[n]);
// 
//     for (int i = 0; i < image[n].get_num_mipmaps(); i++)
//     {
//         glTexImage2D(target, i+1, image.get_components(), 
//             image[n].get_mipmap(i).get_width(), 
//             image[n].get_mipmap(i).get_height(), 0,
//             image.get_format(), GL_UNSIGNED_BYTE, image[n].get_mipmap(i));
//     }
// }
//
///////////////////////////////////////////////////////////////////////////////
// 
// Loading a volume texture:
//
// CDDSImage image;
// GLuint texobj;
// 
// image.load("volume.dds");
// 
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_3D);
// glBindTexture(GL_TEXTURE_3D, texobj);
// 
// PFNGLTEXIMAGE3DPROC glTexImage3D;
// glTexImage3D(GL_TEXTURE_3D, 0, image.get_components(), image.get_width(), 
//     image.get_height(), image.get_depth(), 0, image.get_format(), 
//     GL_UNSIGNED_BYTE, image);
// 
// for (int i = 0; i < image.get_num_mipmaps(); i++)
// {
//     glTexImage3D(GL_TEXTURE_3D, i+1, image.get_components(), 
//         image[0].get_mipmap(i).get_width(), 
//         image[0].get_mipmap(i).get_height(), 
//         image[0].get_mipmap(i).get_depth(), 0, image.get_format(), 
//         GL_UNSIGNED_BYTE, image[0].get_mipmap(i));
// }

#include <cstdio>
#include <cstring>
#include <cassert>

// spring related
#include "Rendering/GL/myGL.h"
#include "nv_dds.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/byteorder.h"
#include "System/Log/ILog.h"

using namespace std;
using namespace nv_dds;

static constexpr unsigned int BASE_INT_FORMATS[] = {0, GL_RED, GL_RG, GL_RGB, GL_RGBA};

///////////////////////////////////////////////////////////////////////////////
// CDDSImage public functions

///////////////////////////////////////////////////////////////////////////////
// default constructor
CDDSImage::CDDSImage()
  : m_format(0),
    m_components(0),
    m_type(TextureNone),
    m_valid(false)
{
}


#if 0
void CDDSImage::create_textureFlat(unsigned int format, unsigned int components, const CTexture &baseImage)
{
    assert(format != 0);
    assert(components != 0);
    assert(baseImage.get_depth() == 1);

    // remove any existing images
    clear();
    
    m_format = format;
    m_components = components;
    m_type = TextureFlat;

    m_images.push_back(baseImage);

    m_valid = true;
}

void CDDSImage::create_texture3D(unsigned int format, unsigned int components, const CTexture &baseImage)
{
    assert(format != 0);
    assert(components != 0);
    assert(baseImage.get_depth() > 1);

    // remove any existing images
    clear();

    m_format = format;
    m_components = components;
    m_type = Texture3D;

    m_images.push_back(baseImage);

    m_valid = true;
}
#endif


#if 0
inline bool same_size(const CTexture &a, const CTexture &b)
{
    if (a.get_width() != b.get_width())
        return false;
    if (a.get_height() != b.get_height())
        return false;
    if (a.get_depth() != b.get_depth())
        return false;

    return true;
}

void CDDSImage::create_textureCubemap(unsigned int format, unsigned int components,
                                      const CTexture &positiveX, const CTexture &negativeX, 
                                      const CTexture &positiveY, const CTexture &negativeY, 
                                      const CTexture &positiveZ, const CTexture &negativeZ)
{
    assert(format != 0);
    assert(components != 0);
    assert(positiveX.get_depth() == 1);

    // verify that all dimensions are the same 
    assert(same_size(positiveX, negativeX));
    assert(same_size(positiveX, positiveY));
    assert(same_size(positiveX, negativeY));
    assert(same_size(positiveX, positiveZ));
    assert(same_size(positiveX, negativeZ));

    // remove any existing images
    clear();

    m_format = format;
    m_components = components;
    m_type = TextureCubemap;

    m_images.push_back(positiveX);
    m_images.push_back(negativeX);
    m_images.push_back(positiveY);
    m_images.push_back(negativeY);
    m_images.push_back(positiveZ);
    m_images.push_back(negativeZ);

    m_valid = true;
}
#endif


///////////////////////////////////////////////////////////////////////////////
// loads DDS image
//
// filename - fully qualified name of DDS image
// flipImage - specifies whether image is flipped on load, default is true
bool CDDSImage::load(string filename, bool flipImage)
{
	assert(filename.length() != 0);

	// clear any previously loaded images
	clear();
    

	// open file
#if 0
	FILE *fp = fopen(filename.c_str(), "rb");
#else
	CFileHandler file(filename);

	std::vector<uint8_t> fileBuf;
	int filePos = 0;

	if (!file.FileExists())
		return false;
#endif

	// read in file marker, make sure its a DDS file
	char filecode[4];
#if 0
	fread(filecode, 1, 4, fp);
#else
	file.Read(filecode, 4);
#endif
	if (strncmp(filecode, "DDS ", 4) != 0)
	{
		#if 0
		fclose(fp);
		#endif
		return false;
	}

	// read in DDS header
	DDS_HEADER ddsh;
#if 0
	fread(&ddsh, sizeof(DDS_HEADER), 1, fp);
#else
	int tmp = sizeof(unsigned int);
	file.Read(&ddsh.dwSize, tmp);
	file.Read(&ddsh.dwFlags, tmp);
	file.Read(&ddsh.dwHeight, tmp);
	file.Read(&ddsh.dwWidth, tmp);
	file.Read(&ddsh.dwPitchOrLinearSize, tmp);
	file.Read(&ddsh.dwDepth, tmp);
	file.Read(&ddsh.dwMipMapCount, tmp);
	file.Read(&ddsh.dwReserved1, tmp*11);
	file.Read(&ddsh.ddspf.dwSize, tmp);
	file.Read(&ddsh.ddspf.dwFlags, tmp);
	file.Read(&ddsh.ddspf.dwFourCC, tmp);
	file.Read(&ddsh.ddspf.dwRGBBitCount, tmp);
	file.Read(&ddsh.ddspf.dwRBitMask, tmp);
	file.Read(&ddsh.ddspf.dwGBitMask, tmp);
	file.Read(&ddsh.ddspf.dwBBitMask, tmp);
	file.Read(&ddsh.ddspf.dwABitMask, tmp);
	file.Read(&ddsh.dwCaps1, tmp);
	file.Read(&ddsh.dwCaps2, tmp);
	file.Read(&ddsh.dwReserved2, tmp*3);

	// if in VFS, read post-header data directly from buffer
	if (file.IsBuffered()) {
		fileBuf = std::move(file.GetBuffer());
		filePos = file.GetPos();
	}
#endif

	ddsh.dwSize = swabDWord(ddsh.dwSize);
	ddsh.dwFlags = swabDWord(ddsh.dwFlags);
	ddsh.dwHeight = swabDWord(ddsh.dwHeight);
	ddsh.dwWidth = swabDWord(ddsh.dwWidth);
	ddsh.dwPitchOrLinearSize = swabDWord(ddsh.dwPitchOrLinearSize);
	ddsh.dwDepth = swabDWord(ddsh.dwDepth);
	ddsh.dwMipMapCount = swabDWord(ddsh.dwMipMapCount);
	ddsh.ddspf.dwSize = swabDWord(ddsh.ddspf.dwSize);
	ddsh.ddspf.dwFlags = swabDWord(ddsh.ddspf.dwFlags);
	ddsh.ddspf.dwFourCC = swabDWord(ddsh.ddspf.dwFourCC);
	ddsh.ddspf.dwRGBBitCount = swabDWord(ddsh.ddspf.dwRGBBitCount);
	ddsh.ddspf.dwRBitMask = swabDWord(ddsh.ddspf.dwRBitMask);
	ddsh.ddspf.dwGBitMask = swabDWord(ddsh.ddspf.dwGBitMask);
	ddsh.ddspf.dwBBitMask = swabDWord(ddsh.ddspf.dwBBitMask);
	ddsh.ddspf.dwABitMask = swabDWord(ddsh.ddspf.dwABitMask);
	ddsh.dwCaps1 = swabDWord(ddsh.dwCaps1);
	ddsh.dwCaps2 = swabDWord(ddsh.dwCaps2);

	// default to flat texture type (1D, 2D, or rectangle)
	m_type = TextureFlat;

	// check if image is a cubemap
	if (ddsh.dwCaps2 & DDSF_CUBEMAP)
		m_type = TextureCubemap;

	// check if image is a volume texture
	if ((ddsh.dwCaps2 & DDSF_VOLUME) && (ddsh.dwDepth > 0))
		m_type = Texture3D;

	// figure out what the image format is
	if (ddsh.ddspf.dwFlags & DDSF_FOURCC)
	{
		switch(ddsh.ddspf.dwFourCC)
		{
			case FOURCC_DXT1:
				m_format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				m_components = 3;
				break;
			case FOURCC_DXT3:
				m_format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				m_components = 4;
				break;
			case FOURCC_DXT5:
				m_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				m_components = 4;
				break;
			default:
				//fclose(fp);
				return false;
		}
	}
	else if (ddsh.ddspf.dwFlags == DDSF_RGBA && ddsh.ddspf.dwRGBBitCount == 32)
	{
		m_format = GL_BGRA;
		m_components = 4;
	}
	else if (ddsh.ddspf.dwFlags == DDSF_RGB  && ddsh.ddspf.dwRGBBitCount == 32)
	{
		m_format = GL_BGRA;
		m_components = 4;
	}
	else if (ddsh.ddspf.dwFlags == DDSF_RGB  && ddsh.ddspf.dwRGBBitCount == 24)
	{
		m_format = GL_BGR;
		m_components = 3;
	}
	else if (ddsh.ddspf.dwRGBBitCount == 8)
	{
		m_format = GL_RED;
		m_components = 1;
	}
	else
	{
		#if 0
		fclose(fp);
		#endif
		return false;
	}


	// store primary surface width/height/depth
	unsigned int width = ddsh.dwWidth;
	unsigned int height = ddsh.dwHeight;
	unsigned int depth = clamp_size(ddsh.dwDepth);   // set to 1 if 0

	// use correct size calculation function depending on whether image is
	// compressed
	unsigned int (CDDSImage::*sizefunc)(unsigned int, unsigned int) const = (is_compressed() ? &CDDSImage::size_dxtc : &CDDSImage::size_rgb);

	m_images.reserve((m_type == TextureCubemap)? 6 : 1);

	// load all surfaces for the image (6 surfaces for cubemaps)
	for (unsigned int n = 0; n < (unsigned int)(m_type == TextureCubemap ? 6 : 1); n++)
	{
		// add empty texture object
		m_images.emplace_back();

		// get reference to newly added texture object
		CTexture &img = m_images[n];

		// calculate surface size
		unsigned int size = (this->*sizefunc)(width, height)*depth;


	#if 0
		// load surface
		unsigned char *pixels = new unsigned char[size];

		fread(pixels, 1, size, fp);
	#else
		if (fileBuf.empty()) {
			fileBuf.resize(size);

			file.Read(fileBuf.data(), size);
			img.create(width, height, depth, size, fileBuf.data());

			fileBuf.clear();
		} else {
			img.create(width, height, depth, size, fileBuf.data() + filePos);
			filePos += size;
		}
	#endif


		if (flipImage)
			flip(img);
        
		unsigned int w = clamp_size(width >> 1);
		unsigned int h = clamp_size(height >> 1);
		unsigned int d = clamp_size(depth >> 1);

		// store number of mipmaps
		unsigned int numMipmaps = ddsh.dwMipMapCount;

		// number of mipmaps in file includes main surface so decrease count
		// by one
		if (numMipmaps != 0)
			numMipmaps--;

		// load all mipmaps for current surface
		for (unsigned int i = 0; i < numMipmaps && (w || h); i++)
		{
			// add empty surface
			img.add_mipmap();

			// get reference to newly added mipmap
			CSurface &mipmap = img.get_mipmap(i);

			// calculate mipmap size
			size = (this->*sizefunc)(w, h)*d;


		#if 0
			unsigned char *pixels = new unsigned char[size];

			fread(pixels, 1, size, fp);
		#else
			if (fileBuf.empty()) {
				fileBuf.resize(size);

				file.Read(fileBuf.data(), size);
				mipmap.create(w, h, d, size, fileBuf.data());

				fileBuf.clear();
			} else {
				mipmap.create(w, h, d, size, fileBuf.data() + filePos);
				filePos += size;
			}
		#endif


			if (flipImage)
				flip(mipmap);

			// shrink to next power of 2
			w = clamp_size(w >> 1);
			h = clamp_size(h >> 1);
			d = clamp_size(d >> 1);
		}
	}

	// swap cubemaps on y axis (since image is flipped in OGL)
	if (m_type == TextureCubemap && flipImage)
	{
		CTexture tmp = std::move(m_images[3]);
		m_images[3] = std::move(m_images[2]);
		m_images[2] = std::move(tmp);
	}

	#if 0
	fclose(fp);
	#endif

	return (m_valid = true);
}

bool CDDSImage::write_texture(const CTexture &texture, FILE *fp) const
{
    assert(get_num_mipmaps() == texture.get_num_mipmaps());
    
    int res = fwrite(texture, 1, texture.get_size(), fp);
    if (res<0) {
        return false;
    }
    for (unsigned int i = 0; i < texture.get_num_mipmaps(); i++)
    {
        const CSurface &mipmap = texture.get_mipmap(i);
        res = fwrite(mipmap, 1, mipmap.get_size(), fp);
        if (res<0) {
            return false;
        }
    }
    return true;
}

bool CDDSImage::save(std::string filename, bool flipImage) const
{
    assert(m_valid);
    assert(m_type != TextureNone);

    DDS_HEADER ddsh;
    unsigned int headerSize = sizeof(DDS_HEADER);
    memset(&ddsh, 0, headerSize);
    ddsh.dwSize = headerSize;
    ddsh.dwFlags = DDSF_CAPS | DDSF_WIDTH | DDSF_HEIGHT | DDSF_PIXELFORMAT;
    ddsh.dwHeight = get_height();
    ddsh.dwWidth = get_width();

    if (is_compressed())
    {
        ddsh.dwFlags |= DDSF_LINEARSIZE;
        ddsh.dwPitchOrLinearSize = get_size();
    }
    else
    {
        ddsh.dwFlags |= DDSF_PITCH;
        ddsh.dwPitchOrLinearSize = get_dword_aligned_linesize(get_width(), m_components * 8);
    }
    
    if (m_type == Texture3D)
    {
        ddsh.dwFlags |= DDSF_DEPTH;
        ddsh.dwDepth = get_depth();
    }

    if (get_num_mipmaps() > 0)
    {
        ddsh.dwFlags |= DDSF_MIPMAPCOUNT;
        ddsh.dwMipMapCount = get_num_mipmaps() + 1;
    }

    ddsh.ddspf.dwSize = sizeof(DDS_PIXELFORMAT);

    if (is_compressed())
    {
        ddsh.ddspf.dwFlags = DDSF_FOURCC;
        
        if (m_format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
            ddsh.ddspf.dwFourCC = FOURCC_DXT1;
        if (m_format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
            ddsh.ddspf.dwFourCC = FOURCC_DXT3;
        if (m_format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
            ddsh.ddspf.dwFourCC = FOURCC_DXT5;
    }
    else
    {
        ddsh.ddspf.dwFlags = (m_components == 4) ? DDSF_RGBA : DDSF_RGB;
        ddsh.ddspf.dwRGBBitCount = m_components * 8;
        ddsh.ddspf.dwRBitMask = 0x00ff0000;
        ddsh.ddspf.dwGBitMask = 0x0000ff00;
        ddsh.ddspf.dwBBitMask = 0x000000ff;
 
        if (m_components == 4)
        {
            ddsh.ddspf.dwFlags |= DDSF_ALPHAPIXELS;
            ddsh.ddspf.dwABitMask = 0xff000000;
        }
    }
    
    ddsh.dwCaps1 = DDSF_TEXTURE;
    
    if (m_type == TextureCubemap)
    {
        ddsh.dwCaps1 |= DDSF_COMPLEX;
        ddsh.dwCaps2 = DDSF_CUBEMAP | DDSF_CUBEMAP_ALL_FACES;
    }

    if (m_type == Texture3D)
    {
        ddsh.dwCaps1 |= DDSF_COMPLEX;
        ddsh.dwCaps2 = DDSF_VOLUME;
    }

    if (get_num_mipmaps() > 0)
        ddsh.dwCaps1 |= DDSF_COMPLEX | DDSF_MIPMAP;

    // open file
    FILE *fp = fopen(filename.c_str(), "wb");
    if (fp == nullptr) {
        LOG_L(L_ERROR, "couldn't create texture %s", filename.c_str());
        return false;
    }

    // write file header
    int res = fwrite("DDS ", 1, 4, fp);
    if (res<0) {
        fclose(fp);
        return false;
    }
    
    // write dds header
    res = fwrite(&ddsh, 1, sizeof(DDS_HEADER), fp);
    if (res<0) {
        fclose(fp);
        return false;
    }

    if (m_type != TextureCubemap)
    {
        CTexture tex = m_images[0]; // copy
        if (flipImage) flip_texture(tex);
        if (!write_texture(tex, fp)) {
            LOG_L(L_ERROR, "couldn't write texture %s: %s",filename.c_str(), strerror(ferror(fp)));
            fclose(fp);
            return false;
	}
    }
    else
    {
        assert(m_images.size() == 6);

        for (unsigned int i = 0; i < m_images.size(); i++)
        {
            CTexture cubeFace; // copy

            if (i == 2) 
                cubeFace = m_images[3];
            else if (i == 3) 
                cubeFace = m_images[2];
            else 
                cubeFace = m_images[i];

            if (flipImage) flip_texture(cubeFace);
            write_texture(cubeFace, fp);
        }
    }

    fclose(fp);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// free image memory
void CDDSImage::clear()
{
    m_components = 0;
    m_format = 0;
    m_type = TextureNone;
    m_valid = false;

    m_images.clear();
}

bool CDDSImage::is_compressed() const
{
	return ((m_format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ||
		   (m_format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) ||
		   (m_format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT));
}

#ifndef BITMAP_NO_OPENGL

///////////////////////////////////////////////////////////////////////////////
// uploads a compressed/uncompressed 1D texture
bool CDDSImage::upload_texture1D() const
{
    assert(m_valid);
    assert(!m_images.empty());

    const CTexture &baseImage = m_images[0];

    assert(baseImage.get_height() == 1);
    assert(baseImage.get_width() > 0);

    if (is_compressed())
    {
        glCompressedTexImage1D(GL_TEXTURE_1D, 0, m_format, 
            baseImage.get_width(), 0, baseImage.get_size(), baseImage);
        
        // load all mipmaps
        for (unsigned int i = 0; i < baseImage.get_num_mipmaps(); i++)
        {
            const CSurface &mipmap = baseImage.get_mipmap(i);
            glCompressedTexImage1D(GL_TEXTURE_1D, i+1, m_format, 
                mipmap.get_width(), 0, mipmap.get_size(), mipmap);
        }
    }
    else
    {
        GLint alignment = -1;
        if (!is_dword_aligned())
        {
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }

        glTexImage1D(GL_TEXTURE_1D, 0, BASE_INT_FORMATS[m_components], baseImage.get_width(), 0,
            m_format, GL_UNSIGNED_BYTE, baseImage);

        // load all mipmaps
        for (unsigned int i = 0; i < baseImage.get_num_mipmaps(); i++)
        {
            const CSurface &mipmap = baseImage.get_mipmap(i);

            glTexImage1D(GL_TEXTURE_1D, i+1, BASE_INT_FORMATS[m_components],
                mipmap.get_width(), 0, m_format, GL_UNSIGNED_BYTE, mipmap);
        }

        if (alignment != -1)
            glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// uploads a compressed/uncompressed 2D texture
//
// imageIndex - allows you to optionally specify other loaded surfaces for 2D
//              textures such as a face in a cubemap or a slice in a volume
//
//              default: 0
//
// target     - allows you to optionally specify a different texture target for
//              the 2D texture such as a specific face of a cubemap
//
//              default: GL_TEXTURE_2D
bool CDDSImage::upload_texture2D(unsigned int imageIndex, int target) const
{
    assert(m_valid);
    assert(!m_images.empty());
    assert(imageIndex >= 0);
    assert(imageIndex < m_images.size());
    assert(m_images[imageIndex]);

    const CTexture &image = m_images[imageIndex];

    assert(image.get_height() > 0);
    assert(image.get_width() > 0);
    assert(target == GL_TEXTURE_2D || target == GL_TEXTURE_RECTANGLE_NV ||
        (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && 
         target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z));
    
    if (is_compressed())
    {
        glCompressedTexImage2D(target, 0, m_format, image.get_width(), 
            image.get_height(), 0, image.get_size(), image);
        
        // load all mipmaps
        for (unsigned int i = 0; i < image.get_num_mipmaps(); i++)
        {
            const CSurface &mipmap = image.get_mipmap(i);
            glCompressedTexImage2D(target, i+1, m_format, 
                mipmap.get_width(), mipmap.get_height(), 0, 
                mipmap.get_size(), mipmap);
        }
    }
    else
    {
        GLint alignment = -1;
        if (!is_dword_aligned())
        {
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }

        glTexImage2D(target, 0, BASE_INT_FORMATS[m_components], image.get_width(),
            image.get_height(), 0, m_format, GL_UNSIGNED_BYTE,
            image);

        // load all mipmaps
        for (unsigned int i = 0; i < image.get_num_mipmaps(); i++)
        {
            const CSurface &mipmap = image.get_mipmap(i);
            
            glTexImage2D(target, i+1, BASE_INT_FORMATS[m_components], mipmap.get_width(),
                mipmap.get_height(), 0, m_format, GL_UNSIGNED_BYTE, mipmap); 
        }

        if (alignment != -1)
            glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    }
    
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// uploads a compressed/uncompressed 3D texture
bool CDDSImage::upload_texture3D() const
{
    assert(m_valid);
    assert(!m_images.empty());
    assert(m_type == Texture3D);

    const CTexture &baseImage = m_images[0];
    
    assert(baseImage.get_depth() >= 1);

    if (is_compressed())
    {
        glCompressedTexImage3D(GL_TEXTURE_3D, 0, m_format,  
            baseImage.get_width(), baseImage.get_height(), baseImage.get_depth(),
            0, baseImage.get_size(), baseImage);
        
        // load all mipmap volumes
        for (unsigned int i = 0; i < baseImage.get_num_mipmaps(); i++)
        {
            const CSurface &mipmap = baseImage.get_mipmap(i);
            glCompressedTexImage3D(GL_TEXTURE_3D, i+1, m_format, 
                mipmap.get_width(), mipmap.get_height(), mipmap.get_depth(), 
                0, mipmap.get_size(), mipmap);
        }
    }
    else
    {
        GLint alignment = -1;
        if (!is_dword_aligned())
        {
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }

        glTexImage3D(GL_TEXTURE_3D, 0, BASE_INT_FORMATS[m_components], baseImage.get_width(),
            baseImage.get_height(), baseImage.get_depth(), 0, m_format,
            GL_UNSIGNED_BYTE, baseImage);
        
        // load all mipmap volumes
        for (unsigned int i = 0; i < baseImage.get_num_mipmaps(); i++)
        {
            const CSurface &mipmap = baseImage.get_mipmap(i);

            glTexImage3D(GL_TEXTURE_3D, i+1, BASE_INT_FORMATS[m_components],
                mipmap.get_width(), mipmap.get_height(), mipmap.get_depth(), 0, 
                m_format, GL_UNSIGNED_BYTE,  mipmap);
        }

        if (alignment != -1)
            glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    }
    
    return true;
}

bool CDDSImage::upload_textureRectangle() const
{
    return upload_texture2D(0, GL_TEXTURE_RECTANGLE_NV);
}

///////////////////////////////////////////////////////////////////////////////
// uploads a compressed/uncompressed cubemap texture
bool CDDSImage::upload_textureCubemap() const
{
    assert(m_valid);
    assert(!m_images.empty());
    assert(m_type == TextureCubemap);
    assert(m_images.size() == 6);

    GLenum target;

    // loop through cubemap faces and load them as 2D textures 
    for (unsigned int n = 0; n < 6; n++)
    {
        // specify cubemap face
        target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + n;
        if (!upload_texture2D(n, target))
            return false;
    }

    return true;
}

#endif // !BITMAP_NO_OPENGL

///////////////////////////////////////////////////////////////////////////////
// clamps input size to [1-size]
inline unsigned int CDDSImage::clamp_size(unsigned int size) const
{
    if (size <= 0)
        size = 1;

    return size;
}

///////////////////////////////////////////////////////////////////////////////
// CDDSImage private functions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// calculates size of DXTC texture in bytes
inline unsigned int CDDSImage::size_dxtc(unsigned int width, unsigned int height) const
{
    return ((width+3)/4)*((height+3)/4)*
        (m_format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16);   
}

///////////////////////////////////////////////////////////////////////////////
// calculates size of uncompressed RGB texture in bytes
inline unsigned int CDDSImage::size_rgb(unsigned int width, unsigned int height) const
{
    return width*height*m_components;
}

///////////////////////////////////////////////////////////////////////////////
// flip image around X axis
void CDDSImage::flip(CSurface &surface) const
{
    unsigned int linesize;

    if (!is_compressed())
    {
        assert(surface.get_depth() > 0);

        unsigned int imagesize = surface.get_size()/surface.get_depth();
        linesize = imagesize / surface.get_height();

        for (unsigned int n = 0; n < surface.get_depth(); n++)
        {
            unsigned int offset = imagesize*n;
            unsigned char *top = (unsigned char*)surface + offset;
            unsigned char *bottom = top + (imagesize-linesize);
    
            for (unsigned int i = 0; i < (surface.get_height() >> 1); i++)
            {
                swap(bottom, top, linesize);

                top += linesize;
                bottom -= linesize;
            }
        }
    }
    else
    {
        void (CDDSImage::*flipblocks)(DXTColBlock*, unsigned int) const = nullptr;
        unsigned int xblocks = surface.get_width() / 4;
        unsigned int yblocks = surface.get_height() / 4;
        unsigned int blocksize;

        switch (m_format)
        {
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: 
                blocksize = 8;
                flipblocks = &CDDSImage::flip_blocks_dxtc1;
                break;
            case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: 
                blocksize = 16;
                flipblocks = &CDDSImage::flip_blocks_dxtc3;
                break;
            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: 
                blocksize = 16;
                flipblocks = &CDDSImage::flip_blocks_dxtc5;
                break;
            default:
                return;
        }

        linesize = xblocks * blocksize;

        DXTColBlock *top;
        DXTColBlock *bottom;
    
        for (unsigned int j = 0; j < (yblocks >> 1); j++)
        {
            top = (DXTColBlock*)((unsigned char*)surface+ j * linesize);
            bottom = (DXTColBlock*)((unsigned char*)surface + (((yblocks-j)-1) * linesize));

            (this->*flipblocks)(top, xblocks);
            (this->*flipblocks)(bottom, xblocks);

            swap(bottom, top, linesize);
        }
    }
}    

void CDDSImage::flip_texture(CTexture &texture) const
{
    flip(texture);
    
    for (unsigned int i = 0; i < texture.get_num_mipmaps(); i++)
    {
        flip(texture.get_mipmap(i));
    }
}

///////////////////////////////////////////////////////////////////////////////
// swap to sections of memory
void CDDSImage::swap(void *byte1, void *byte2, unsigned int size) const
{
    unsigned char *tmp = new unsigned char[size];

    memcpy(tmp, byte1, size);
    memcpy(byte1, byte2, size);
    memcpy(byte2, tmp, size);

    delete [] tmp;
}

///////////////////////////////////////////////////////////////////////////////
// flip a DXT1 color block
void CDDSImage::flip_blocks_dxtc1(DXTColBlock *line, unsigned int numBlocks) const
{
    DXTColBlock *curblock = line;

    for (unsigned int i = 0; i < numBlocks; i++)
    {
        swap(&curblock->row[0], &curblock->row[3], sizeof(unsigned char));
        swap(&curblock->row[1], &curblock->row[2], sizeof(unsigned char));

        curblock++;
    }
}

///////////////////////////////////////////////////////////////////////////////
// flip a DXT3 color block
void CDDSImage::flip_blocks_dxtc3(DXTColBlock *line, unsigned int numBlocks) const
{
    DXTColBlock *curblock = line;
    DXT3AlphaBlock *alphablock;

    for (unsigned int i = 0; i < numBlocks; i++)
    {
        alphablock = reinterpret_cast<DXT3AlphaBlock*>(curblock);

        swap(&alphablock->row[0], &alphablock->row[3], sizeof(unsigned short));
        swap(&alphablock->row[1], &alphablock->row[2], sizeof(unsigned short));

        curblock++;

        swap(&curblock->row[0], &curblock->row[3], sizeof(unsigned char));
        swap(&curblock->row[1], &curblock->row[2], sizeof(unsigned char));

        curblock++;
    }
}

///////////////////////////////////////////////////////////////////////////////
// flip a DXT5 alpha block
void CDDSImage::flip_dxt5_alpha(DXT5AlphaBlock *block) const
{
    unsigned char gBits[4][4];
    
    const unsigned int mask = 0x00000007;          // bits = 00 00 01 11
    unsigned int bits = 0;
    memcpy(&bits, &block->row[0], sizeof(unsigned char) * 3);

    gBits[0][0] = (unsigned char)(bits & mask);
    bits >>= 3;
    gBits[0][1] = (unsigned char)(bits & mask);
    bits >>= 3;
    gBits[0][2] = (unsigned char)(bits & mask);
    bits >>= 3;
    gBits[0][3] = (unsigned char)(bits & mask);
    bits >>= 3;
    gBits[1][0] = (unsigned char)(bits & mask);
    bits >>= 3;
    gBits[1][1] = (unsigned char)(bits & mask);
    bits >>= 3;
    gBits[1][2] = (unsigned char)(bits & mask);
    bits >>= 3;
    gBits[1][3] = (unsigned char)(bits & mask);

    bits = 0;
    memcpy(&bits, &block->row[3], sizeof(unsigned char) * 3);

    gBits[2][0] = (unsigned char)(bits & mask);
    bits >>= 3;
    gBits[2][1] = (unsigned char)(bits & mask);
    bits >>= 3;
    gBits[2][2] = (unsigned char)(bits & mask);
    bits >>= 3;
    gBits[2][3] = (unsigned char)(bits & mask);
    bits >>= 3;
    gBits[3][0] = (unsigned char)(bits & mask);
    bits >>= 3;
    gBits[3][1] = (unsigned char)(bits & mask);
    bits >>= 3;
    gBits[3][2] = (unsigned char)(bits & mask);
    bits >>= 3;
    gBits[3][3] = (unsigned char)(bits & mask);

    memset(block->row, 0, sizeof(unsigned char) * 6);

    unsigned int *pBits = ((unsigned int*) &(block->row[0]));

    *pBits = *pBits | (gBits[3][0] << 0);
    *pBits = *pBits | (gBits[3][1] << 3);
    *pBits = *pBits | (gBits[3][2] << 6);
    *pBits = *pBits | (gBits[3][3] << 9);

    *pBits = *pBits | (gBits[2][0] << 12);
    *pBits = *pBits | (gBits[2][1] << 15);
    *pBits = *pBits | (gBits[2][2] << 18);
    *pBits = *pBits | (gBits[2][3] << 21);

    pBits = ((unsigned int*) &(block->row[3]));

#ifdef MACOS
    *pBits &= 0x000000ff;
#else
    *pBits &= 0xff000000;
#endif

    *pBits = *pBits | (gBits[1][0] << 0);
    *pBits = *pBits | (gBits[1][1] << 3);
    *pBits = *pBits | (gBits[1][2] << 6);
    *pBits = *pBits | (gBits[1][3] << 9);

    *pBits = *pBits | (gBits[0][0] << 12);
    *pBits = *pBits | (gBits[0][1] << 15);
    *pBits = *pBits | (gBits[0][2] << 18);
    *pBits = *pBits | (gBits[0][3] << 21);
}

///////////////////////////////////////////////////////////////////////////////
// flip a DXT5 color block
void CDDSImage::flip_blocks_dxtc5(DXTColBlock *line, unsigned int numBlocks) const
{
    DXTColBlock *curblock = line;
    DXT5AlphaBlock *alphablock;
    
    for (unsigned int i = 0; i < numBlocks; i++)
    {
        alphablock = reinterpret_cast<DXT5AlphaBlock*>(curblock);
        
        flip_dxt5_alpha(alphablock);

        curblock++;

        swap(&curblock->row[0], &curblock->row[3], sizeof(unsigned char));
        swap(&curblock->row[1], &curblock->row[2], sizeof(unsigned char));

        curblock++;
    }
}

///////////////////////////////////////////////////////////////////////////////
// CTexture implementation
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// creates an empty texture
CTexture::CTexture(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const unsigned char *pixels)
  : CSurface(w, h, d, imgsize, pixels)  // initialize base class part
{
}

///////////////////////////////////////////////////////////////////////////////
// assignment operator
CTexture &CTexture::operator=(const CTexture &rhs)
{
	if (this != &rhs) {
		CSurface::operator=(rhs);

		m_mipmaps.clear();
		m_mipmaps.resize(rhs.get_num_mipmaps());

		for (unsigned int i = 0; i < rhs.get_num_mipmaps(); i++)
			m_mipmaps[i] = rhs.m_mipmaps[i];
	}

    return *this;
}

CTexture &CTexture::operator=(CTexture &&rhs) noexcept
{
	if (this != &rhs) {
		CSurface::operator=(rhs);

		m_mipmaps.clear();
		m_mipmaps.resize(rhs.get_num_mipmaps());

		for (unsigned int i = 0; i < rhs.get_num_mipmaps(); i++)
			m_mipmaps[i] = std::move(rhs.m_mipmaps[i]);

		rhs.m_mipmaps.clear();
	}

	return *this;
}

void CTexture::create(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const unsigned char *pixels)
{
    CSurface::create(w, h, d, imgsize, pixels);

    m_mipmaps.clear();
}

void CTexture::clear()
{
    CSurface::clear();

    m_mipmaps.clear();
}

///////////////////////////////////////////////////////////////////////////////
// CSurface implementation
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// default constructor
CSurface::CSurface()
  : m_width(0),
    m_height(0),
    m_depth(0),
    m_size(0),
    m_pixels(nullptr)
{
}

///////////////////////////////////////////////////////////////////////////////
// creates an empty image
CSurface::CSurface(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const unsigned char *pixels)
  : m_width(0),
    m_height(0),
    m_depth(0),
    m_size(0),
    m_pixels(nullptr)
{
    create(w, h, d, imgsize, pixels);
}

///////////////////////////////////////////////////////////////////////////////
// assignment operator
CSurface &CSurface::operator= (const CSurface &rhs)
{
	if (this != &rhs) {
		clear();

		m_size = rhs.get_size();
		m_width = rhs.get_width();
		m_height = rhs.get_height();
		m_depth = rhs.get_depth();

		if (m_size != 0) {
			m_pixels = new unsigned char[m_size];
			memcpy(m_pixels, rhs, m_size);
		}
	}

	return *this;
}

CSurface &CSurface::operator=(CSurface &&rhs) noexcept
{
	if (this != &rhs) {
		clear();

		m_size = rhs.get_size();
		m_width = rhs.get_width();
		m_height = rhs.get_height();
		m_depth = rhs.get_depth();

		m_pixels = rhs.m_pixels;
		rhs.m_pixels = nullptr;
	}

	return *this;
}

///////////////////////////////////////////////////////////////////////////////
// returns a pointer to image
CSurface::operator unsigned char*() const
{ 
    return m_pixels; 
}

///////////////////////////////////////////////////////////////////////////////
// creates an empty image
void CSurface::create(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const unsigned char *pixels)
{
    assert(w != 0);
    assert(h != 0);
    assert(d != 0);
    assert(imgsize != 0);
    assert(pixels);

    clear();

    m_width = w;
    m_height = h;
    m_depth = d;
    m_size = imgsize;
    m_pixels = new unsigned char[imgsize];
    memcpy(m_pixels, pixels, imgsize);
}

///////////////////////////////////////////////////////////////////////////////
// free surface memory
void CSurface::clear()
{
	delete [] m_pixels;
	m_pixels = nullptr;
}
