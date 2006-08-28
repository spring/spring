// CManufacturer
#include "../Core/helper.h"

float3 CManufacturer::GetBuildPos(int builder, const UnitDef* target, float3 unitpos){
	NLOG("CManufacturer::GetBuildPos");
	// Find the least congested sector
	// skew the new position around the centre of the sector
	// issue a new call to closestbuildsite()
	// push the unitpos out so that the commander does not try to build on top of itself leading to stall (especially important for something like nanoblobz)
	if(G->Map->CheckFloat3(unitpos) == false) return unitpos;
	if(builder < 1) return unitpos;
	if (target == 0) return unitpos;

	float3 epos = unitpos;
	epos.z -= max(target->ysize*14,target->xsize*14);
	epos.z -= target->buildDistance*0.6f;
	srand(uint(time(NULL)+G->Cached->randadd+G->Cached->team));
	G->Cached->randadd++;
	float angle = float(rand()%320);
	unitpos = G->Map->Rotate(epos,angle,unitpos);
	// move the new position if it's off map
	if(unitpos.x > G->cb->GetMapWidth() * SQUARE_SIZE){
		unitpos.x = (G->cb->GetMapWidth() * SQUARE_SIZE)-(unitpos.x-(G->cb->GetMapWidth() * SQUARE_SIZE));
		unitpos.x -= 500;
	}
	if(unitpos.z > G->cb->GetMapHeight() * SQUARE_SIZE){
		unitpos.z = (G->cb->GetMapHeight() * SQUARE_SIZE)-(unitpos.z-(G->cb->GetMapHeight() * SQUARE_SIZE));
		unitpos.z -= 500;
	}
	if(unitpos.x < 0){
		unitpos.x *= (-1);
		unitpos.x += 500;
	}
	if(unitpos.z <0){
		unitpos.z *= (-1);
		unitpos.z += 500;
	}
	int wc = G->Map->WhichCorner(unitpos);
	if(wc == t_NE){
		unitpos.z += 40;
		unitpos.x -= 40;
	}
	if(wc == t_NW){
		unitpos.z += 40;
		unitpos.x += 40;
	}
	if(wc == t_SE){
		unitpos.z -= 40;
		unitpos.x -= 40;
	}
	if(wc == t_SW){
		unitpos.z -= 40;
		unitpos.x += 40;
	}
	return unitpos;
}

bool CManufacturer::TaskCycle(CBuilder* i){
	NLOG("CManufacturer::TaskCycle");
	if(i->tasks.empty() == true){
		if(i->GetRepeat()==true){
			G->L.print("CManufacturer::TaskCycle, buildtreecomeptled and now being reloaded!");
			if(LoadBuildTree(i)){
				G->L.print("loaded build tree for " + i->GetUnitDef()->name);
			}else{
				G->L.print("failed loading of build tree for " + i->GetUnitDef()->name);
			}
		}else{
			G->L.print("CManufacturer::TaskCycle, cycle has no tasks!!!");
			return false;
		}
	}
	if(i->tasks.empty() == true){ // STILL EMPTY!!!
		return false;
	}
	// ok first task
	vector<Task>::iterator t = i->tasks.begin();
	G->L.print("CManufacturer::TaskCycle executing task :: " + GetTaskName(t->type));
	if(t->execute(i->GetID())==false){
		G->L.print("CManufacturer::TaskCycle Task returned false");
		t->valid = false;
		i->tasks.erase(t);
		return false;
	}else{
		G->L.print("CManufacturer::TaskCycle Task executed");
		btype j = t->type;
		i->tasks.erase(t);
		if(j == B_RULE_EXTREME_CARRY){
			Task t (G, B_RULE_EXTREME_CARRY);
			if(t.valid == true){
				i->tasks.insert(i->tasks.begin(),t);
			}
		}
		if(j == B_GUARDIAN){
			Task t (G, B_GUARDIAN);
			if(t.valid == true){
				i->tasks.insert(i->tasks.begin(),t);
			}
		}
		return true;
	}
}
float3 CManufacturer::GetBuildPlacement(int unit,float3 unitpos,const UnitDef* ud, int spacing){
	float3 pos = unitpos;
	if(ud->type == string("MetalExtractor")){
		pos = G->M->getNearestPatch(unitpos,0.7f,ud->extractsMetal,ud);
		if(G->Map->CheckFloat3(pos) == false){
			G->L.print("zero mex co-ordinates intercepted");
			return UpVector;
		}
		return pos;
	} else if(ud->isFeature==true){ // dragon teeth for dragon teeth rings
		pos = G->DTHandler->GetDTBuildSite(unit);
		if(G->Map->CheckFloat3(pos) == false){
			G->L.print("zero DT co-ordinates intercepted");
			return UpVector;
		}
		return pos;
	} else if((ud->type == string("Building"))&&(ud->builder == false)&&(ud->weapons.empty() == true)&&(ud->radarRadius > 100)){ // Radar!
		//pos = G->DTHandler->GetDTBuildSite(unit);
		pos = G->RadarHandler->NextSite(unit,ud,1200);
		if(G->Map->CheckFloat3(pos) == false){
			G->L.print("zero radar placement co-ordinates intercepted");
			return UpVector;
		}
		return pos;
	}else{
		//pos = GetBuildPos(unit,ud,pos);
		pos = G->cb->ClosestBuildSite(ud,unitpos,float((4000+(G->cb->GetCurrentFrame()%940))),spacing);
		//pos = G->Map->ClosestBuildSite(ud,pos,float((4000+(G->cb->GetCurrentFrame()%940))),spacing);
		if(G->Map->CheckFloat3(pos) == false){
			//pos = GetBuildPos(unit,ud,unitpos);
			pos = G->cb->ClosestBuildSite(ud,pos,9000.0f,int(spacing*0.8f));
			//pos = G->Map->ClosestBuildSite(ud,pos,9000.0f,spacing);
			int* en = new int[100];
			int e = G->cb->GetFriendlyUnits(en,pos,10.0f);
			if(e >0){
				const UnitDef* ed = G->GetUnitDef(en[0]);
				if(ed != 0){
					if(ed->type == string("Factory")){
						return UpVector;
					}
				}
			}
			if(G->Map->CheckFloat3(pos) == false){
				G->L.print("bad vector returned for " + ud->name);
				return UpVector;
			}
		}
		//if(G->cb->CanBuildAt(ud,pos)==false){
		//	G->L.print("bad result for CanBuildAt returned for " + ud->name);
		//	return UpVector;
		//}
		return pos;
	}
	return pos;
}
bool CManufacturer::CBuild(string name, int unit, int spacing){
	NLOG("CManufacturer::CBuild");
	G->L.print("CBuild() :: " + name);
	const UnitDef* uda = G->GetUnitDef(unit);
	const UnitDef* ud = G->GetUnitDef(name);

	/////////////

	if(ud == 0){
		G->L.print("Factor::CBuild(3) error: a problem occurred loading this units unit definition : " + name);
		return false;
	} else if(uda == 0){
		G->L.print("Factor::CBuild(3) error: a problem occurred loading uda ");
		return false;
	}
	if(G->Pl->AlwaysAntiStall.empty() == false){ // Sort out the stuff that's setup to ALWAYS be under the antistall algorithm
		for(vector<string>::iterator i = G->Pl->AlwaysAntiStall.begin(); i != G->Pl->AlwaysAntiStall.end(); ++i){
			if(*i == name){
				if(G->Pl->feasable(ud,uda) == false){
					G->L.print("Factor::CBuild  unfeasable " + name);
					return false;
				}else{
					break;
				}
			}
		}
	}
	float emax=1000000000;
	string key = "Resource\\MaxEnergy\\";
	key += name;
	/*G->Get_mod_tdf()->GetDef(emax,"3000000",key);// +300k energy per tick by default
	if(G->Pl->GetEnergyIncome() > emax){
	G->L.print("Factor::CBuild  emax " + name);
	return G->Actions->RepairNearby(unit,1000);
	}*/
	// Now sort out stuff that can only be built one at a time
	if(G->Cached->solobuilds.find(name)!= G->Cached->solobuilds.end()){
		G->L.print("Factor::CBuild  solobuild " + name);
		return G->Actions->Repair(unit,G->Cached->solobuilds[name]);// One is already being built, change to a repair order to go help it!
	}
	// Now sort out if it's one of those things that can only be built once
	if(G->Cached->singlebuilds.find(name) != G->Cached->singlebuilds.end()){
		if(G->Cached->singlebuilds[name] == true){
			G->L.print("Factor::CBuild  singlebuild " + name);
			return false;
		}
	}
	//if(G->Pl->feasable(name,unit)==false){
	//	return false;
	//}
	// If Antistall == 2/3 then we always do this
	if(G->info->antistall>1){
		if(G->Pl->feasable(ud,uda) == false){
			G->L.print("Factor::CBuild  unfeasable " + name);
			return false;
		}
	}
	if(G->cb->GetUnitHealth(unit)<1){
		G->L.print("Factor::CBuild(2)unit finished does not exist?! Could it have been destroyed? CBuild()");		
		return false;
	}


	float3 pos=UpVector;
	TCommand TCS(tc,"CBuild");
	tc.unit = unit;
	tc.ID(-ud->id);

	float3 unitpos = G->GetUnitPos(unit);
	if(G->Map->CheckFloat3(unitpos) == false){
		return false;
	}
	float rmax;
	key = "Resource\\ConstructionRepairRanges\\";
	key += name;
	G->Get_mod_tdf()->GetDef(rmax,"5",key);// by default dont do this so we give ti a tiny default range
	if(rmax > 10){
		int* funits = new int[5000];
		int fnum = G->cb->GetFriendlyUnits(funits,unitpos,rmax);
		if(fnum > 1){
			//
			for(int i = 0; i < fnum; i++){
				const UnitDef* udf = G->GetUnitDef(funits[i]);
				if(udf == 0) continue;
				if(udf->name == name){
					if(G->cb->UnitBeingBuilt(funits[i])==true){
						delete [] funits;
						return G->Actions->Repair(unit,funits[i]);
					}
				}
			}
		}
		delete [] funits;
	}
	//if(ud->type != string("MetalExtractor")){
	//	unitpos = GetBuildPos(unit,ud,unitpos);
	//}

	pos = GetBuildPlacement(unit,unitpos,ud,spacing);
	if(G->Map->CheckFloat3(pos)==false){
		G->L.print("GetBuildPlacement returned UpVector or some other nasty position");
		return false;
	}
	pos.y = G->cb->GetElevation(pos.x,pos.z);
	if(G->cb->CanBuildAt(ud,pos) == false){
		G->L.print("GetBuildPlacement returned UpVector or some other nasty position which didnt check out with CanBuildAt()");
		return false;
	}
	tc.PushFloat3(pos);
	tc.Push(0);
	tc.c.timeOut = int(ud->buildTime/5) + G->cb->GetCurrentFrame();
	tc.created = G->cb->GetCurrentFrame();
	tc.delay=0;
	if(G->OrderRouter->GiveOrder(tc)== false){
		G->L.print("Error::Cbuild Failed Order :: " + name);
		return false;
	}else{
		// create a plan
		CBPlan Bplan;
		Bplan.started = false;
		Bplan.builder = unit;
		Bplan.subject = 0;
		Bplan.pos = pos;
		if(ud->movedata != 0){
			Bplan.mobile_mobile = true;
		}else{
			if(ud->canfly == true){
				Bplan.mobile_mobile = true;
			}else{
				Bplan.mobile_mobile = false;
			}
		}
		BPlans.push_back(Bplan);
		return true;
	}
	return false;
}

CManufacturer::CManufacturer(Global* GL){
	G = GL;
	initialized = false;
}

CManufacturer::~CManufacturer(){
	G = 0;
}

void CManufacturer::Init(){
	NLOG("CManufacturer::Init");
	NLOG("Loading MetaTags");
	if(G->Get_mod_tdf()->SectionExist("tags")==true){
		const map<string, string> section = G->Get_mod_tdf()->GetAllValues("tags");
		if(section.empty()==false){
			//
			G->L << "Meta Tags :::" <<endline;
			for(map<string,string>::const_iterator j = section.begin(); j != section.end(); ++j){
				vector<string> vh;
				string f,se;
				f = j->first;
				trim(f);
				tolowercase(f);
				se= j->second;
				trim(se);
				tolowercase(se);
				vh = bds::set_cont(vh,se);
				if(vh.empty()==false){
					metatags[f]= vh;
					G->L << "Meta tag :: " << f << " :: " << se <<endline;
				}else{
					G->L << "Meta tag empty?? :: " << f << " :: " << se <<endline;
				}
			}
		}
	}else{
		G->L << "No MetaTags where defined" << endline;
	}
	/*if(contents != string("")){
	vector<string> v;
	v = bds::set_cont(v,contents.c_str());
	if(v.empty() == false){
	for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
	string sdg = "TAGS\\";
	sdg += *vi;

	if(vh.empty()==false){
	metatags[*vi]= vh;
	}
	}
	}
	}*/
	NLOG("Registering TaskTypes");
	RegisterTaskTypes();
	//NLOG("Loading GlobalBuildTree");
	//LoadGlobalTree();
	initialized = true;
	NLOG("Manufacturer Init Done");
	//	if(G->L.FirstInstance() == true){
	//
	//		G->DrawTGA("logo.tga",ZeroVector);
	//	}
}

void CManufacturer::UnitCreated(int uid){
	NLOG("CManufacturer::UnitCreated");
	// sort out the plans
	// update congestion matrix
	if(BPlans.empty() == false){
		//
		float3 upos = G->GetUnitPos(uid);
		if(upos != UpVector){
			//
			for(vector<CBPlan>::iterator i = BPlans.begin(); i != BPlans.end();++i){
				if(i->pos == upos){
					i->subject = uid;
					i->started = true;
					break;
				}
			}
		}
	}
	G->RadarHandler->Change(uid,true);
}

void CManufacturer::UnitFinished(int uid){
	NLOG("CManufacturer::UnitFinished");
	if(initialized == false){
		G->L.print("CManufacturer::UnitFinished Initialized == false");
		return;
	}else{
		//
		G->L.print("CManufacturer::UnitFinished");
	}
	if(BPlans.empty() == false){
		//
		for(vector<CBPlan>::iterator i = BPlans.begin(); i != BPlans.end(); ++i){
			//
			if(i->subject == uid){
				if(i->mobile_mobile == true){
					G->Actions->ScheduleIdle(uid);
				}
				BPlans.erase(i);
				break;
			}
		}
	}
	// create a new CBuilder struct
	CBuilder b(G,uid);
	if(b.IsValid() == true){
		const UnitDef* ud = b.GetUnitDef();
		if(ud != 0){
			//
			if(ud->builder == true){
				bool setback = false;
				b.SetRepeat(true);
				if(ud->movedata == 0){
					if(ud->canfly == false){
						if(ud->buildOptions.empty() == false){
							b.SetRole(R_FACTORY); // Factory!
							setback = true;
						}else{
							b.SetRole(R_BUILDER); // nano tower type thingymajig!
							setback = true;
						}
					}else{
						b.SetRole(R_BUILDER); // Aircraft!!
					}
				}else{
					b.SetRole(R_BUILDER); // any other construction unit
				}
				G->L.print("CManufacturer::UnitFinished role set");
				LoadBuildTree(&b);
				G->L.print("CManufacturer::UnitFinished buildtree loaded");
				builders.push_back(b);
				G->L.print("CManufacturer::UnitFinished builder pushed back");
				//	if (setback == true){
				if(G->cb->GetCurrentFrame() > 10) G->Actions->ScheduleIdle(uid);
				//	}
			}
		}
	}else{
		G->L.print("cbuilder->valid() == false");
	}
}

void CManufacturer::UnitIdle(int uid){
	NLOG("CManufacturer::UnitIdle");
	if(builders.empty() == false){
		bool idle = false;
		for(vector<CBuilder>::iterator i = builders.begin(); i != builders.end(); ++i){
			if(i->GetID() == uid){
				/*if(G->Actions->DGunNearby(uid)==true){
				idle = false;
				break;
				}*/
				G->L.print("CManufacturer::TaskCycle has found the unitID, issuing TaskCycle");
				bool result = TaskCycle(&*i);
				if(result == false){
					G->L.print("CManufacturer::TaskCycle Attempting to schedule an idle...");
					idle = true;
					break;
				}else{
					G->L.print("CManufacturer::TaskCycle Succesful, exiting loop");
					idle=false;
					break;
				}
			}
		}
		if(idle == true){
			G->Actions->ScheduleIdle(uid);
		}
		return;
	}else{
		//
		G->L.print("CManufacturer::TaskCycle unitID has no builders!!!! aborting");
		return;
	}
}

void CManufacturer::UnitDestroyed(int uid){
	NLOG("CManufacturer::UnitDestroyed");
	//get rid of CBuilder, destroy plans needing this unit
	if(BPlans.empty() == false){
		for(vector<CBPlan>::iterator i = BPlans.begin(); i != BPlans.end(); ++i){
			if(i->builder == uid){
				BPlans.erase(i);
				break;
			}else if(i->subject == uid){
				BPlans.erase(i);
				break;
			}
		}
	}
	G->RadarHandler->Change(uid,false);
}

void CManufacturer::Update(){
	NLOG("CManufacturer::Update");
	if(G->cb->GetCurrentFrame() == 30){ // kick start the commander and any other starting units
		if(builders.empty() == false){
			for(vector<CBuilder>::iterator i = builders.begin();i != builders.end(); ++i){
				UnitIdle(i->GetID());
			}
		}
	}
}

void CManufacturer::EnemyDestroyed(int eid){
	NLOG("CManufacturer::EnemyDestroyed");
}

/*bool CManufacturer::FBuild(string name, int unit, int quantity){
NLOG("CManufacturer::FBuild");
if(G->GetUnitDef(unit) == 0) return false;
if(G->cb->GetUnitHealth(unit)<1){
G->L.print("FBuild() unit finished doesnt exist?! Could it have been destroyed? :: " + name);	
return false;
}
const UnitDef* ud = G->GetUnitDef(name);
if(ud == 0){
G->L.print("error: a problem occurred loading this units unit def : " + name);
return false;
} else {
TCommand TCS(tc,"FBuild");
tc.c.id = -ud->id;
G->L.print("issuing mobile unit :: " + name);
tc.PushFloat3(UpVector);
//tc.Push(1);
tc.delay = 2 SECONDS;
tc.c.timeOut = int(ud->buildTime*4) + G->cb->GetCurrentFrame() + tc.delay;
tc.created = G->cb->GetCurrentFrame();
tc.unit = unit;
tc.type = B_CMD;
tc.clear = false;
tc.Priority = tc_pseudoinstant;
//if(tc.c.params.empty() == false){
for(int q=1;q==quantity;q++){
G->OrderRouter->GiveOrder(tc);
}
return true;
//}else{
//	return false;
//}
}
}*/
void CManufacturer::RegisterTaskPair(string name, btype type){
	types[name] = type;
	typenames[type] = name;
}
void CManufacturer::RegisterTaskTypes(){
	NLOG("CManufacturer::RegisterTaskTypes");
	if(types.empty()==true){
		RegisterTaskPair("b_metatag_failed", B_METAFAILED);
		RegisterTaskPair("b_solar", B_POWER);
		RegisterTaskPair("b_power",B_POWER);
		RegisterTaskPair("b_mex", B_MEX);
		RegisterTaskPair("b_rand_assault", B_RAND_ASSAULT);
		RegisterTaskPair("b_assault", B_ASSAULT);
		RegisterTaskPair("b_factory_cheap", B_FACTORY_CHEAP);
		RegisterTaskPair("b_factory", B_FACTORY);
		RegisterTaskPair("b_builder", B_BUILDER);
		RegisterTaskPair("b_geo", B_GEO);
		RegisterTaskPair("b_scout", B_SCOUT);
		RegisterTaskPair("b_random", B_RANDOM);
		RegisterTaskPair("b_defence", B_DEFENCE);
		RegisterTaskPair("b_defense", B_DEFENCE);
		RegisterTaskPair("b_radar", B_RADAR);
		RegisterTaskPair("b_estore", B_ESTORE);
		RegisterTaskPair("b_targ", B_TARG);
		RegisterTaskPair("b_mstore", B_MSTORE);
		RegisterTaskPair("b_silo", B_SILO);
		RegisterTaskPair("b_jammer", B_JAMMER);
		RegisterTaskPair("b_sonar", B_SONAR);
		RegisterTaskPair("b_antimissile", B_ANTIMISSILE);
		RegisterTaskPair("b_artillery", B_ARTILLERY);
		RegisterTaskPair("b_focal_mine", B_FOCAL_MINE);
		RegisterTaskPair("b_sub", B_SUB);
		RegisterTaskPair("b_amphib", B_AMPHIB);
		RegisterTaskPair("b_mine", B_MINE);
		RegisterTaskPair("b_carrier", B_CARRIER);
		RegisterTaskPair("b_metal_maker", B_METAL_MAKER);
		RegisterTaskPair("b_fortification", B_FORTIFICATION);
		RegisterTaskPair("b_dgun", B_DGUN);
		RegisterTaskPair("b_repair", B_REPAIR);
		RegisterTaskPair("repair", B_REPAIR);
		RegisterTaskPair("reclaim", B_RECLAIM);
		RegisterTaskPair("retreat", B_RETREAT);
		RegisterTaskPair("b_idle", B_IDLE);
		RegisterTaskPair("b_cmd", B_CMD);
		RegisterTaskPair("b_randmove", B_RANDMOVE);
		RegisterTaskPair("b_resurrect", B_RESURECT);
		RegisterTaskPair("b_ressurect", B_RESURECT);
		RegisterTaskPair("b_resurect", B_RESURECT);
		RegisterTaskPair("b_global", B_RULE);
		RegisterTaskPair("b_rule", B_RULE);
		RegisterTaskPair("b_rule_extreme", B_RULE_EXTREME);
		RegisterTaskPair("b_rule_extreme_nofact", B_RULE_EXTREME_NOFACT);
		RegisterTaskPair("b_rule_extreme_carry", B_RULE_EXTREME_CARRY);
		RegisterTaskPair("b_rule_carry", B_RULE_EXTREME_CARRY);
		RegisterTaskPair("b_retreat", B_RETREAT);
		RegisterTaskPair("b_guardian", B_GUARDIAN);
		RegisterTaskPair("b_bomber", B_BOMBER);
		RegisterTaskPair("b_gunship", B_GUNSHIP);
		RegisterTaskPair("b_plasma_repulsor", B_SHIELD);
		RegisterTaskPair("b_plasma_repulser", B_SHIELD);
		RegisterTaskPair("b_shield", B_SHIELD);
		RegisterTaskPair("b_missile_unit", B_MISSILE_UNIT);
		RegisterTaskPair("b_na", B_NA);
		RegisterTaskPair("b_fighter", B_FIGHTER);
		RegisterTaskPair("b_guard_factory", B_GUARD_FACTORY);
		RegisterTaskPair("b_guard_like_con", B_GUARD_LIKE_CON);
		RegisterTaskPair("b_reclaim", B_RECLAIM);
		RegisterTaskPair("b_wait", B_WAIT);
		//,
		//	,
		//	RegisterTaskPair("")] = ;
	}
}
btype CManufacturer::GetTaskType(string s){
	NLOG("CManufacturer::GetTaskType");
	if(types.empty()==false){
		if(types.find(s) != types.end()){
			return types[s];
		}
	}
	return B_NA;
}

string CManufacturer::GetTaskName(btype type){
	NLOG("CManufacturer::GetTaskName");
	if(typenames.empty()==false){
		if(typenames.find(type) != typenames.end()){
			return typenames[type];
		}
	}
	return "";
}

void CManufacturer::LoadGlobalTree(){
	NLOG("CManufacturer::LoadGlobalTree");
	return;
	string filename;
	string buffer = "";
	/* if abstract = true and we're not to use mod specific buidler.txt etc then load from the default folder
	*/if((G->info->abstract == true)&&(G->info->use_modabstracts == false)){ 
	filename = G->info->datapath + string(slash) + "Default" + string(slash) + string("global.txt");
	if(G->ReadFile(filename,&buffer) == false){
		G->L.print("error :: " + filename + " could not be opened!!! (1)" );
		return;
	}
	}else if(G->info->use_modabstracts == true){
		filename = G->info->datapath + string(slash) + G->info->tdfpath + string(slash)+string("global.txt");
		if(G->ReadFile(filename,&buffer) == false){
			G->L.print("error :: " + filename + " could not be opened!!! (2)");
			return;
		}	
	}else {
		filename = G->info->datapath + string(slash) + G->info->tdfpath + string(slash) + G->cb->GetMapName() + string(slash) + string("global.txt");
		// map specific doesnt exist, load mod specific instead
		if(G->ReadFile(filename,&buffer) == false){
			filename = G->info->datapath + string(slash) + G->info->tdfpath + string(slash) + string("global.txt");
			if(G->ReadFile(filename,&buffer) == false){
				// absent_abstract is true so we'll load the builder.txt etc now that the definite data cannot be loaded
				if(G->info->absent_abstract == true){
					filename = G->info->datapath + string(slash) + G->info->tdfpath + slash +string("global.txt");
					if(G->ReadFile(filename,&buffer) == false){
						G->L.print("error :: " + filename + " could not be opened!!! (3)" );
						return;
					}
				}else{
					return;
				}
			}
		}
	}
	vector<string> v;
	string s = buffer;
	if(s.empty() == true){
		G->L.print(" error loading contents of  file :: " + filename + " :: buffer empty, most likely because of an empty file");
		return;
	}
	v = bds::set_cont(v,s.c_str());
	if(v.empty() == false){
		for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
			if(metatags.empty() ==false){
				if (metatags.find(*vi) != metatags.end()){
					Task t(G,true,*vi);
					t.metatag = true;
					if(t.valid == true){
						Global_queue.push_back(t);
						if(v.size() == 1) Global_queue.push_back(t);
						continue;
					}
					continue;
				}
			}
			const UnitDef* uj = G->GetUnitDef(*vi);
			if(uj != 0){
				Task t(G,false,*vi);
				if(t.valid == true){
					Global_queue.push_back(t);
				}
				if(v.size() == 1) Global_queue.push_back(t);
			}  else if(*vi == string("repair")){
				Task t (G, B_REPAIR);
				if(t.valid == true){
					Global_queue.push_back(t);
					if(v.size() == 1) Global_queue.push_back(t);
				}
			}else if(*vi == string("")){
				continue;
			} else{
				btype x = GetTaskType(*vi);
				if(x != B_NA){
					Task t (G, x);
					if(t.valid == true){
						Global_queue.push_back(t);
						if(v.size() == 1) Global_queue.push_back(t);
					}
				}
				G->L.iprint("error :: a value :: " + *vi +" :: was parsed in :: "+filename + " :: this does not have a valid UnitDef according to the engine, and is not a Task keyword such as repair or B_MEX");
			}
		}
	} else{
		G->L.print(" error loading contents of  file :: " + filename + " :: buffer empty, most likely because of an empty file");
	}
}

bool CManufacturer::CanBuild(int uid, const UnitDef* ud, string name){
	NLOG("CManufacturer::CanBuild");
	string n = name;
	tolowercase(n);
	trim(n);
	if(ud == 0){
		G->L <<"CManufacturer::CanBuild(" <<uid << ",const UnitDef* ud," <<  name << ") gave false because ud == 0" << endline;
		return false;
	}
	if(ud->buildOptions.empty()){
		G->L << "CManufacturer::CanBuild(" <<uid << ",const UnitDef* ud," <<  name << ") gave false because ud->buildOptions.empty() == true" << endline;
		return false;
	}else{
		for(map<int,string>::const_iterator i = ud->buildOptions.begin(); i !=ud->buildOptions.end(); ++i){
			string k = i->second;
			tolowercase(k);
			trim(k);
			if(k==n){
				return true;
			}
		}
		return false;
	}
}

string CManufacturer::GetBuild(int uid, string tag, bool efficiency){
	NLOG("CManufacturer::GetBuild");
	G->L << "CManufacturer::GetBuild("<<uid<<","<<tag<<","<< efficiency<<")"<<endline;
	if(metatags.empty()){
		return string("b_metatag_failed");
	}
	const UnitDef* ud = G->GetUnitDef(uid);
	if(ud == 0){
		G->L.print("GetBuild metatag error with Builder not getting a unit def");
		return string("b_metatag_failed");
	}else{
		map<string, vector<string> >::const_iterator ik = metatags.find(tag);
		if(ik == metatags.end()){
			G->L.print("Meta tag error in GetBuild ik == metatags.end() for "+tag);
			return string("b_metatag_failed");
		}
		vector<string> v = ik->second;
		float best_score = 0;
		string best = "b_metatag_failed";
		if(v.empty() == false){
			for(vector<string>::const_iterator is = v.begin(); is != v.end();++is){
				string w = *is;
				trim(w);
				if(w == string("")) continue;
				if(CanBuild(uid,ud,w)==false){
					G->L.print("hmm canbuild() == false for \""+w+"\" on the "+ud->name);
					continue;
				}else{
					float temp = G->GetEfficiency(w);
					G->L << "canbuild \""<< w << "\" efficiency == "<< temp<< endline;
					if(temp > best_score){
						best_score = temp;
						best = w;
					}
				}
			}
			G->L << "returning \"" << best << "\""<< endline;
			return best;
		}else{
			return "b_metatag_failed";
		}
	}
}

bool CManufacturer::LoadBuildTree(CBuilder* ui){
	NLOG("CManufacturer::LoadBuildTree");
	const UnitDef* ud = ui->GetUnitDef();
	G->L.print("loading buildtree for "+ud->name);
	// get the list of filenames
	vector<string> vl;
	string sl = G->Get_mod_tdf()->SGetValueMSG(string("TASKLISTS\\")+ud->name);
	tolowercase(sl);
	trim(sl);
	string u = ud->name;
	if(sl != string("")){
		vl = bds::set_cont(vl,sl.c_str());
		if(vl.empty() == false){
			srand(uint(time(NULL) +G->Cached->team +  G->Cached->randadd));
			G->Cached->randadd++;
			int randnum = rand()%vl.size();
			u = vl.at(min(randnum,max(int(vl.size()-1),1)));
		}
	}
	string filename;
	string* buffer= new string("");
	/* if abstract = true and we're not to use mod specific builder.txt etc then load from the default folder
	*/
	if((G->info->abstract == true)&&(G->info->use_modabstracts == false)){
		filename = G->info->datapath + string(slash) + "Default" + string(slash);
		if(ud->isCommander == true){
			filename += "commander.txt";
		}else if(ui->GetRole() == R_FACTORY){
			filename += "factory.txt";
		}else{
			filename += "builder.txt";
		}
		if(G->ReadFile(filename,buffer) == false){
			G->L.print("error :: " + filename + " could not be opened!!! (1)" );
			return false;
		}
	}else if(G->info->use_modabstracts == true){ /*Otherwise is mod_abstracts = true then load from datapath\tdfpath\builder.txt etc*/
		filename = G->info->datapath + string(slash) + G->info->tdfpath + string(slash);
		if(ud->isCommander == true){
			filename += "commander.txt";
		}else if(ui->GetRole() == R_FACTORY){
			filename += "factory.txt";
		}else{
			filename += "builder.txt";
		}
		if(G->ReadFile(filename,buffer) == false){
			G->L.print("error :: " + filename + " could not be opened!!! (2)" );
			return false;
		}
	}else {/* load map specific*/
		tolowercase(u);
		filename = G->info->datapath + string(slash) + G->info->tdfpath + string(slash) + G->cb->GetMapName() + string(slash) + u + string(".txt");
		G->L.print("loading buildtree from "+filename);
		// map specific doesnt exist, load mod specific instead
		if(G->ReadFile(filename,buffer) == false){
			if(G->Cached->cheating == true){
				filename = G->info->datapath + string(slash) + G->info->tdfpath + string(slash) + string("cheat") + string(slash) +  u + string(".txt");
				if(G->ReadFile(filename,buffer) == false){
					filename = G->info->datapath + string(slash) + G->info->tdfpath + string(slash) + u + string(".txt");
					if(G->ReadFile(filename,buffer) == false){
						// absent_abstract is true so we'll load the builder.txt etc now that the definite data cannot be loaded
						if(G->info->absent_abstract == true){
							filename = G->info->datapath + string(slash) + G->info->tdfpath + slash;
							if(ud->isCommander == true){
								filename += "commander.txt";
							}else if(ui->GetRole() == R_FACTORY){
								filename += "factory.txt";
							}else{
								filename += "builder.txt";
							}
							if(G->ReadFile(filename,buffer) == false){
								G->L.print("error :: " + filename + " could not be opened!!! (3)" );
								return false;
							}
						}else{
							return false;
						}
					}
				}
			}else{
				filename = G->info->datapath + string(slash) + G->info->tdfpath + string(slash) + u + string(".txt");
				if(G->ReadFile(filename,buffer) == false){
					// absent_abstract is true so we'll load the builder.txt etc now that the definite data cannot be loaded
					if(G->info->absent_abstract == true){
						filename = G->info->datapath + string(slash) + G->info->tdfpath + slash;
						if(ud->isCommander == true){
							filename += "commander.txt";
						}else if(ui->GetRole() == R_FACTORY){
							filename += "factory.txt";
						}else{
							filename += "builder.txt";
						}
						if(G->ReadFile(filename,buffer) == false){
							G->L.print("error :: " + filename + " could not be opened!!! (3)");
							return false;
						}
					}else{
						return false;
					}
				}
			}
		}
	}
	vector<string> v;
	string s = *buffer;
	if(s.empty() == true){
		G->L.print(" error loading contents of  file :: " + filename + " :: buffer empty, most likely because of an empty file");
		return false;
	}
	G->L.print("loaded contents of  file :: " + filename + " :: filling buildtree");
	tolowercase(s);
	trim(s);
	v = bds::set_cont(v,s.c_str());
	if(v.empty() == false){
		bool polate=false;
		bool polation = G->info->rule_extreme_interpolate;
		btype bt = GetTaskType(G->Get_mod_tdf()->SGetValueDef("b_rule_extreme_nofact","AI\\interpolate_tag"));
		if(ui->GetRole() == R_FACTORY){
			polation = false;
		}
		if((ui->GetUnitDef()->canfly ||ui->GetUnitDef()->movedata) &&  (ui->GetAge() < (5 SECONDS))){
			G->Actions->ScheduleIdle(ui->GetID());
			return true;
		}
		for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
			if(polation==true){
				if(polate==true){
					ui->AddTask(bt);
					polate = false;
				}else{
					polate=true;
				}
			}
			string q = *vi;
			trim(q);
			tolowercase(q);
			if(metatags.empty() ==false){
				if (metatags.find(q) != metatags.end()){
					string s = "";
					s = GetBuild(ui->GetID(),q,true);
					if((s != q)&&(s != string(""))&&(s != string("b_metatag_failed"))){
						ui->AddTask(s,false);
					}else{
						G->L.print(" error with metatag!!!!!!! ::  "+ s + " :: " + *vi);
					}
					continue;
				}
			}
			const UnitDef* uj = G->GetUnitDef(*vi);
			if(uj != 0){
				ui->AddTask(q,false);
				if(v.size() == 1) ui->AddTask(q,false);
			}else if(q == string("")){
				continue;
			} else if(q == string("is_factory")){
				ui->SetRole(R_FACTORY);
			} else if(q == string("is_builder")){
				ui->SetRole(R_BUILDER);
			} else if(q == string("repeat")){
				ui->SetRepeat(true);
			} else if(q == string("no_rule_interpolation")){
				polation=false;
			} else if(q == string("rule_interpolate")){
				polation=true;
			} else if(q == string("dont_repeat")){
				ui->SetRepeat(false);
			}else if(q == string("base_pos")){
				G->Map->base_positions.push_back(G->GetUnitPos(ui->GetID()));
			} else if(q == string("gaia")){
				G->info->gaia = true;
			} else if(q == string("not_gaia")){
				G->info->gaia = false;
			} else if(q == string("switch_gaia")){
				G->info->gaia = !G->info->gaia;
			} else{
				btype x = GetTaskType(q);
				if( x != B_NA){
					ui->AddTask(x);
				}else{
					G->L.iprint("error :: a value :: " + *vi +" :: was parsed in :: "+filename + " :: this does not have a valid UnitDef according to the engine, and is not a Task keyword such as repair or B_MEX");
				}
			}
		}
		if(ud->isCommander == true)	G->Map->basepos = G->GetUnitPos(ui->GetID());
		if((ud->isCommander== true)||(ui->GetRole() == R_FACTORY))	G->Map->base_positions.push_back(G->GetUnitPos(ui->GetID()));
		G->L.print("issuing scheduled idle from factor::loadbuildtree");
		return true;
		//G->Actions->ScheduleIdle(ui->GetID());
	} else{
		G->L.print(" error loading contents of  file :: " + filename + " :: buffer empty, most likely because of an empty file");
		return false;
	}
}

/** unitdef->type
MetalExtractor
Transport
Builder
Factory
Bomber
Fighter
GroundUnit
Building
**/
