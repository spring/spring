/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/
#include "../../Core/include.h"

namespace ntai {

	CUnitDefLoader::CUnitDefLoader(Global* GL){

		// Initialize pointer to the Global class
		G = GL;
		
		// retrieve the number of unit definitions
		unum = G->cb->GetNumUnitDefs();

		// for debugging purposes:
		G->L.print("AI interface says this mod has this many units! :: "+to_string(unum));
		
		// Check if a horrific error has occured
		if(unum < 1){
			// this should never happen!
			G->L.eprint("URGENT! GetNumUnitDefs returned ZERO!! This means that there are no unit definitions of any kind!!! Unrecoverable error!");

			/* A horrible event has occurred somewhere in the spring engine for this to have happened.
			 Even if the AI could recover from this, the engine could not.

			 Exit this method immediatly. A crash is likely but if the crash was fixed, the engine
			 or another AI, would crash afterwards anyway.*/
			return;

			
		}

		// initialize arrays

		// The unitdeflist array will be passed to the engine where it will be filled with pointers
		UnitDefList = new const UnitDef*[unum];

		// retrieve the list of unit definition pointers from the engine
		G->cb->GetUnitDefList(UnitDefList);

		// for each definition
		for(int n=0; n < unum; n++){

			// retrieve the units definition
			const UnitDef* pud = UnitDefList[n];

			// initialize a UnitTypeData object
			CUnitTypeData* cutd = new CUnitTypeData();
			
			// now initialize the newly added object with the unit definition
			cutd->Init(G,pud);

			// add it into the main array
			type_data[pud->id] = cutd;

			// check if the unit definition is zero, if so skip
			//if(pud == 0) continue;

			// make sure the name is in the correct format and add it to the map container
			string na = pud->name;
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

	const UnitDef* CUnitDefLoader::GetUnitDefByIndex(int i) const{
		if(i > unum) return 0;
		if(i <0) return 0;
		return UnitDefList[i];
	}

	const UnitDef* CUnitDefLoader::GetUnitDef(string name) const{
		string n = name;

		trim(n);
		tolowercase(n);

		std::map<string,int>::const_iterator it = defs.find(n);

		if(it != defs.end()){
			const UnitDef* u = GetUnitDefByIndex(it->second);
			return u;
		}
		return 0;
	}

	CUnitTypeData* CUnitDefLoader::GetUnitTypeDataByUnitId(int uid) const{

		if(!ValidUnitID(uid)){
			return 0;
		}

		const UnitDef* ud = G->GetUnitDef(uid);

		if(ud == 0){
			return 0;
		}else{
			return this->GetUnitTypeDataById(ud->id);
		}
	}

	CUnitTypeData* CUnitDefLoader::GetUnitTypeDataById(int id) const{
		//
		if((id <0)||(id > unum)){
			return 0;
		}else{
			std::map<int,CUnitTypeData* >::const_iterator it = type_data.find(id);
			return it->second;
		}
	}

	CUnitTypeData* CUnitDefLoader::GetUnitTypeDataByName(string name) const{
		//
		int id = GetIdByName(name);
		return GetUnitTypeDataById(id);
	}

	int CUnitDefLoader::GetIdByName(string name) const{
		//
		string n = name;
		
		trim(n);
		tolowercase(n);

		std::map<string,int>::const_iterator it = defs.find(n);
		
		if(it != defs.end()){
			return it->second;
		}else{
			return -1;
		}
	}

	bool CUnitDefLoader::HasUnit(string name) const{
		//
		string n = name;

		trim(n);
		tolowercase(n);

		return (defs.find(n) != defs.end());
	}
}
