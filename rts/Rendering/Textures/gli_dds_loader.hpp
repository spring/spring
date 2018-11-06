#ifndef GLI_DDS_LOADER_H
#define GLI_DDS_LOADER_H

namespace gli {
	class texture;
}

namespace spring {
	gli::texture load_dds_image(const char* filename);

	unsigned int create_dds_texture(const gli::texture& dds_tex, unsigned int tex_id, float anisoLvl, float lodBias, bool mipmaps);
}

#endif

