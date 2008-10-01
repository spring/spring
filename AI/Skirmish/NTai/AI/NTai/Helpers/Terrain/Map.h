// Map

namespace ntai {

	struct SearchOffset {

		int dx;
		int dy;

		/** dx*dx+dy*dy */
		int qdist;

	};

	class CMap{
	public:
		CMap(Global* GL);

		void Init(){}

		/**
		* the first base pos (defaulted to by standard)
		*/
		float3 basepos;

		/**
		 * base positions (factorys etc)
		 */
		vector<float3> base_positions;

		/**
		 * returns the base position nearest to the given float3
		 * @param pos The position to be tested
		 * @return The nearest base position to the pos parameter
		 */
		float3 nbasepos(float3 pos);

		float3 distfrom(float3 Start, float3 Target, float distance);

		bool Overlap(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2);

		/**
		 * rotates a position around a given origin and angle
		 *
		 * @return The rotated position
		 */
		float3 Rotate(float3 pos, float angle, float3 origin);

		vector<SearchOffset> GetSearchOffsetTable (int radius);

		bool CheckFloat3(float3 pos);

		/**
		 * returns a value signifying which corner of the map this location is in
		 * @return The corner of the map the parameter is occupying
		 */
		t_direction WhichCorner(float3 pos);

		float GetAngle(float3 origin, float3 a, float3 b);
		float3 Pos2BuildPos(float3 pos, const UnitDef* ud);
		float GetBuildHeight(float3 pos, const UnitDef* unitdef);

		int xMapSize;
		int yMapSize;
	private:
		Global* G;
	};
}
