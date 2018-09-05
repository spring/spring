/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MODEL_RENDER_CONTAINER_HDR
#define MODEL_RENDER_CONTAINER_HDR

#define MDL_TYPE(o) (o->model->type)
#define TEX_TYPE(o) (o->model->textureType)

#include <array>
#include <vector>

#include "Rendering/Models/3DModel.h"
#include "System/ContainerUtil.h"

template<typename TObject>
class ModelRenderContainer {
private:
	// [0] stores the number of texture-type keys, i.e. non-empty bins
	// note there can be no more texture-types than S3DModel instances
	std::array<int, 1 + MAX_MODEL_OBJECTS> objectBinKeys;
	std::array<std::vector<TObject*>, MAX_MODEL_OBJECTS> objectBins;

	unsigned int numObjects = 0;

	typedef           decltype(objectBinKeys)              BinKeySet;
	typedef  typename decltype(objectBins   )::value_type  ObjectBin;

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

			#ifdef DEBUG
			const auto beg = keys.begin() + 1;
			const auto end = keys.begin() + 1 + keys[0];

			assert(std::find(beg, end, TEX_TYPE(o)) == end);
			#endif

			keys[ ++keys[0] ] = TEX_TYPE(o);
		}

		// cast since updating an object's draw-position requires mutability
		numObjects += spring::VectorInsertUnique(bin, const_cast<TObject*>(o));
	}

	void DelObject(const TObject* o) {
		assert(TEX_TYPE(o) < objectBins.size());

		auto& bin = objectBins[TEX_TYPE(o)];
		auto& keys = objectBinKeys;

		// object can be legally absent from this container
		// e.g. UnitDrawer invokes DelObject on both opaque
		// and alpha containers (since it does not know the
		// cloaked state) which also means the tex-type key
		// might not exist here
		numObjects -= spring::VectorErase(bin, const_cast<TObject*>(o));

		if (!bin.empty())
			return;

		// keep empty bin, just remove it from the key-set
		// TODO: might want to (re)sort keys, less jumping
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

	const BinKeySet& GetObjectBinKeys() const { return objectBinKeys; }
	const ObjectBin& GetObjectBin(int key) const { return objectBins[key]; }
};

#endif

