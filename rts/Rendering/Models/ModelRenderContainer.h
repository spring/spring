/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MODEL_RENDER_CONTAINER_HDR
#define MODEL_RENDER_CONTAINER_HDR

#define MDL_TYPE(o) (o->model->type)
#define TEX_TYPE(o) (o->model->textureType)

#include <array>
#include <vector>

#include "Rendering/Models/3DModel.h"
#include "System/ContainerUtil.h"

#if 0
class CUnit;
class CFeature;
class CProjectile;
#endif

template<typename TObject>
class ModelRenderContainer {
private:
	// [0] stores the number of texture-type keys, i.e. non-empty bins
	// note there can be no more texture-types than S3DModel instances
	std::array<int, 1 + MAX_MODEL_OBJECTS> objectBinKeys;
	std::array<std::vector<TObject*>, MAX_MODEL_OBJECTS> objectBins;

	unsigned int numObjects = 0;

public:
	void Kill() {}
	void Init() {
		objectBinKeys.fill(0);

		// reuse inner vectors when reloading
		for (auto& bin: objectBins) {
			bin.clear();
		}

		numObjects = 0;
	}


	void AddObject(const TObject* o) {
		auto& bin = objectBins[TEX_TYPE(o)];
		auto& keys = objectBinKeys;

		if (bin.empty()) {
			bin.reserve(256);

			const auto beg = keys.begin() + 1;
			const auto end = keys.begin() + 1 + keys[0];

			if (std::find(beg, end, TEX_TYPE(o)) == end)
				keys[ ++keys[0] ] = TEX_TYPE(o);
		}

		// cast since updating an object's draw-position requires mutability
		if (!spring::VectorInsertUnique(bin, const_cast<TObject*>(o)))
			assert(false);

		numObjects += 1;
	}

	void DelObject(const TObject* o) {
		assert(TEX_TYPE(o) < objectBins.size());

		auto& bin = objectBins[TEX_TYPE(o)];
		auto& keys = objectBinKeys;

		// object can be legally absent from this container
		// e.g. UnitDrawer invokes DelObject on both opaque
		// and alpha containers (since it does not know the
		// cloaked state)
		if (spring::VectorErase(bin, const_cast<TObject*>(o)))
			numObjects -= 1;

		if (!bin.empty())
			return;

		// keep the empty bin, just remove it from the key-set
		// TODO: might want to keep these in sorted order
		const auto beg = keys.begin() + 1;
		const auto end = keys.begin() + 1 + keys[0];
		const auto  it = std::find(beg, end, TEX_TYPE(o));

		if (it == end)
			return;

		keys[*it] = keys[ keys[0]-- ];
	}


	unsigned int GetNumObjects() const { return numObjects; }
	unsigned int GetNumObjectBins() const { return objectBinKeys[0]; }

	int GetObjectBinKey(unsigned int i) const { return objectBinKeys[1 + i]; }

public:
	typedef typename std::remove_reference<decltype(objectBins[0])>::type ObjectBin;

	const decltype(objectBinKeys)& GetObjectBinKeys() const { return objectBinKeys; }

	const ObjectBin& GetObjectBin       (int key) const { return objectBins[key]; }
	      ObjectBin& GetObjectBinMutable(int key)       { return objectBins[key]; }
};

#endif

