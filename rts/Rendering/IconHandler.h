/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ICON_HANDLER_H
#define ICON_HANDLER_H

#include <array>
#include <string>

#include "Icon.h"
#include "System/float3.h"
#include "System/UnorderedMap.hpp"

namespace icon {
	class CIconData {
		public:
			CIconData() = default;
			CIconData(CIconData&& id) { *this = std::move(id); }
			CIconData(
				const std::string& name,
				unsigned int texID,
				float size,
				float distance,
				bool radiusAdjust,
				bool ownTexture,
				unsigned int xsize,
				unsigned int ysize
			);
			~CIconData();

			CIconData& operator = (CIconData&& id) {
				std::swap(name, id.name);

				std::swap(refCount, id.refCount);
				std::swap(texID, id.texID);

				xsize = id.xsize;
				ysize = id.ysize;

				size = id.size;
				distance = id.distance;
				distSqr = id.distSqr;

				std::swap(ownTexture, id.ownTexture);

				radiusAdjust = id.radiusAdjust;
				return *this;
			}

			void Ref() { refCount++; }
			void UnRef() {
				if ((--refCount) > 0)
					return;

				// trigger texture deletion
				*this = {};
			}

			void CopyData(const CIconData* iconData);
			void SwapOwner(CIconData* iconData) {
				ownTexture = true;
				iconData->ownTexture = false;
			}

			void BindTexture() const;

			const std::string& GetName()   const { return name;         }

			unsigned int GetTextureID()    const { return texID;        }
			int          GetSizeX()        const { return xsize;        }
			int          GetSizeY()        const { return ysize;        }

			float        GetSize()         const { return size;         }
			float        GetDistance()     const { return distance;     }
			float        GetDistanceSqr()  const { return distSqr;      }
			float        GetRadiusScale()  const { return 30.0f;        }

			bool         GetRadiusAdjust() const { return radiusAdjust; }

		private:
			std::string name;

			int refCount = 123456;
			unsigned int texID = 0;
			int xsize = 1;
			int ysize = 1;

			float size = 1.0f;
			float distance = 1.0f;
			float distSqr = 1.0f;

			bool ownTexture = false;
			bool radiusAdjust = false;
	};


	class CIconHandler {
		friend class CIcon;

		public:
			CIconHandler() = default;
			CIconHandler(const CIconHandler&) = delete; // no-copy

			void Init() { LoadIcons("gamedata/icontypes.lua"); }
			void Kill();

			bool AddIcon(
				const std::string& iconName,
				const std::string& texName,
				float size,
				float distance,
				bool radiusAdjust
			);

			bool FreeIcon(const std::string& iconName);

			CIcon GetIcon(const std::string& iconName) const;
			CIcon GetSafetyIcon() const { return CIcon(SAFETY_DATA_IDX); }
			CIcon GetDefaultIcon() const { return CIcon(DEFAULT_DATA_IDX); }

			static const CIconData* GetSafetyIconData();
			static const CIconData* GetDefaultIconData();

		private:
			CIconData* GetIconDataMut(unsigned int idx) { return (const_cast<CIconData*>(GetIconData(idx))); }

			const CIconData* GetIconData(unsigned int idx) const {
				switch (idx) {
					case  SAFETY_DATA_IDX: { return (GetSafetyIconData());  } break;
					case DEFAULT_DATA_IDX: { return (GetDefaultIconData()); } break;
					default              : {                                } break;
				}

				return &iconData[idx - ICON_DATA_OFFSET];
			}

			bool LoadIcons(const std::string& filename);
			unsigned int GetDefaultTexture();

		public:
			static constexpr unsigned int  SAFETY_DATA_IDX = 0;
			static constexpr unsigned int DEFAULT_DATA_IDX = 1;
			static constexpr unsigned int ICON_DATA_OFFSET = 2;

			static constexpr unsigned int DEFAULT_TEX_SIZE_X = 128;
			static constexpr unsigned int DEFAULT_TEX_SIZE_Y = 128;

		private:
			unsigned int defTexID = 0;
			unsigned int numIcons = 0;

			spring::unordered_map<std::string, CIcon> iconMap;
			std::array<CIconData, 2048> iconData;
	};

	extern CIconHandler iconHandler;
}

#endif // ICON_HANDLER_H
