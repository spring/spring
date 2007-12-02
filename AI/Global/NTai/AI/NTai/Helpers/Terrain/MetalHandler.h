#ifndef METALHANDLER_H
#define METALHANDLER_H

namespace ntai {

	class CMetalHandler{
	public:
		CMetalHandler(Global* GLI);
		~CMetalHandler(void);

		CMetalMap* m;

		bool Viable();
		float getExtractionRanged(float x, float z);

		std::vector<float3> metalpatch;
		std::vector<float3> hotspot;
		std::vector<int> mex;

		void loadState();

		/**
		*	Return best zone to extract metal, where there
		*	are more than usual metal spot and with higher
		*	value. It's not safe to delete the returned
		*	pointer.
		*
		*	@return vector<float3> where the .y represent the zone radius
		*/
		std::vector<float3> *getHotSpots();

		/**
		*	Return the metal patch that can be used in a determined
		*	radius, filtering out metal spot that are under the influence
		*	of other extractor. You should take care of delete the returned
		*	pointer after usage.
		*
		*	@param float3 pos the search position, .y represnet the search radius
		*	@param float minMetal the minimum metal value extracted that is returned.
		*		note that a metal a map have is 255*IGlobalAIInterface::getMaxMetal()
		*	@param float depth is the extracting capacity of the unit you would build,
		*		to provide support for moho extractor. (as in UnitDef::extractsMetal, 
		*		that is the metal multiplier, or depth) 		
		*	@return vector<float3> where .y represent the metal extracted
		*	        by a standard extractor in that point
		*/
		std::vector<float3> *getMetalPatch(float3 pos, float minMetal, /*float radius,*/float depth);
		
		/**
		*	Quick call, this calls the getMetalPatch and returns
		*	the nearest metal patch from pos. You should take care to delete
		*	the returned pointer after usage.
		*
		*	@param float3 pos the search position, .y represnet the search radius
		*	@param float minMetal the minimum metal value extracted that is returned
		*	@param float depth is the extracting capacity of the unit you would build,
		*		to provide support for moho extractor. (use depth=UnitDef::extractsMetal)		
		*
		*	@return float3 where y represent the metal extracted by a standard extractor
		*           in that point, y=-1 on error
		*/
		float3 getNearestPatch(float3 pos, float minMetal, float depth, const UnitDef* ud);
		
		/**
		*	If you wand support for getting results afar from other
		*	metal extractor, you should call this class whenewer
		*	a unit is added to the game. Note that it already does
		*	internals check to exlude non extracting units, so adding
		*	your own external chek will result in a little overhead,
		*	also this function will be inlined.
		*
		*	@param unit a unitid added for the ai to control
		*/
		void addExtractor(int unit) {
			mex.push_back(unit);
		};

		/**
		*	Used to notify the metal handler that a metal unit
		*	is no longer here and that the metal map
		*	has to be freed from it's influence
		*	
		*	@param unit The ID of the extractor unit
		*
		*/
		void removeExtractor(int unit) {
			if(mex.empty() == false){
				for(vector<int>::iterator qi = mex.begin();qi!=mex.end();++qi){
					if(*qi == unit){
						mex.erase(qi);
						break;
					}
				}
			}
		};

	private:
		Global* G;
		IAICallback *cb;
		float getMetalAmount(int x, int z);
	};
}

#endif
