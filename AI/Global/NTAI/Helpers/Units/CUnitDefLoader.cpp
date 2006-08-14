#include "../../Core/helper.h"

CUnitDefLoader::CUnitDefLoader(Global* GL){
	G = GL;
	unum = G->cb->GetNumUnitDefs();
	UnitDefList = new const UnitDef*[unum];
	G->cb->GetUnitDefList(UnitDefList);
	for(int n=0; n < unum; n++){
		const UnitDef* pud = UnitDefList[n];
		string na = pud->name;
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
		return defs[n];
	}
	return 0;
}