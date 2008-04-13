#include "Unit.h"
#include "GlobalAI.h"

static Command nullParamsCommand;
static Command oneParamsCommand;
static Command threeParamsCommand;
static Command fourParamsCommand;
static int MapWidthTest;
static int MapHeightTest;

CR_BIND(CUNIT, )

CR_REG_METADATA(CUNIT,(
				CR_MEMBER(myid),
				CR_MEMBER(groupID),
				CR_MEMBER(stuckCounter),
				CR_MEMBER(attemptedUnstuck),
				CR_MEMBER(maneuverCounter),
				CR_MEMBER(ai),
				CR_RESERVED(16),
				CR_POSTLOAD(PostLoad)
				));

CUNIT::CUNIT()
{
	this->ai=0;
	//added for attackgroup usage
	this->groupID = 0;
	this->stuckCounter = 0;
	this->attemptedUnstuck = false;
	this->maneuverCounter = 0;
}

CUNIT::CUNIT(AIClasses *ai)
{
	this->ai=ai;
	//added for attackgroup usage
	this->groupID = 0;
	this->stuckCounter = 0;
	this->attemptedUnstuck = false;
	this->maneuverCounter = 0;
	//static Command oneParamsCommand;
	//static Command threeParamsCommand;
	//static Command fourParamsCommand;
	if(oneParamsCommand.params.size() == 0) // Dont assign the static commands more than once.
	{
		MapWidthTest = ai->cb->GetMapWidth() * 8;
		MapHeightTest = ai->cb->GetMapHeight() * 8;

		oneParamsCommand.options = 0;
		oneParamsCommand.params.push_back(0);
		threeParamsCommand.options = 0;
		threeParamsCommand.params.push_back(0);
		threeParamsCommand.params.push_back(0);
		threeParamsCommand.params.push_back(0);
		fourParamsCommand.options = 0;
		fourParamsCommand.params.push_back(0);
		fourParamsCommand.params.push_back(0);
		fourParamsCommand.params.push_back(0);
		fourParamsCommand.params.push_back(0);
	}
}	
CUNIT::~CUNIT()
{
}

void CUNIT::PostLoad()
{
	//static Command oneParamsCommand;
	//static Command threeParamsCommand;
	//static Command fourParamsCommand;
	if(oneParamsCommand.params.size() == 0) // Dont assign the static commands more than once.
	{
		MapWidthTest = ai->cb->GetMapWidth() * 8;
		MapHeightTest = ai->cb->GetMapHeight() * 8;

		oneParamsCommand.options = 0;
		oneParamsCommand.params.push_back(0);
		threeParamsCommand.options = 0;
		threeParamsCommand.params.push_back(0);
		threeParamsCommand.params.push_back(0);
		threeParamsCommand.params.push_back(0);
		fourParamsCommand.options = 0;
		fourParamsCommand.params.push_back(0);
		fourParamsCommand.params.push_back(0);
		fourParamsCommand.params.push_back(0);
		fourParamsCommand.params.push_back(0);
	}
}

const UnitDef* CUNIT::def()
{
	return ai->cb->GetUnitDef(myid);
}

float3 CUNIT::pos(){
	//if(def())
		return ai->cb->GetUnitPos(myid);
	//return float3(-1,0,0);
}

int CUNIT::owner() // 0: mine, 1: allied, 2: enemy -1: non-existant
{
	if(ai->cheat->GetUnitDef(myid)){
		if(def()){
			if(ai->cb->GetUnitTeam(myid) == ai->cb->GetMyTeam()){
				return 0;
			}
			if(ai->cb->GetUnitAllyTeam(myid) == ai->cb->GetMyAllyTeam()){
				return 1;
			}
			return 2;
		}
	}		
	return -1;
}


float CUNIT::Health()
{
	return ai->cb->GetUnitHealth(myid);
}

float CUNIT::MaxHealth()
{
	return ai->cb->GetUnitHealth(myid);
}

int CUNIT::category()
{
	return GCAT(myid);
}

bool CUNIT::CanAttack(int otherUnit)
{
	//currently doesnt do what i want it to do: to see if sending me vs other is a good idea, like peewee vs bomber (not)
	//L("doing CanAttack from " << this->myid << " to " << otherUnit);
	const UnitDef *ud_mine = ai->cb->GetUnitDef(this->myid);
	const UnitDef *ud_other = ai->cheat->GetUnitDef(otherUnit);
	if (ud_mine && ud_other) {
		//L("CanAttack: GUD returned on the second unit: " << ud_other);
		//L("types: " << ud_mine->humanName << " to " << ud_other->humanName);
		//assert(otherUnit != NULL);
		//float CUnitTable::GetDPSvsUnit(const UnitDef* unit,const UnitDef* victim)
		float dps;
//		if (ud_other->canmove) //GetDPSvsUnit crashes on buildings
//		{
			//dps = this->ai->ut->GetDPSvsUnit(ud_mine, ud_other);
			dps = ai->ut->unittypearray[ud_mine ->id].DPSvsUnit[ud_other->id];
//		} else {
//			dps = this->ai->ut->GetDPS(ud_mine);
//		}
		//L("part-result in CUNIT::CanAttack dps: " << dps);
		return dps > 5.0f;
	}
	return false; //might be a false negative
}

bool CUNIT::CanAttackMe(int otherUnit) {
	//currently doesnt do what i want it to do: to see if sending me vs other is a good idea, like peewee vs bomber (not)
	//L("doing CanAttackMe from " << otherUnit << " to " << this->myid);
	const UnitDef *ud_mine = ai->cb->GetUnitDef(this->myid);
	const UnitDef *ud_other = ai->cheat->GetUnitDef(otherUnit);
	if (ud_mine && ud_other) {
		//L("CanAttack: GUD returned on the second unit: " << ud_other);
		//L("types: " << ud_other->humanName << " to " << ud_mine->humanName);
		//assert(otherUnit != NULL);
		//float CUnitTable::GetDPSvsUnit(const UnitDef* unit,const UnitDef* victim)
		float dps;
//		if (ud_other->canmove) //GetDPSvsUnit crashes on buildings
//		{
			//dps = this->ai->ut->GetDPSvsUnit(ud_mine, ud_other);
			dps = ai->ut->unittypearray[ud_other->id].DPSvsUnit[ud_mine->id];
//		} else {
//			dps = this->ai->ut->GetDPS(ud_mine);
//		}
		//L("part-result in CUNIT::CanAttack dps: " << dps);
		return dps > 5.0f;
	}
	return false; //might be a false negative
}

float CUNIT::GetMyDPSvsUnit(int otherUnit)
{
	const UnitDef *ud_mine = ai->cb->GetUnitDef(this->myid);
	const UnitDef *ud_other = ai->cheat->GetUnitDef(otherUnit);
	if (ud_mine && ud_other) {
		float dps;
		dps = ai->ut->unittypearray[ud_mine ->id].DPSvsUnit[ud_other->id];
		return dps;
	}
	return 0.0f;
}

float CUNIT::GetUnitDPSvsMe(int otherUnit)
{
	const UnitDef *ud_mine = ai->cb->GetUnitDef(this->myid);
	const UnitDef *ud_other = ai->cheat->GetUnitDef(otherUnit);
	if (ud_mine && ud_other) {
		float dps;
		dps = ai->ut->unittypearray[ud_other->id].DPSvsUnit[ud_mine->id];
		return dps;
	}
	return 0.0f;
}

float CUNIT::GetAverageDPS() {
	return ai->ut->unittypearray[ai->cb->GetUnitDef(this->myid)->id].AverageDPS;
}

unsigned CUNIT::GetMoveType() {
	const UnitDef* ud = ai->cb->GetUnitDef(this->myid);
	if (ud) {
		return ai->ut->unittypearray[ud->id].moveSlopeType | ai->ut->unittypearray[ud->id].moveDepthType;
	}
	return PATHTOUSE;
}
unsigned CUNIT::GetMoveSlopeType() {
	const UnitDef* ud = ai->cb->GetUnitDef(this->myid);
	if (ud) {
		return ai->ut->unittypearray[ud->id].moveSlopeType;
	}
	return PATHTOUSE;
}
unsigned CUNIT::GetMoveDepthType() {
	const UnitDef* ud = ai->cb->GetUnitDef(this->myid);
	if (ud) {
		return ai->ut->unittypearray[ud->id].moveDepthType;
	}
	return PATHTOUSE;
}

/*bool CUNIT::Build_ClosestSite(const UnitDef* unitdef,float3 pos,int separation, float radius)
{
	float3 buildpos = ERRORVECTOR;
	int errorCount = -1;
	int startRadius = radius;
	
	float random = RANDINT;
	float dx = sin(random) *100;
	float dy = cos(random) *100;
	
	float3 randomPos = pos + float3(dx,0,dy);
	L("dx: " << dx << ", dy: " << dy);
	
	while(buildpos.x == -1)
	{
		errorCount++;
		//buildpos = ai->cb->ClosestBuildSite(unitdef,pos,RANDINT%int(radius),RANDINT%separation);
		buildpos = ai->cb->ClosestBuildSite(unitdef,randomPos,startRadius,separation);
		
		startRadius += 500;
		//randomPos = pos + float3(RANDINT%100 -50,RANDINT%100 -50,RANDINT%100  -50);
		if(startRadius > radius * 3)
			break;
	}
	L("errorCount: " << errorCount << ", startRadius: " << startRadius);

	L("Closestbuildsite(" << unitdef->name << ",(" << pos.x
		<< "," << pos.y << "," << pos.z << ")," << radius 
		<< "," << separation << "); Result: (" << buildpos.x 
		<< "," << buildpos.y << "," << buildpos.z << ")");
	ai->cb->CreateLineFigure(pos,buildpos,20,0,10000,1);
	if(buildpos.x != -1){
		L("Site found, building");
		Build(buildpos, unitdef);
		L("command sent");
		ai->uh->IdleUnitRemove(myid);
		L("idle unit removed");
        return true;
	}
	return false;
}*/
/*
bool CUNIT::Build_ClosestSite(const UnitDef* unitdef,float3 pos,int separation, float radius)
{
	float3 buildpos = ERRORVECTOR;
	int errorCount = -1;
	int startRadius = radius;
	
	float random = RANDINT;
	float dx = sin(random) *100;
	float dy = cos(random) *100;

	float3 randomPos = pos + float3(dx,0,dy);
	
	buildpos = ai->cb->ClosestBuildSite(unitdef,randomPos,startRadius,separation);
	
	L("Closestbuildsite(" << unitdef->name << ",(" << pos.x
		<< "," << pos.y << "," << pos.z << ")," << radius 
		<< "," << separation << "); Result: (" << buildpos.x 
		<< "," << buildpos.y << "," << buildpos.z << ")");
	ai->cb->CreateLineFigure(pos,buildpos,20,0,10000,1);
	if(buildpos.x != -1){
		L("Site found, building");
		Build(buildpos, unitdef);
		L("command sent");
		ai->uh->IdleUnitRemove(myid);
		L("idle unit removed");
        return true;
	}
	return false;
}*/


bool CUNIT::Build_ClosestSite(const UnitDef* unitdef,float3 targetpos,int separation, float radius,bool queue)
{
	//float3 buildpos;
	float3 buildpos = ai->cb->ClosestBuildSite(unitdef,targetpos,radius,separation);

	L("Closestbuildsite(" << unitdef->name << ",(" << targetpos.x
		<< "," << targetpos.y << "," << targetpos.z << ")," << radius 
		<< "," << separation << "); Result: (" << buildpos.x 
		<< "," << buildpos.y << "," << buildpos.z << ")");
	targetpos.y += 20;
	buildpos.y +=20;
	if(buildpos.x != -1){
		L("Site found, building");
		if (Build(buildpos, unitdef, queue)) {
			L("command sent");
			return true;
		}
	}
	else
	{
		//Move(ai->math->F3Randomize(pos(),300));
		L("Build_ClosestSite failed to find a build spot!");
	}
		
	return false;
}

bool CUNIT::FactoryBuild(const UnitDef *built)
{
		assert(ai->cb->GetUnitDef(myid) != NULL);
		Command c; 
		c.id = -built->id; 
		ai->cb->GiveOrder (myid, &c);
		ai->uh->IdleUnitRemove(myid);
		return true;
}

bool CUNIT::ReclaimBest(bool metal, float radius)
{
	static int features[1000];
	int numfound = ai->cb->GetFeatures(features,1000,pos(),radius);
	float bestscore = 0;
	float myscore = 0;
	float mydistance = 100000;
	int bestfeature = -1;
	if(metal){
		for(int i = 0; i < numfound;i++){
			mydistance = ai->cb->GetFeaturePos(features[i]).distance2D(ai->cb->GetUnitPos(myid));
			myscore = ai->cb->GetFeatureDef(features[i])->metal / (mydistance+1);
			if(ai->tm->ThreatAtThisPoint(ai->cb->GetFeaturePos(features[i])) <= ai->tm->GetAverageThreat()){				
				if(myscore > bestscore){// && !ai->uh->TaskPlanExist(ai->cb->GetFeaturePos(features[i]),ERRORDEF)){			
					// TODO: HACK: See if there are someone on this job already:
					bool found = false;
					for(list<BuilderTracker*>::iterator j = ai->uh->BuilderTrackers.begin(); j != ai->uh->BuilderTrackers.end(); j++){
						if((*j)->customOrderId == (features[i] + MAX_UNITS))
						{
							found = true;
							continue;
						}
					}
					if(found)
						continue;
					bestscore = myscore;
					bestfeature = features[i];
				}
			}
		}
	}
	else{
		for(int i = 0; i < numfound;i++){
			mydistance = ai->cb->GetFeaturePos(features[i]).distance2D(ai->cb->GetUnitPos(myid));
			myscore = ai->cb->GetFeatureDef(features[i])->energy / (mydistance+1);
			if(ai->tm->ThreatAtThisPoint(ai->cb->GetFeaturePos(features[i])) <= ai->tm->GetAverageThreat()){				
				if(myscore > bestscore){// && !ai->uh->TaskPlanExist(ai->cb->GetFeaturePos(features[i]),ERRORDEF)){			
					// TODO: HACK: See if there are someone on this job already:
					bool found = false;
					for(list<BuilderTracker*>::iterator j = ai->uh->BuilderTrackers.begin(); j != ai->uh->BuilderTrackers.end(); j++){
						if((*j)->customOrderId == (features[i] + MAX_UNITS))
						{
							found = true;
							continue;
						}
					}
					if(found)
						continue;
					bestscore = myscore;
					bestfeature = features[i];
				}
			}
		}
	}
	if(bestscore > 0){
		if (Reclaim(bestfeature + MAX_UNITS)) {
		//Reclaim(ai->cb->GetFeaturePos(bestfeature),10);
			//ai->uh->TaskPlanCreate(myid,ai->cb->GetFeaturePos(features[bestfeature]),ERRORDEF);
			return true;
		}
	}
	return false;
}


Command* CUNIT::MakePosCommand(int id,float3* pos, float radius){
	//assert(ai->cb->GetUnitDef(myid) != NULL);	
	if(pos->x > MapWidthTest)
	{
		assert(false);
		pos->x = MapWidthTest;
	}
	if(pos->z  >MapHeightTest)
	{
		assert(false);
		pos->z = MapHeightTest;
	}
	if(pos->x < 0)
	{
		assert(false);
		pos->x = 0;
	}
	/*
	if(pos->y < 0)
	{
		assert(false);
		pos->y = 0;
	}*/
	
	/*
	static Command c;
	c.id = id;
	c.options = 0;
	c.params.clear();
	c.params.push_back(pos->x);
	c.params.push_back(pos->y);
	c.params.push_back(pos->z);
	if(radius)
		c.params.push_back(radius);
	*/
	fourParamsCommand.id = id;
	fourParamsCommand.options = 0;
	fourParamsCommand.params[0] = pos->x;
	fourParamsCommand.params[1] = pos->y;
	fourParamsCommand.params[2] = pos->z;
	fourParamsCommand.params[3] = radius;
	/*
	const CCommandQueue* Mycommands = ai->cb->GetCurrentUnitCommands(myid);	
	if(Mycommands->size()){
		if(Mycommands->back().id == id 
		&& Mycommands->back().params[0] == pos.x 
		&& Mycommands->back().params[2] == pos.z){
		c.id = 0;
		//ai->cb->SendTextMsg("REPEATED COMMAND SENT!",0);
		return c;
		}
	}
	*/
	//L("groupID: " << groupID);
	if(groupID <= 0) // Ignore units that are controlled by groups
		ai->uh->IdleUnitRemove(myid);
	//L("idle unit removed");
	return &fourParamsCommand;
}

Command* CUNIT::MakePosCommand(int id,float3* pos){
	//assert(ai->cb->GetUnitDef(myid) != NULL);	
	if(pos->x > MapWidthTest)
	{
//		assert(false);
		pos->x = MapWidthTest;
	}
	if(pos->z > MapHeightTest)
	{
//		assert(false);
		pos->z = MapHeightTest;
	}
	if(pos->x < 0)
	{
//		assert(false);
		pos->x = 0;
	}
	if(pos->y < 0)
	{
//		assert(false);
		pos->y = 0;
	}
	//static Command c;
	threeParamsCommand.id = id;
	threeParamsCommand.options = 0;
	threeParamsCommand.params[0] = pos->x;
	threeParamsCommand.params[1] = pos->y;
	threeParamsCommand.params[2] = pos->z;
	/*
	const CCommandQueue* Mycommands = ai->cb->GetCurrentUnitCommands(myid);	
	if(Mycommands->size()){
		if(Mycommands->back().id == id 
		&& Mycommands->back().params[0] == pos.x 
		&& Mycommands->back().params[2] == pos.z){
		c.id = 0;
		//ai->cb->SendTextMsg("REPEATED COMMAND SENT!",0);
		return c;
		}
	}
	*/
	//L("groupID: " << groupID);
	if(groupID <= 0) // Ignore units that are controlled by groups
		ai->uh->IdleUnitRemove(myid);
	//L("idle unit removed");
	return &threeParamsCommand;
}

Command* CUNIT::MakeIntCommand(int id,int number,int maxnum){
	//assert(ai->cb->GetUnitDef(myid) != NULL);
	if(number > maxnum)
		number = maxnum;
	if(number < 0)
		number = 0;
	//static Command c;
	//c.id = id;
	//c.options = 0;
	//c.params.clear();
	//c.params.push_back(number);
	oneParamsCommand.id = id;
	oneParamsCommand.options = 0;
	oneParamsCommand.params[0] = number;
	/*
	const CCommandQueue* Mycommands = ai->cb->GetCurrentUnitCommands(myid);
	if(Mycommands->size()){
		if(Mycommands->back().id == id 
		&& Mycommands->back().params[0] == number){
			//ai->cb->SendTextMsg("REPEATED COMMAND SENT!",0);
			c.id = 0;
			return c;
		}
	}
	*/
	//L("groupID: " << groupID);
	if(groupID <= 0) // Ignore units that are controlled by groups
		ai->uh->IdleUnitRemove(myid);
	//L("idle unit removed");
	return &oneParamsCommand;
}


Command* CUNIT::MakeIntCommand(int id,int number){ 
	//assert(ai->cb->GetUnitDef(myid) != NULL);
	//static Command c;
	//c.id = id;
	//c.options = 0;
	//c.params.clear();
	//c.params.push_back(number);
	oneParamsCommand.id = id;
	oneParamsCommand.options = 0;
	oneParamsCommand.params[0] = number;
	/*
	const CCommandQueue* Mycommands = ai->cb->GetCurrentUnitCommands(myid);
	if(Mycommands->size()){
		if(Mycommands->back().id == id 
		&& Mycommands->back().params[0] == number){
			//ai->cb->SendTextMsg("REPEATED COMMAND SENT!",0);
			c.id = 0;
			return c;
		}
	}
	*/
	//L("groupID: " << groupID);
	if(groupID <= 0) // Ignore units that are controlled by groups
		ai->uh->IdleUnitRemove(myid);
	//L("idle unit removed");
	return &oneParamsCommand;
}


//---------|Target-based Abilities|--------//
bool CUNIT::Attack(int target){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canAttack) return false;
	const CCommandQueue* mycommands = ai->cb->GetCurrentUnitCommands(myid);
	if(!mycommands->empty())
	{
		const Command* c = &mycommands->front();
		if(c->id == CMD_ATTACK && c->params[0] == target)
			return true; // It have the attack command already.
	}
	
	Command* c = MakeIntCommand(CMD_ATTACK,target);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}
bool CUNIT::Capture(int target){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canCapture) return false;
	Command* c = MakeIntCommand(CMD_CAPTURE,target);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}
bool CUNIT::Guard(int target){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canGuard) return false;
	Command* c = MakeIntCommand(CMD_GUARD,target);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}
bool CUNIT::Load(int target){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->transportCapacity) return false;
	Command* c = MakeIntCommand(CMD_LOAD_UNITS,target);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}
bool CUNIT::Reclaim(int target){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canReclaim) return false;
	Command* c = MakeIntCommand(CMD_RECLAIM,target);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		ai->uh->BuilderReclaimOrder(myid, target);
		return true;
	}
	return false;
}
bool CUNIT::Repair(int target){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canRepair) return false;
	Command* c = MakeIntCommand(CMD_REPAIR,target);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}
bool CUNIT::Ressurect(int target){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canResurrect) return false;
	Command* c = MakeIntCommand(CMD_RESURRECT,target);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}


//--------|Location Point Abilities|-------//
bool CUNIT::Build(float3 pos, const UnitDef* unit,bool queue)
{
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canBuild) return false;
	if (def()->isCommander && ai->uh->Distance2DToNearestFactory(pos.x,pos.z)>DEFCBS_RADIUS*3/4) 
		return false;
	Command* c = MakePosCommand(-unit->id, &pos);
	if(c->id != 0){
		if(queue){
			c->options|=SHIFT_KEY;
		}
		ai->cb->GiveOrder(myid, c);
		ai->uh->TaskPlanCreate(myid,pos,unit);
		return true;
	}
	return false;
}
bool CUNIT::Move(float3 pos){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canmove) return false;
	Command* c = MakePosCommand(CMD_MOVE, &pos);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}
//added
bool CUNIT::MoveShift(float3 pos){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canmove) return false;
	Command* c = MakePosCommand(CMD_MOVE, &pos);
	if(c->id != 0){
		c->options|=SHIFT_KEY;
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}

bool CUNIT::MoveTwice(const float3* pos1, const float3* pos2){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canmove) return false;
	const CCommandQueue* mycommands = ai->cb->GetCurrentUnitCommands(myid);
	if(mycommands->empty())
	{
		Move(*pos1);
		MoveShift(*pos2);
	}
	else
	{
		const Command* c = &mycommands->front();
		if(c->id != CMD_MOVE)
		{
			// The unit have some other orders, so give it new ones
			//assert(false); // For testing only
			Move(*pos1);
			MoveShift(*pos2);
			return true;
		}
		// The unit have a move order. Test if its the same as the ones in the list.
		if(c->params[0] != pos1->x || c->params[1] != pos1->y || c->params[2] != pos1->z)
		{
			// The unit have a diffrent move order. Give it new ones
			Move(*pos1);
			MoveShift(*pos2);
			return true;
		}
		// The first move order is ok, test if the next one is ok
		//return true;
		if(mycommands->size() == 1)
		{
			// It didnt have more orders. Add move nr.2
			MoveShift(*pos2);
			return true;
		}
		// Get the next order.
		c = &mycommands->at(1);
		//c = &mycommands->back();
		if(c->id != CMD_MOVE || c->params[0] != pos2->x || c->params[1] != pos2->y || c->params[2] != pos2->z)
		{
			// The unit have a diffrent stage two move order. Give it new ones.
			Move(*pos1);
			MoveShift(*pos2);
			return true;
		}
		// Both orders was in the queue.
		return true;
	}

	/*
	Command* c = MakePosCommand(CMD_MOVE, &pos);
	if(c->id != 0){
		c->options|=SHIFT_KEY;
		ai->cb->GiveOrder(myid, c);
		return true;
	}*/
	return true;
}

bool CUNIT::Patrol(float3 pos){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canPatrol) return false;
	Command* c = MakePosCommand(CMD_PATROL, &pos);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}

bool CUNIT::PatrolShift(float3 pos){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canPatrol) return false;
	Command* c = MakePosCommand(CMD_PATROL, &pos);
	if(c->id != 0){
		c->options|=SHIFT_KEY;
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}



//-----------|Radius Abilities|------------//
bool CUNIT::Attack(float3 pos, float radius){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canAttack) return false;
	Command* c = MakePosCommand(CMD_ATTACK, &pos,radius);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}
bool CUNIT::Ressurect(float3 pos, float radius){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canResurrect) return false;
	Command* c = MakePosCommand(CMD_RESURRECT, &pos,radius);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}
bool CUNIT::Reclaim(float3 pos, float radius){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canReclaim) return false;
	Command* c = MakePosCommand(CMD_RECLAIM, &pos,radius);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		//ai->uh->BuilderReclaimOrder(myid, pos);
		return true;
	}
	return false;
}
bool CUNIT::Capture(float3 pos, float radius){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canCapture) return false;
	Command* c = MakePosCommand(CMD_CAPTURE, &pos,radius);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}
bool CUNIT::Restore(float3 pos, float radius){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canRestore) return false;
	Command* c = MakePosCommand(CMD_RESTORE, &pos,radius);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}
bool CUNIT::Load(float3 pos, float radius){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->transportCapacity) return false;
	Command* c = MakePosCommand(CMD_LOAD_UNITS, &pos,radius);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}
bool CUNIT::Unload(float3 pos, float radius){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->transportCapacity) return false;
	Command* c = MakePosCommand(CMD_UNLOAD_UNITS, &pos,radius);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}


//------------|bool CUNIT::Abilities|-------------//

bool CUNIT::Cloaking(bool on){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->canCloak) return false;
	Command* c = MakeIntCommand(CMD_CLOAK,on);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}
bool CUNIT::OnOff(bool on){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!ai->cb->GetUnitDef(myid)->onoffable) return false;
	Command* c = MakeIntCommand(CMD_ONOFF,on);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}


//-----------|Special Abilities|-----------//
bool CUNIT::SelfDestruct(){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	nullParamsCommand.id = CMD_SELFD;
	ai->cb->GiveOrder(myid,&nullParamsCommand);
	return true;
}

bool CUNIT::SetFiringMode(int mode){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command* c = MakeIntCommand(CMD_FIRE_STATE,mode,2);
	if(c->id != 0){
		ai->cb->GiveOrder(myid, c);
		return true;
	}
	return false;
}

bool CUNIT::Stop(){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	nullParamsCommand.id = CMD_STOP;
	ai->cb->GiveOrder(myid,&nullParamsCommand);
	return true;
}

bool CUNIT::SetMaxSpeed(float speed){
	assert(ai->cb->GetUnitDef(myid) != NULL);
	oneParamsCommand.id = CMD_SET_WANTED_MAX_SPEED;
	oneParamsCommand.params[0] = speed;
	ai->cb->GiveOrder(myid,&oneParamsCommand);
	return true;
}

bool CUNIT::BuildWeapon()
{
	assert(ai->cb->GetUnitDef(myid) != NULL);
	if (!def()->stockpileWeaponDef) return false;
	nullParamsCommand.id = CMD_STOCKPILE;
	ai->cb->GiveOrder(myid,&nullParamsCommand);
	return true;
}

int CUNIT::GetStockpiled() const
{
	int a=0;
	if (!ai->cb->GetProperty(myid,AIVAL_STOCKPILED,&a)) return 0;
	return a;
}

int CUNIT::GetStockpileQued() const
{
	int a=0;
	if (!ai->cb->GetProperty(myid,AIVAL_STOCKPILE_QUED,&a)) return 0;
	return a;
}
