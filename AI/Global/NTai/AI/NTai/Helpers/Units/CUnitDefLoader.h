// haha

class CUnitDefLoader{
public:
	CUnitDefLoader(Global* GL);
	~CUnitDefLoader();
	void Init();
	
	const UnitDef* GetUnitDef(string name);
	const UnitDef* GetUnitDefByIndex(int i);
	
	weak_ptr<CUnitTypeData> GetUnitTypeDataByUnitId(int uid);
	weak_ptr<CUnitTypeData> GetUnitTypeDataById(int id);
	weak_ptr<CUnitTypeData> GetUnitTypeDataByName(string name);
	
	bool HasUnit(string name);
private:
	int GetIdByName(string name);
	Global* G;
	map<string,int> defs;
	boost::shared_array< shared_ptr<CUnitTypeData> > type_info;
	const UnitDef** UnitDefList;
	int unum;
};
