#ifndef MAP_DRAWPASS_TYPES_H
#define MAP_DRAWPASS_TYPES_H

struct DrawPass {
	enum e {
		Normal,
		Shadow,
		WaterReflection,
		WaterRefraction,
		TerrainReflection,
		TerrainDeferred,
	};
};

#endif

