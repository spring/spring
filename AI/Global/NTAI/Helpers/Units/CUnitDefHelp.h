class CUnitDefHelp{
public:
	Global* G;
	CUnitDefHelp(Global* GL);
	virtual ~CUnitDefHelp(){}
	bool IsMex(const UnitDef* ud);
	bool IsEnergy(const UnitDef* ud);
	bool IsFactory(const UnitDef* ud);

	bool IsUWCapable(const UnitDef* ud){
		return IsUWStructure(ud)||IsShip(ud)||IsSub(ud);
	}
	bool IsUWStructure(const UnitDef* ud){ return false;}
	bool IsHovercraft(const UnitDef* ud){ return false;}
	bool IsAmphib(const UnitDef* ud){ return false;}
	bool IsShip(const UnitDef* ud){ return false;}
	bool IsSub(const UnitDef* ud){ return false;}
	//bool IsFactory(const UnitDef* ud){return false;}
};
