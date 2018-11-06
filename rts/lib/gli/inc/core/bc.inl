#include <glm/gtc/packing.hpp>

namespace gli
{
	namespace detail
	{
		inline glm::vec4 decompress_bc1(const bc1_block &Block, const extent2d &BlockTexelCoord)
		{
			return decompress_dxt1(Block, BlockTexelCoord);
		}

		inline texel_block4x4 decompress_bc1_block(const bc1_block &Block)
		{
			return decompress_dxt1_block(Block);
		}

		inline glm::vec4 decompress_bc2(const bc2_block &Block, const extent2d &BlockTexelCoord)
		{
			return decompress_dxt3(Block, BlockTexelCoord);
		}

		inline texel_block4x4 decompress_bc2_block(const bc2_block &Block)
		{
			return decompress_dxt3_block(Block);
		}

		inline glm::vec4 decompress_bc3(const bc3_block &Block, const extent2d &BlockTexelCoord)
		{
			return decompress_dxt5(Block, BlockTexelCoord);
		}

		inline texel_block4x4 decompress_bc3_block(const bc3_block &Block)
		{
			return decompress_dxt5_block(Block);
		}

		inline void create_single_channel_lookup_table(bool Interpolate6, float Min, float *LookupTable)
		{
			if(Interpolate6)
			{
				LookupTable[2] = (6.0f / 7.0f) * LookupTable[0] + (1.0f / 7.0f) * LookupTable[1];
				LookupTable[3] = (5.0f / 7.0f) * LookupTable[0] + (2.0f / 7.0f) * LookupTable[1];
				LookupTable[4] = (4.0f / 7.0f) * LookupTable[0] + (3.0f / 7.0f) * LookupTable[1];
				LookupTable[5] = (3.0f / 7.0f) * LookupTable[0] + (4.0f / 7.0f) * LookupTable[1];
				LookupTable[6] = (2.0f / 7.0f) * LookupTable[0] + (5.0f / 7.0f) * LookupTable[1];
				LookupTable[7] = (1.0f / 7.0f) * LookupTable[0] + (6.0f / 7.0f) * LookupTable[1];
			}
			else
			{
				LookupTable[2] = (4.0f / 5.0f) * LookupTable[0] + (1.0f / 5.0f) * LookupTable[1];
				LookupTable[3] = (3.0f / 5.0f) * LookupTable[0] + (2.0f / 5.0f) * LookupTable[1];
				LookupTable[4] = (2.0f / 5.0f) * LookupTable[0] + (3.0f / 5.0f) * LookupTable[1];
				LookupTable[5] = (1.0f / 5.0f) * LookupTable[0] + (4.0f / 5.0f) * LookupTable[1];
				LookupTable[6] = Min;
				LookupTable[7] = 1.0f;
			}
		}

		inline void single_channel_bitmap_data_unorm(uint8_t Channel0, uint8_t Channel1, const uint8_t *ChannelBitmap, float *LookupTable, uint64_t &ContiguousBitmap)
		{
			LookupTable[0] = Channel0 / 255.0f;
			LookupTable[1] = Channel1 / 255.0f;

			create_single_channel_lookup_table(Channel0 > Channel1, 0.0f, LookupTable);

			ContiguousBitmap = ChannelBitmap[0] | (ChannelBitmap[1] << 8) | (ChannelBitmap[2] << 16);
			ContiguousBitmap |= uint64_t(ChannelBitmap[3] | (ChannelBitmap[4] << 8) | (ChannelBitmap[5] << 16)) << 24;
		}

		inline void single_channel_bitmap_data_snorm(uint8_t Channel0, uint8_t Channel1, const uint8_t *ChannelBitmap, float *LookupTable, uint64_t &ContiguousBitmap)
		{
			LookupTable[0] = (Channel0 / 255.0f) * 2.0f - 1.0f;
			LookupTable[1] = (Channel1 / 255.0f) * 2.0f - 1.0f;

			create_single_channel_lookup_table(Channel0 > Channel1, -1.0f, LookupTable);

			ContiguousBitmap = ChannelBitmap[0] | (ChannelBitmap[1] << 8) | (ChannelBitmap[2] << 16);
			ContiguousBitmap |= uint64_t(ChannelBitmap[3] | (ChannelBitmap[4] << 8) | (ChannelBitmap[5] << 16)) << 24;
		}

		inline void single_channel_bitmap_data_snorm(uint8_t Channel0, uint32_t Channel1, bool Interpolate6, const uint8_t *ChannelBitmap, float *LookupTable, uint64_t &ContiguousBitmap)
		{
			LookupTable[0] = (Channel0 / 255.0f) * 2.0f - 1.0f;
			LookupTable[1] = (Channel1 / 255.0f) * 2.0f - 1.0f;

			create_single_channel_lookup_table(Interpolate6, -1.0f, LookupTable);

			ContiguousBitmap = ChannelBitmap[0] | (ChannelBitmap[1] << 8) | (ChannelBitmap[2] << 16);
			ContiguousBitmap |= uint64_t(ChannelBitmap[3] | (ChannelBitmap[4] << 8) | (ChannelBitmap[5] << 16)) << 24;
		}

		inline glm::vec4 decompress_bc4unorm(const bc4_block &Block, const extent2d &BlockTexelCoord)
		{
			float RedLUT[8];
			uint64_t Bitmap;

			single_channel_bitmap_data_unorm(Block.Red0, Block.Red1, Block.Bitmap, RedLUT, Bitmap);

			uint8_t RedIndex = (Bitmap >> ((BlockTexelCoord.y * 4 + BlockTexelCoord.x) * 3)) & 0x7;
			return glm::vec4(RedLUT[RedIndex], 0.0f, 0.0f, 1.0f);
		}

		inline glm::vec4 decompress_bc4snorm(const bc4_block &Block, const extent2d &BlockTexelCoord)
		{
			float RedLUT[8];
			uint64_t Bitmap;

			single_channel_bitmap_data_snorm(Block.Red0, Block.Red1, Block.Bitmap, RedLUT, Bitmap);

			uint8_t RedIndex = (Bitmap >> ((BlockTexelCoord.y * 4 + BlockTexelCoord.x) * 3)) & 0x7;
			return glm::vec4(RedLUT[RedIndex], 0.0f, 0.0f, 1.0f);
		}

		inline texel_block4x4 decompress_bc4unorm_block(const bc4_block &Block)
		{
			float RedLUT[8];
			uint64_t Bitmap;

			single_channel_bitmap_data_unorm(Block.Red0, Block.Red1, Block.Bitmap, RedLUT, Bitmap);

			texel_block4x4 TexelBlock;
			for(uint8_t Row = 0; Row < 4; ++Row)
			{
				for(uint8_t Col = 0; Col < 4; ++Col)
				{
					uint8_t RedIndex = (Bitmap >> ((Row * 4 + Col) * 3)) & 0x7;
					TexelBlock.Texel[Row][Col] = glm::vec4(RedLUT[RedIndex], 0.0f, 0.0f, 1.0f);
				}
			}
			
			return TexelBlock;
		}

		inline texel_block4x4 decompress_bc4snorm_block(const bc4_block &Block)
		{
			float RedLUT[8];
			uint64_t Bitmap;

			single_channel_bitmap_data_snorm(Block.Red0, Block.Red1, Block.Bitmap, RedLUT, Bitmap);

			texel_block4x4 TexelBlock;
			for(uint8_t Row = 0; Row < 4; ++Row)
			{
				for(uint8_t Col = 0; Col < 4; ++Col)
				{
					uint8_t RedIndex = (Bitmap >> ((Row * 4 + Col) * 3)) & 0x7;
					TexelBlock.Texel[Row][Col] = glm::vec4(RedLUT[RedIndex], 0.0f, 0.0f, 1.0f);
				}
			}

			return TexelBlock;
		}

		inline glm::vec4 decompress_bc5unorm(const bc5_block &Block, const extent2d &BlockTexelCoord)
		{
			float RedLUT[8];
			uint64_t RedBitmap;

			single_channel_bitmap_data_unorm(Block.Red0, Block.Red1, Block.RedBitmap, RedLUT, RedBitmap);
			uint8_t RedIndex = (RedBitmap >> ((BlockTexelCoord.y * 4 + BlockTexelCoord.x) * 3)) & 0x7;
			
			float GreenLUT[8];
			uint64_t GreenBitmap;

			single_channel_bitmap_data_unorm(Block.Green0, Block.Green1, Block.GreenBitmap, GreenLUT, GreenBitmap);
			uint8_t GreenIndex = (GreenBitmap >> ((BlockTexelCoord.y * 4 + BlockTexelCoord.x) * 3)) & 0x7;

			return glm::vec4(RedLUT[RedIndex], GreenLUT[GreenIndex], 0.0f, 1.0f);
		}

		inline glm::vec4 decompress_bc5snorm(const bc5_block &Block, const extent2d &BlockTexelCoord)
		{
			float RedLUT[8];
			uint64_t RedBitmap;

			single_channel_bitmap_data_snorm(Block.Red0, Block.Red1, Block.RedBitmap, RedLUT, RedBitmap);
			uint8_t RedIndex = (RedBitmap >> ((BlockTexelCoord.y * 4 + BlockTexelCoord.x) * 3)) & 0x7;

			float GreenLUT[8];
			uint64_t GreenBitmap;

			single_channel_bitmap_data_snorm(Block.Green0, Block.Green1, Block.GreenBitmap, GreenLUT, GreenBitmap);
			uint8_t GreenIndex = (GreenBitmap >> ((BlockTexelCoord.y * 4 + BlockTexelCoord.x) * 3)) & 0x7;

			return glm::vec4(RedLUT[RedIndex], GreenLUT[GreenIndex], 0.0f, 1.0f);
		}

		inline texel_block4x4 decompress_bc5unorm_block(const bc5_block &Block)
		{
			float RedLUT[8];
			uint64_t RedBitmap;

			single_channel_bitmap_data_unorm(Block.Red0, Block.Red1, Block.RedBitmap, RedLUT, RedBitmap);
			
			float GreenLUT[8];
			uint64_t GreenBitmap;

			single_channel_bitmap_data_unorm(Block.Green0, Block.Green1, Block.GreenBitmap, GreenLUT, GreenBitmap);

			texel_block4x4 TexelBlock;
			for(uint8_t Row = 0; Row < 4; ++Row)
			{
				for(uint8_t Col = 0; Col < 4; ++Col)
				{
					uint8_t RedIndex = (RedBitmap >> ((Row * 4 + Col) * 3)) & 0x7;
					uint8_t GreenIndex = (GreenBitmap >> ((Row * 4 + Col) * 3)) & 0x7;
					TexelBlock.Texel[Row][Col] = glm::vec4(RedLUT[RedIndex], GreenLUT[GreenIndex], 0.0f, 1.0f);
				}
			}
			
			return TexelBlock;
		}

		inline texel_block4x4 decompress_bc5snorm_block(const bc5_block &Block)
		{
			float RedLUT[8];
			uint64_t RedBitmap;

			single_channel_bitmap_data_snorm(Block.Red0, Block.Red1, Block.RedBitmap, RedLUT, RedBitmap);

			float GreenLUT[8];
			uint64_t GreenBitmap;

			single_channel_bitmap_data_snorm(Block.Green0, Block.Green1, Block.Red0 > Block.Red1, Block.GreenBitmap, GreenLUT, GreenBitmap);

			texel_block4x4 TexelBlock;
			for(uint8_t Row = 0; Row < 4; ++Row)
			{
				for(uint8_t Col = 0; Col < 4; ++Col)
				{
					uint8_t RedIndex = (RedBitmap >> ((Row * 4 + Col) * 3)) & 0x7;
					uint8_t GreenIndex = (GreenBitmap >> ((Row * 4 + Col) * 3)) & 0x7;
					TexelBlock.Texel[Row][Col] = glm::vec4(RedLUT[RedIndex], GreenLUT[GreenIndex], 0.0f, 1.0f);
				}
			}

			return TexelBlock;
		}

	}//namespace detail
}//namespace gli
