/// @brief Include to compress and decompress s3tc blocks
/// @file gli/s3tc.hpp

#pragma once

namespace gli
{
	namespace detail
	{
		struct dxt1_block {
			uint16_t Color0;
			uint16_t Color1;
			uint8_t Row[4];
		};

		struct dxt3_block {
			uint16_t AlphaRow[4];
			uint16_t Color0;
			uint16_t Color1;
			uint8_t Row[4];
		};

		struct dxt5_block {
			uint8_t Alpha[2];
			uint8_t AlphaBitmap[6];
			uint16_t Color0;
			uint16_t Color1;
			uint8_t Row[4];
		};

		struct texel_block4x4 {
			// row x col
			glm::vec4 Texel[4][4];
		};
		
		glm::vec4 decompress_dxt1(const dxt1_block &Block, const extent2d &BlockTexelCoord);
		texel_block4x4 decompress_dxt1_block(const dxt1_block &Block);

		glm::vec4 decompress_dxt3(const dxt3_block &Block, const extent2d &BlockTexelCoord);
		texel_block4x4 decompress_dxt3_block(const dxt3_block &Block);

		glm::vec4 decompress_dxt5(const dxt5_block &Block, const extent2d &BlockTexelCoord);
		texel_block4x4 decompress_dxt5_block(const dxt5_block &Block);

	}//namespace detail
}//namespace gli

#include "./s3tc.inl"
