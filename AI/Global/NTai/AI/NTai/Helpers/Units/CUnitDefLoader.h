// 

namespace ntai {

	class CUnitDefLoader{
	public:
		CUnitDefLoader(Global* GL);
		~CUnitDefLoader();
		void Init();
		
		const UnitDef* GetUnitDef(string name);
		const UnitDef* GetUnitDefByIndex(int i);
		
		CUnitTypeData* GetUnitTypeDataByUnitId(int uid);
		CUnitTypeData* GetUnitTypeDataById(int id);
		CUnitTypeData* GetUnitTypeDataByName(string name);
		
		bool HasUnit(string name);
	private:
		int GetIdByName(string name);
		Global* G;
		map<string,int> defs;
		std::map<int,CUnitTypeData* > type_data;

		const UnitDef** UnitDefList;
		int unum;
	};
}
