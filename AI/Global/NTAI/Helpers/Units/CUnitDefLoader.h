// haha

class CUnitDefLoader{
public:
	CUnitDefLoader(Global* GL);
	virtual ~CUnitDefLoader(){}
	void Init();
	const UnitDef* GetUnitDef(string name);
	const UnitDef* GetUnitDefByIndex(int i);
private:
	Global* G;
	map<string,const UnitDef*> defs;
	const UnitDef** UnitDefList;
	int unum;
};