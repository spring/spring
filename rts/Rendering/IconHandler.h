/* Author: Teake Nutma */

#ifndef ICON_HANDLER_H
#define ICON_HANDLER_H

#include <map>
#include <string>
#include <vector>

#include "Icon.h"
#include "float3.h"


class CIconData {
	public:
		CIconData(); // for CIconHandler::safetyData
		CIconData(const std::string& name, unsigned int texID,
		          float size, float distance, bool radiusAdjust,
		          bool ownTexture, int xsize, int ysize);
		~CIconData();

		void Ref();
		void UnRef();

		void CopyData(const CIconData* iconData);

		void BindTexture() const;
		void Draw(float x0, float y0, float x1, float y1) const;
		void Draw(const float3& botLeft, const float3& botRight,
		          const float3& topLeft, const float3& topRight) const;

		inline const std::string& GetName()         const { return name;         }
		inline const float        GetSize()         const { return size;         }
		inline const float        GetDistance()     const { return distance;     }
		inline const float        GetDistanceSqr()  const { return distSqr;      }
		inline const bool         GetRadiusAdjust() const { return radiusAdjust; }
		inline const int          GetSizeX()        const { return xsize;        }
		inline const int          GetSizeY()        const { return ysize;        }

	private:
		bool ownTexture;
		int  refCount;

		std::string name;
		unsigned int texID;
		int xsize;
		int ysize;
		float size;
		float distance;
		float distSqr;
		bool  radiusAdjust;
};


class CIconHandler {

	friend class CIcon;		

	public:
		CIconHandler(void);
		~CIconHandler(void);

		bool AddIcon(const std::string& iconName,
								 const std::string& textureName,
								 float size, float distance,
								 bool radiusAdjust);

		bool FreeIcon(const std::string& iconName);

		CIcon GetIcon(const std::string& iconName) const;

		CIcon GetDefaultIcon() const { return CIcon(defIconData); }

		const CIconData* GetDefaultIconData() const { return defIconData; }

	private:
		bool LoadIcons(const std::string& filename);
		unsigned int GetDefaultTexture();

	private:
		unsigned int defTexID;
		CIconData* defIconData;

		typedef std::map<std::string, CIcon> IconMap;
		IconMap iconMap;

	private:
		static CIconData safetyData;
};


extern CIconHandler* iconHandler;


#endif // ICON_HANDLER_H
