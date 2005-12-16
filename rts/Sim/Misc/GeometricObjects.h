#ifndef GEOMETRICOBJECTS_H
#define GEOMETRICOBJECTS_H

#include <map>
#include <vector>


class CGeoSquareProjectile;

class CGeometricObjects
{
public:
	CGeometricObjects(void);
	~CGeometricObjects(void);

	struct GeoGroup{
		std::vector<CGeoSquareProjectile*> squares;
	};

	std::map<int,GeoGroup>	geoGroups;

	std::multimap<int,int> toBeDeleted;

	int firstFreeGroup;
	int AddSpline(float3 b1, float3 b2, float3 b3, float3 b4, float width, int arrow, int lifeTime=-1, int group=0);
	void DeleteGroup(int group);
	void SetColor(int group, float r, float g, float b, float a);
	float3 CalcSpline(float pos, const float3& p1, const float3& p2, const float3& p3, const float3& p4);
	int AddLine(float3 start, float3 end, float width, int arrow, int lifetime=-1, int group=0);
	void Update(void);
	void MarkSquare(int mapSquare);
};

extern CGeometricObjects* geometricObjects;

#endif /* GEOMETRICOBJECTS_H */
