#pragma once

#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/3DModelVAO.h"

class ModelPreloader {
public:
	static void Load() {
		// map features are loaded earlier in featureHandler.LoadFeaturesFromMap(); - not a big deal
		// Functions below cannot be multithreaded because modelLoader.LoadModel() deal with OpenGL functions
		LoadUnitDefs();
		LoadFeatureDefs();
		LoadWeaponDefs();

		// after that point we should've loaded all models, it's time to dispatch VBO/EBO/VAO creation
		S3DModelVAO::GetInstance().Init();
	}
private:
	static void LoadUnitDefs();
	static void LoadFeatureDefs();
	static void LoadWeaponDefs();
};