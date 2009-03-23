#include "../core/include.h"

namespace ntai {

	

	int iterations=0;
	bool loaded=false;
	bool firstload=false;
	bool saved=false;

	CEfficiency::CEfficiency(Global* GL){
		loaded = false;
		firstload = false;
		iterations = 0;
		saved = false;
	}

	CEfficiency::~CEfficiency(){
		//
	}

	void CEfficiency::RecieveMessage(CMessage &message){
		//
	}

	bool CEfficiency::Init(){
		G->L.print("Loading unit data");
		LoadUnitData();
		G->L.print("Unit data loaded");
		return true;
	}


	void CEfficiency::DestroyModule(){
		//
	}

	float CEfficiency::GetEfficiency(string s, float def_value){

		CUnitTypeData* ud = this->G->UnitDefLoader->GetUnitTypeDataByName(s);
		if(ud == 0){
			return def_value;
		}

		if(efficiency.find(ud->GetName()) != efficiency.end()){
			if(ud->CanConstruct()){

				if(builderefficiency.find(ud->GetName()) != builderefficiency.end()){
					int i = lastbuilderefficiencyupdate[ud->GetName()];
					if(G->GetCurrentFrame()-(5 MINUTES) < i){
						return builderefficiency[ud->GetName()];
					}
				}

				float e = efficiency[ud->GetName()];
				
				set<string> alreadydone;
				alreadydone.insert(ud->GetName());

				if(!ud->GetUnitDef()->buildOptions.empty()){
					for(map<int, string>::const_iterator i = ud->GetUnitDef()->buildOptions.begin();i != ud->GetUnitDef()->buildOptions.end(); ++i){
						alreadydone.insert(i->second);
						e += efficiency[s];
					}
				}

				lastbuilderefficiencyupdate[ud->GetName()] = G->GetCurrentFrame();
				builderefficiency[ud->GetName()] = e;

				return e;
			}else{
				return efficiency[ud->GetName()];
			}

		}else{
			G->L.print("error ::   " + ud->GetName() + " is missing from the efficiency array");
			return def_value;
		}

	}

	void CEfficiency::SetEfficiency(std::string s, float e){
		trim(s);
		tolowercase(s);
		efficiency[s] = e;
		//
	}

	float CEfficiency::GetEfficiency(std::string s, std::set<string>& doneconstructors, int techlevel){

		CUnitTypeData* ud = this->G->UnitDefLoader->GetUnitTypeDataByName(s);

		if(ud == 0){
			return 0;
		}

		if(efficiency.find(ud->GetName()) != efficiency.end()){
			if(ud->CanConstruct()&&(doneconstructors.find(ud->GetName())==doneconstructors.end())){

				if(builderefficiency.find(s) != builderefficiency.end()){
					int i = lastbuilderefficiencyupdate[ud->GetName()];
					if(G->GetCurrentFrame()-(5 MINUTES) < i){
						return builderefficiency[ud->GetName()];
					}
				}

				float e = efficiency[ud->GetName()];
				
				doneconstructors.insert(ud->GetName());

				for(map<int, string>::const_iterator i = ud->GetUnitDef()->buildOptions.begin();i != ud->GetUnitDef()->buildOptions.end(); ++i){
					CUnitTypeData* ud2 = G->UnitDefLoader->GetUnitTypeDataByName(i->second);

					if(doneconstructors.find(i->second)==doneconstructors.end()){
						doneconstructors.insert(i->second);

						if (ud2->GetUnitDef()->techLevel<techlevel){
							e+=1.0f;
						} else if (ud2->GetUnitDef()->techLevel != techlevel){
							e+= efficiency[i->second]*pow(0.6f, (ud2->GetUnitDef()->techLevel-techlevel));
						} else {
							e+= efficiency[i->second];
						}
					}else{

						if(ud2->GetUnitDef()->techLevel > ud->GetUnitDef()->techLevel){
							e+= 1.0f;
						}

						e += builderefficiency[i->second];
					}

				}

				lastbuilderefficiencyupdate[ud->GetName()] = G->GetCurrentFrame();
				builderefficiency[ud->GetName()] = e;

				return e;
			}else{
				if(ud->GetUnitDef()->techLevel < techlevel){
					return 1.0f;
				}else  if(ud->GetUnitDef()->techLevel > techlevel){
					return efficiency[ud->GetName()]*pow(0.6f, (ud->GetUnitDef()->techLevel-techlevel));
				}

				return efficiency[ud->GetName()];
			}
		}else{
			G->L.print("error ::   " + ud->GetName() + " is missing from the efficiency array");
			return 0.0f;
		}
	}



	bool CEfficiency::LoadUnitData(){
		if(G->L.FirstInstance()){
			int unum = G->cb->GetNumUnitDefs();
			const UnitDef** ulist = new const UnitDef*[unum];
			G->cb->GetUnitDefList(ulist);
			for(int i = 0; i < unum; i++){
				const UnitDef* pud = ulist[i];

				if(pud == 0){
					continue;
				}

				string eu = pud->name;

				tolowercase(eu);
				trim(eu);

				float ef = pud->energyMake + pud->metalMake;

				if(pud->energyCost < 0){
					ef += -pud->energyCost;
				}

				ef *= 2;

				if(pud->weapons.empty() == false){
					for(vector<UnitDef::UnitDefWeapon>::const_iterator k = pud->weapons.begin();k != pud->weapons.end();++k){
						//ef += k->def->
						float av=0;
						int numTypes;// = cb->getk->def->damages.numTypes;
						
						G->cb->GetValue(AIVAL_NUMDAMAGETYPES, &numTypes);
						for(int a=0;a<numTypes;++a){
							if(a == 0){
								av = k->def->damages[0];//damages
							}else{
								av = (av+k->def->damages[a])/2;
							}
						}
						ef += av;
					}
				}

				ef += pud->power;

				this->efficiency[eu] = ef;
				G->unit_names[eu] = pud->humanName;
				G->unit_descriptions[eu] = pud->tooltip;
			}

			string filename = G->info->datapath;
			filename += slash;
			filename += "learn";
			filename += slash;
			filename += G->info->tdfpath;
			filename += ".tdf";

			string* buffer = new string;

			if(G->ReadFile(filename, buffer)){

				TdfParser cq(G);

				cq.LoadBuffer(buffer->c_str(), buffer->size());
				iterations = atoi(cq.SGetValueDef("1", "AI\\iterations").c_str());

				for(map<string, float>::iterator i = efficiency.begin(); i != efficiency.end(); ++i){
					string s = "AI\\";
					s += i->first;
					float ank = (float)atof(cq.SGetValueDef("14", s.c_str()).c_str());
					if(ank > i->second) i->second = ank;
				}

				iterations = atoi(cq.SGetValueDef("1", "AI\\iterations").c_str());
				iterations++;

				cq.GetDef(firstload, "1", "AI\\firstload");

				if(firstload == true){
					G->L.iprint(" This is the first time this mod has been loaded, up. Take this first game to train NTai up, and be careful of throwing the same units at it over and over again");
					firstload = false;

					for(map<string, float>::iterator i = efficiency.begin(); i != efficiency.end(); ++i){
						CUnitTypeData* uda = G->UnitDefLoader->GetUnitTypeDataByName(i->first);
						if(uda){
							i->second += uda->GetUnitDef()->health;
						}
					}

				}

				loaded = true;
				return true;
			} else{

				for(int i = 0; i < unum; i++){
					float ts = 500;
					if(ulist[i]->weapons.empty()){
						ts += ulist[i]->health+ulist[i]->energyMake + ulist[i]->metalMake + ulist[i]->extractsMetal*50+ulist[i]->tidalGenerator*30 + ulist[i]->windGenerator*30;
						ts *= 300;
					}else{
						ts += 20*ulist[i]->weapons.size();
					}
					string eu = ulist[i]->name;
					tolowercase(eu);
					trim(eu);
					efficiency[eu] = ts;
				}

				SaveUnitData();

				G->L.print("failed to load :" + filename);

				return false;
			}
		}
		return false;
	}

	bool CEfficiency::SaveUnitData(){
		NLOG("Global::SaveUnitData()");

		if(G->L.FirstInstance() == true){
			ofstream off;

			string filename = G->info->datapath;
			filename += slash;
			filename += "learn";
			filename += slash;
			filename += G->info->tdfpath;
			filename += ".tdf";

			off.open(filename.c_str());

			if(off.is_open() == true){
				off << "[AI]" << endl << "{" << endl << "    // " << AI_NAME << " AF :: unit efficiency cache file" << endl << endl;

				off << "    version=XE9.79;" << endl;
				off << "    firstload=" << firstload << ";" << endl;
				off << "    modname=" << G->cb->GetModName() << ";" << endl;
				off << "    iterations=" << iterations << ";" << endl;
				off << endl;

				off << "    [VALUES]" << endl << "    {" << endl;

				for(map<string, float>::const_iterator i = efficiency.begin(); i != efficiency.end(); ++i){
					off << "        "<< i->first << "=" << i->second << ";    // " << G->unit_names[i->first] << " :: "<< G->unit_descriptions[i->first]<<endl;
				}
				off << "    }" << endl;
				off << "    [NAMES]" << endl << "    {"<< endl;

				for(map<string, float>::const_iterator i = efficiency.begin(); i != efficiency.end(); ++i){
					off << "        "<< i->first << "=" << G->unit_names[i->first] << ";" <<endl;
				}

				off << "    }" << endl;

				off << "    [DESCRIPTIONS]" << endl << "    {"<< endl;

				for(map<string, float>::const_iterator i = efficiency.begin(); i != efficiency.end(); ++i){
					off << "        "<< i->first << "=" << G->unit_descriptions[i->first] << ";"<<endl;
				}

				off << "    }" << endl;
				off << "}" << endl;
				off.close();

				saved = true;
				return true;
			}else{
				G->L.print("failed to save :" + filename);
				off.close();
				return false;
			}
		}
		return false;
	}
}