/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// This software contains source code provided by NVIDIA Corporation.
// License: http://developer.download.nvidia.com/licenses/general_license.txt

// Modified DDS class from NVIDIA SDK.

#ifndef __NV_DDS_H__
#define __NV_DDS_H__

#include <string>
#include <vector>
#include <cstdio>
#include <assert.h>

namespace nv_dds
{
    // surface description flags
    const unsigned int DDSF_CAPS           = 0x00000001;
    const unsigned int DDSF_HEIGHT         = 0x00000002;
    const unsigned int DDSF_WIDTH          = 0x00000004;
    const unsigned int DDSF_PITCH          = 0x00000008;
    const unsigned int DDSF_PIXELFORMAT    = 0x00001000;
    const unsigned int DDSF_MIPMAPCOUNT    = 0x00020000;
    const unsigned int DDSF_LINEARSIZE     = 0x00080000;
    const unsigned int DDSF_DEPTH          = 0x00800000;

    // pixel format flags
    const unsigned int DDSF_ALPHAPIXELS    = 0x00000001;
    const unsigned int DDSF_FOURCC         = 0x00000004;
    const unsigned int DDSF_RGB            = 0x00000040;
    const unsigned int DDSF_RGBA           = 0x00000041;

    // dwCaps1 flags
    const unsigned int DDSF_COMPLEX         = 0x00000008;
    const unsigned int DDSF_TEXTURE         = 0x00001000;
    const unsigned int DDSF_MIPMAP          = 0x00400000;

    // dwCaps2 flags
    const unsigned int DDSF_CUBEMAP         = 0x00000200;
    const unsigned int DDSF_CUBEMAP_POSITIVEX  = 0x00000400;
    const unsigned int DDSF_CUBEMAP_NEGATIVEX  = 0x00000800;
    const unsigned int DDSF_CUBEMAP_POSITIVEY  = 0x00001000;
    const unsigned int DDSF_CUBEMAP_NEGATIVEY  = 0x00002000;
    const unsigned int DDSF_CUBEMAP_POSITIVEZ  = 0x00004000;
    const unsigned int DDSF_CUBEMAP_NEGATIVEZ  = 0x00008000;
    const unsigned int DDSF_CUBEMAP_ALL_FACES  = 0x0000FC00;
    const unsigned int DDSF_VOLUME          = 0x00200000;

    // compressed texture types
    const unsigned int FOURCC_DXT1 = 0x31545844; //(MAKEFOURCC('D','X','T','1'))
    const unsigned int FOURCC_DXT3 = 0x33545844; //(MAKEFOURCC('D','X','T','3'))
    const unsigned int FOURCC_DXT5 = 0x35545844; //(MAKEFOURCC('D','X','T','5'))

    struct DXTColBlock
    {
        unsigned short col0;
        unsigned short col1;

        unsigned char row[4];
    };

    struct DXT3AlphaBlock
    {
        unsigned short row[4];
    };

    struct DXT5AlphaBlock
    {
        unsigned char alpha0;
        unsigned char alpha1;

        unsigned char row[6];
    };

    struct DDS_PIXELFORMAT
    {
        unsigned int dwSize;
        unsigned int dwFlags;
        unsigned int dwFourCC;
        unsigned int dwRGBBitCount;
        unsigned int dwRBitMask;
        unsigned int dwGBitMask;
        unsigned int dwBBitMask;
        unsigned int dwABitMask;
    };

    struct DDS_HEADER
    {
        unsigned int dwSize;
        unsigned int dwFlags;
        unsigned int dwHeight;
        unsigned int dwWidth;
        unsigned int dwPitchOrLinearSize;
        unsigned int dwDepth;
        unsigned int dwMipMapCount;
        unsigned int dwReserved1[11];
        DDS_PIXELFORMAT ddspf;
        unsigned int dwCaps1;
        unsigned int dwCaps2;
        unsigned int dwReserved2[3];
    };

    enum TextureType
    {
        TextureNone,
        TextureFlat,    // 1D, 2D, and rectangle textures
        Texture3D,
        TextureCubemap
    };

    class CSurface
    {
        public:
            CSurface();
            CSurface(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const unsigned char *pixels);
            CSurface(const CSurface &copy): CSurface() { *this = copy; }
            CSurface(CSurface &&surf): CSurface() { *this = std::move(surf); }
            CSurface &operator= (const CSurface &rhs);
            CSurface &operator= (CSurface &&rhs) noexcept;
            virtual ~CSurface() { clear(); }

            operator unsigned char*() const;

            virtual void create(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const unsigned char *pixels);
            virtual void clear();

            inline unsigned int get_width() const { return m_width; }
            inline unsigned int get_height() const { return m_height; }
            inline unsigned int get_depth() const { return m_depth; }
            inline unsigned int get_size() const { return m_size; }

        private:
            unsigned int m_width;
            unsigned int m_height;
            unsigned int m_depth;
            unsigned int m_size;

            unsigned char *m_pixels;
    };

    class CTexture : public CSurface
    {
        friend class CDDSImage;

        public:
            CTexture(): CSurface() {}
            CTexture(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const unsigned char *pixels);
            CTexture(const CTexture &copy): CTexture() { *this = copy; }
            CTexture(CTexture &&text): CTexture() { *this = std::move(text); }
            CTexture &operator=(const CTexture &rhs);
            CTexture &operator=(CTexture &&rhs) noexcept;

            void create(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const unsigned char *pixels);
            void clear();

            inline const CSurface &get_mipmap(unsigned int index) const
            {
                assert(!m_mipmaps.empty());
                assert(index < m_mipmaps.size());

                return m_mipmaps[index];
            }

            inline void add_mipmap()
            {
                m_mipmaps.emplace_back();
            }

            inline unsigned int get_num_mipmaps() const { return (unsigned int)m_mipmaps.size(); }

        protected:
            inline CSurface &get_mipmap(unsigned int index)
            {
                assert(!m_mipmaps.empty());
                assert(index < m_mipmaps.size());

                return m_mipmaps[index];
            }

        private:
            std::vector<CSurface> m_mipmaps;
    };

    class CDDSImage
    {
        public:
			CDDSImage();
			CDDSImage(const CDDSImage& img) { *this = img; }
			CDDSImage(CDDSImage&& img) { *this = std::move(img); }

			CDDSImage& operator = (const CDDSImage& img) {
				m_format = img.m_format;
				m_components = img.m_components;
				m_type = img.m_type;
				m_valid = img.m_valid;

				m_images = img.m_images;
				return *this;
			}
			CDDSImage& operator = (CDDSImage&& img) {
				m_format = img.m_format;
				m_components = img.m_components;
				m_type = img.m_type;
				m_valid = img.m_valid;

				m_images = std::move(img.m_images);
				return *this;
			}

			#if 0
            void create_textureFlat(unsigned int format, unsigned int components, const CTexture &baseImage);
            void create_texture3D(unsigned int format, unsigned int components, const CTexture &baseImage);
            void create_textureCubemap(unsigned int format, unsigned int components,
                                       const CTexture &positiveX, const CTexture &negativeX,
                                       const CTexture &positiveY, const CTexture &negativeY,
                                       const CTexture &positiveZ, const CTexture &negativeZ);
			#endif

            void clear();
            bool load(std::string filename, bool flipImage = true);
            bool save(std::string filename, bool flipImage = true) const;

            bool upload_texture1D() const;
            bool upload_texture2D(unsigned int imageIndex, int target) const;
            bool upload_texture3D() const;
            bool upload_textureRectangle() const;
            bool upload_textureCubemap() const;

            inline operator unsigned char*()
            {
                assert(m_valid);
                assert(!m_images.empty());

                return m_images[0];
            }

            inline unsigned int get_width() const
            {
                assert(m_valid);
                assert(!m_images.empty());

                return m_images[0].get_width();
            }

            inline unsigned int get_height() const
            {
                assert(m_valid);
                assert(!m_images.empty());

                return m_images[0].get_height();
            }

            inline unsigned int get_depth() const
            {
                assert(m_valid);
                assert(!m_images.empty());

                return m_images[0].get_depth();
            }

            inline unsigned int get_size() const
            {
                assert(m_valid);
                assert(!m_images.empty());

                return m_images[0].get_size();
            }

            inline unsigned int get_num_mipmaps() const
            {
                assert(m_valid);
                assert(!m_images.empty());

                return m_images[0].get_num_mipmaps();
            }

            inline const CSurface &get_mipmap(unsigned int index) const
            {
                assert(m_valid);
                assert(!m_images.empty());
                assert(index < m_images[0].get_num_mipmaps());

                return m_images[0].get_mipmap(index);
            }

            inline const CTexture &get_cubemap_face(unsigned int face) const
            {
                assert(m_valid);
                assert(!m_images.empty());
                assert(m_images.size() == 6);
                assert(m_type == TextureCubemap);
                assert(face < 6);

                return m_images[face];
            }

            inline unsigned int get_components() const { return m_components; }
            inline unsigned int get_format() const { return m_format; }
            inline TextureType get_type() const { return m_type; }

            bool is_compressed() const;

            inline bool is_cubemap() const { return (m_type == TextureCubemap); }
            inline bool is_volume() const { return (m_type == Texture3D); }
            inline bool is_valid() const { return m_valid; }

            inline bool is_dword_aligned() const
            {
                assert(m_valid);

                int dwordLineSize = get_dword_aligned_linesize(get_width(), m_components*8);
                int curLineSize = get_width() * m_components;

                return (dwordLineSize == curLineSize);
            }

        private:
            unsigned int clamp_size(unsigned int size) const;
            unsigned int size_dxtc(unsigned int width, unsigned int height) const;
            unsigned int size_rgb(unsigned int width, unsigned int height) const;

            // calculates 4-byte aligned width of image
            inline unsigned int get_dword_aligned_linesize(unsigned int width, unsigned int bpp) const
            {
                return ((width * bpp + 31) & -32) >> 3;
            }

            void flip(CSurface &surface) const;
            void flip_texture(CTexture &texture) const;

            void swap(void *byte1, void *byte2, unsigned int size) const;
            void flip_blocks_dxtc1(DXTColBlock *line, unsigned int numBlocks) const;
            void flip_blocks_dxtc3(DXTColBlock *line, unsigned int numBlocks) const;
            void flip_blocks_dxtc5(DXTColBlock *line, unsigned int numBlocks) const;
            void flip_dxt5_alpha(DXT5AlphaBlock *block) const;

            bool write_texture(const CTexture &texture, FILE *fp) const;

            unsigned int m_format;
            unsigned int m_components;
            TextureType m_type;
            bool m_valid;

            std::vector<CTexture> m_images;
    };
}

#endif
