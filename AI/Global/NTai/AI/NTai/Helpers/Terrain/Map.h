// Map

struct SearchOffset {
	int dx,dy;
	int qdist; // dx*dx+dy*dy
};

class CMap{
public:
	CMap(Global* GL);
	void Init(){}

	float3 basepos;					// the first base pos (defaulted to by standard)
	vector<float3> base_positions;	// base positions (factorys etc)
	float3 nbasepos(float3 pos);	// returns the base position nearest to the given float3

	float3 distfrom(float3 Start, float3 Target, float distance);
	bool Overlap(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2);
	float3 Rotate(float3 pos, float angle, float3 origin); // rotates a position around a given origin and angle

	vector<SearchOffset> GetSearchOffsetTable (int radius);

	bool CheckFloat3(float3 pos);
	t_direction WhichCorner(float3 pos);//returns a value signifying which corner of the map this location is in

	float GetAngle(float3 origin, float3 a, float3 b);
	float3 Pos2BuildPos(float3 pos, const UnitDef* ud);
	float GetBuildHeight(float3 pos, const UnitDef* unitdef);

	int xMapSize;
	int yMapSize;
private:
	Global* G;
};
