#include "../../Core/helper.h"

CUnitDefLoader::CUnitDefLoader(Global* GL){

	// Initialize pointer to the Global class
	G = GL;
	
	// retrieve the number of unit definitions
	unum = G->cb->GetNumUnitDefs();

	// for debugging purposes:
	//G->L.iprint("AI interface says this mod has this many units! :: "+to_string(unum));
	
	// Check if a horrific error has occured
	if(unum < 1){
		// omgwtf this should never happen!
		G->L.eprint("URGENT! GetNumUnitDefs returned ZERO!! This means that there are no unit definitions of any kind!!! Unrecoverable error!");

		// A horrible event has occurred somewhere in the spring engine for this to have happened.
		// Even if the AI could recover from this, the engine could not.
		// Exit this method immediatly. A crash is likely but if the crash was fixed, the engine
		// or another AI, would crash afterwards anyway.
		return;

		
	}

	// initialize arrays

	// The unitdeflist array will be passed to the engine where it will be filled with pointers
	UnitDefList = new const UnitDef*[unum];

	// Create an array of UnitTypeData shared_ptr objects
	shared_ptr<CUnitTypeData>* types = new shared_ptr<CUnitTypeData>[unum];

	// initialize boost::shared_array
	type_info.reset(types);

	// retrieve the list of unit definition pointers from the engine
	G->cb->GetUnitDefList(UnitDefList);

	// for each definition
	for(int n=0; n < unum; n++){

		// retrieve the units definition
		const UnitDef* pud = UnitDefList[n];

		// initialize a UnitTypeData object and add it into the main array
		type_info[n] = shared_ptr<CUnitTypeData>(new CUnitTypeData());

		// now initialize the newly added object with the unit definition
		type_info[n]->Init(G,pud);

		// check if the unit definition is zero, if so skip
		if(pud == 0) continue;

		// make sure the name is in the correct format and add it to the map container
		string na = pud->name.c_str();
		trim(na);
		tolowercase(na);
		defs[na] = pud->id;
	}
}

CUnitDefLoader::~CUnitDefLoader(){
	delete[] UnitDefList;
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
		const UnitDef* u = GetUnitDefByIndex(defs[n]);
		return u;
	}
	return 0;
}

weak_ptr<CUnitTypeData> CUnitDefLoader::GetUnitTypeDataByUnitId(int uid){
	//
	const UnitDef* ud = G->cb->GetUnitDef(uid);
	if(ud == 0){
		return weak_ptr<CUnitTypeData>();
	}else{
		return this->GetUnitTypeDataById(ud->id);
	}
}

weak_ptr<CUnitTypeData> CUnitDefLoader::GetUnitTypeDataById(int id){
	//
	if((id <0)||(id > unum)){
		return weak_ptr<CUnitTypeData>();
	}else{
		return this->type_info[id];
	}
}

weak_ptr<CUnitTypeData> CUnitDefLoader::GetUnitTypeDataByName(string name){

	//

	int id = GetIdByName(name);
	return this->GetUnitTypeDataById(id);


}

int CUnitDefLoader::GetIdByName(string name){
	//
	string n = name;
	trim(n);
	tolowercase(n);
	if(defs.find(n) != defs.end()){
		return defs[n];
	}else{
		return -1;
	}
}

bool CUnitDefLoader::HasUnit(string name){
	//
	string n = name;
	trim(n);
	tolowercase(n);
	return (defs.find(n) != defs.end());
}
