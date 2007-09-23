class CUnitDefHelp{
public:
	Global* G;
	CUnitDefHelp(Global* GL);
	virtual ~CUnitDefHelp(){}
	bool IsMex(const UnitDef* ud);
	bool IsMetalMaker(const UnitDef* ud);
	bool IsEnergy(const UnitDef* ud);
	bool IsFactory(const UnitDef* ud);
	bool IsHub(const UnitDef* ud);

	bool IsAirCraft(const UnitDef* ud);
	bool IsGunship(const UnitDef* ud);
	bool IsFighter(const UnitDef* ud);
	bool IsBomber(const UnitDef* ud);

	bool IsUWCapable(const UnitDef* ud){
		return IsUWStructure(ud)||IsShip(ud)||IsSub(ud);
	}
	bool IsUWStructure(const UnitDef* ud){ return false;}
	bool IsHovercraft(const UnitDef* ud){ return false;}
	bool IsAmphib(const UnitDef* ud){ return false;}
	bool IsShip(const UnitDef* ud){ return false;}
	bool IsSub(const UnitDef* ud){ return false;}

	bool IsMobile(const UnitDef* ud);

	bool IsAIRSUPPORT(const UnitDef* ud);

	bool IsAttacker(const UnitDef* ud);

};
