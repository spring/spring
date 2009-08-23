#include "FeatureDef.h"

#include "Rendering/UnitModels/IModelParser.h"

S3DModel* FeatureDef::LoadModel()
{
	if (model==NULL)
		model = modelParser->Load3DModel(modelname);
	return model;
}
