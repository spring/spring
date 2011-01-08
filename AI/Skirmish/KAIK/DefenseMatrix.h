#ifndef KAIK_DEFENSEMATRIX_HDR
#define KAIK_DEFENSEMATRIX_HDR

#include <vector>
#include "System/float3.h"

class CSpotFinder;
struct AIClasses;
struct UnitDef;

class CDefenseMatrix {
	public:
		CR_DECLARE(CDefenseMatrix);

		CDefenseMatrix(AIClasses* ai);

		void PostLoad();
		void Init();
		void AddDefense(float3 pos, const UnitDef* def);
		void RemoveDefense(float3 pos, const UnitDef* def);
		void UpdateChokePointArray();
		float3 GetDefensePos(const UnitDef* def, float3 builderpos);
		void MaskBadBuildSpot(float3 pos);

		std::vector<std::vector<float> > ChokeMapsByMovetype;
		std::vector<float> ChokePointArray;
		std::vector<int> BuildMaskArray;

	private:
		bool IsInitialized() const { return (spotFinder != NULL); }

		CSpotFinder* spotFinder;
		AIClasses* ai;

		struct DefPos {
			float3 pos;
			const UnitDef* def;
		};
		/**
		 * Used for defs that get added before the matrix is initialized.
		 */
		std::vector<DefPos> defAddQueue;
		/**
		 * Used for defs that got removed after AI init
		 * but before the matrix is initialized.
		 */
		std::vector<DefPos> defRemoveQueue;
};

#endif
