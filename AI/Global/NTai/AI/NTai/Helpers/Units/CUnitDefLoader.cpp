#include "../../Core/helper.h"

CUnitDefLoader::CUnitDefLoader(Global* GL){
	G = GL;
	unum = G->cb->GetNumUnitDefs();
	if(unum < 1){
		// omgwtf this should enver happen!
		return;
		//G->L.iprint("AI interface says this mod has this many units! :: "+to_string(unum));
	}
	UnitDefList = new const UnitDef*[unum];
	G->cb->GetUnitDefList(UnitDefList);
	for(int n=0; n < unum; n++){
		const UnitDef* pud = UnitDefList[n];
		if(pud == 0) continue;
		string na = pud->name.c_str();
		trim(na);
		tolowercase(na);
		defs[na] = pud;
	}
}
void CUnitDefLoader::Init(){
	
}
const UnitDef* CUnitDefLoader::GetUnitDefByIndex(int i){
	if(i > unum) return 0;
	if(i <0) return 0;
	return UnitDefList[i];
}

const UnitDef* CUnitDefLoader::GetUnitDef(string name){
	string n = name;
	trim(n);
	tolowercase(n);
	if(defs.find(n) != defs.end()){
		const UnitDef* u = defs[n];
		return u;
	}
	return 0;
}
