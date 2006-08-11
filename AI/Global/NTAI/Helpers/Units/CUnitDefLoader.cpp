#include "../../Core/helper.h"

CUnitDefLoader::CUnitDefLoader(Global* GL){
	G = GL;
	int unum = G->cb->GetNumUnitDefs();
	const UnitDef** UnitDefList = new const UnitDef*[unum];
	G->cb->GetUnitDefList(UnitDefList);
	for(int n=0; n < unum; n++){
		const UnitDef* pud = UnitDefList[n];
		string na = G->lowercase(pud->name);
		defs[na] = pud;
	}
}
void CUnitDefLoader::Init(){
	
}
const UnitDef* CUnitDefLoader::GetUnitDef(string name){
	string n = G->lowercase(name);
	if(defs.find(n) != defs.end()){
		return defs[n];
	}
	return 0;
}
