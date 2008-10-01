// 

namespace ntai {

	class CUnitDefLoader{
	public:
		CUnitDefLoader(Global* GL);
		~CUnitDefLoader();
		void Init();
		
		const UnitDef* GetUnitDef(string name) const;
		const UnitDef* GetUnitDefByIndex(int i) const;
		
		CUnitTypeData* GetUnitTypeDataByUnitId(int uid) const;
		CUnitTypeData* GetUnitTypeDataById(int id) const;
		CUnitTypeData* GetUnitTypeDataByName(string name) const;
		
		bool HasUnit(string name) const;
	private:
		int GetIdByName(string name) const;

		Global* G;

		std::map<string,int> defs;
		std::map<int,CUnitTypeData* > type_data;

		const UnitDef** UnitDefList;
		int unum;
	};
}
