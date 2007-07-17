#include "Maths.h"


CMaths::CMaths(AIClasses *ai)
{
	this->ai=ai;

	mapfloat3height = ai->cb->GetMapHeight() * MAPUNIT2POS;
	mapfloat3width = ai->cb->GetMapWidth() * MAPUNIT2POS;

	MTRandInt.seed(time(NULL));
	MTRandFloat.seed(MTRandInt());

#ifdef WIN32
	QueryPerformanceFrequency(&ticksPerSecond);
	ticksPerSecondDivisor = float(ticksPerSecond.QuadPart);
#endif
	groupNumberCounter = 0;
	GetNewTimerGroupNumber("Total time");
}
CMaths::~CMaths()
{
}

void CMaths::F3MapBound(float3* pos)
{
	if(pos->x < 65)
		pos->x = 65;
	else if(pos->x > mapfloat3width-65)
		pos->x = mapfloat3width - 65;
	if(pos->z < 65)
		pos->z = 65;
	else if(pos->z > mapfloat3height-65)
		pos->z = mapfloat3height - 65;
}

float3 CMaths::F3Randomize(float3 pos, float radius)
{  
	pos.x += sin(float(RANDINT/1000))* radius;
	pos.z += sin(float(RANDINT/1000))* radius;
	return pos;
}

void  CMaths::F32XY(float3 pos, int* x, int* y, int resolution)
{
	*x = int(pos.x / 8 / resolution);
	*y = int(pos.z / 8 / resolution);
}

float3  CMaths::XY2F3(int x ,int y, int resolution)
{
	float3 testpos;
	testpos.x = x * 8 * resolution;
	testpos.y = 0;
	testpos.z = y * 8 * resolution;
	return testpos;
}

float CMaths::BuildMetalPerSecond(const UnitDef* builder,const UnitDef* built) {
	if (builder->buildSpeed) {
		float buildtime = built->buildTime / builder->buildSpeed;
		return built->metalCost / buildtime;
	}
	ai->cb->SendTextMsg("MPS FAILED, unit has no buildspeed!",0);
	return -1;
}

float CMaths::BuildEnergyPerSecond(const UnitDef* builder,const UnitDef* built) {
	if(builder->buildSpeed){
		float buildtime = built->buildTime / builder->buildSpeed;
		return built->energyCost / buildtime;
	}
	ai->cb->SendTextMsg("EPS FAILED, unit has no buildspeed!",0);
	return -1;
}
float CMaths::BuildTime(const UnitDef* builder,const UnitDef* built)
{
	if(builder->buildSpeed)
		return built->buildTime / builder->buildSpeed;
	return -1;
}

bool CMaths::FeasibleConstruction(const UnitDef* builder,const UnitDef* built,float MinMpc, float MinEpc)
{
	if(builder->buildSpeed && built != NULL){
		float buildtime = built->buildTime / builder->buildSpeed;
		float Echange = ai->cb->GetEnergyIncome()*ECONRATIO - ai->cb->GetEnergyUsage() - built->energyCost / buildtime;
		float ResultingRatio = (ai->cb->GetEnergy()+(Echange*buildtime)) / ai->cb->GetEnergyStorage();
		if(ResultingRatio > MinEpc){
			float Mchange =ai->cb->GetMetalIncome()*ECONRATIO- ai->cb->GetMetalUsage() - built->metalCost / buildtime;
			ResultingRatio = (ai->cb->GetMetal()+(Mchange*buildtime)) / ai->cb->GetMetalStorage();
			if(ResultingRatio > MinMpc){
				return true;
			}
		}
	}
	return false;
}
bool CMaths::MFeasibleConstruction(const UnitDef* builder,const UnitDef* built,float MinMpc)
{
	if(builder->buildSpeed && built != NULL){
		float buildtime = built->buildTime / builder->buildSpeed;
		float Mchange =ai->cb->GetMetalIncome()*ECONRATIO- ai->cb->GetMetalUsage() - built->metalCost / buildtime;
		float ResultingRatio = (ai->cb->GetMetal()+(Mchange*buildtime)) / ai->cb->GetMetalStorage();
		if(ResultingRatio > MinMpc){
			return true;
		}
	}
	return false;
}
bool CMaths::EFeasibleConstruction(const UnitDef* builder,const UnitDef* built,float MinEpc)
{
	if(builder->buildSpeed && built != NULL){
		float buildtime = built->buildTime / builder->buildSpeed;
		float Echange = ai->cb->GetEnergyIncome()*ECONRATIO - ai->cb->GetEnergyUsage() - built->energyCost / buildtime;
		float ResultingRatio = (ai->cb->GetEnergy()+(Echange*buildtime)) / ai->cb->GetEnergyStorage();
		if(ResultingRatio > MinEpc){
			return true;
		}
	}
	return false;
}


float CMaths::ETA(int unit, float3 destination)
{
	float speed = ai->cb->GetUnitDef(unit)->speed;
	float distance = destination.distance2D(ai->cb->GetUnitPos(unit));
	return distance/speed * 2;
}

float CMaths::ETT(BuildTask* bt)
{
	float percentdone = ai->cb->GetUnitHealth(bt->id) / ai->cb->GetUnitMaxHealth(bt->id);
	float buildpower = bt->currentBuildPower;
	/*list<int> killbuilders;
	for(list<int>::iterator i = bt.builders.begin(); i != bt.builders.end(); i++){
		if(ai->cb->GetUnitDef(*i))
			buildpower += ai->cb->GetUnitDef(*i)->buildSpeed;
		else
			killbuilders.push_back(*i);
	}
	for(list<int>::iterator i = killbuilders.begin(); i != killbuilders.end(); i++){
		bt.builders.remove(*i);
	}*/
	if(buildpower > 0){ // Spring use 30 frames in one sec but 32 in one economic sec, so buildpower is lower
		return (ai->cb->GetUnitDef(bt->id)->buildTime / (buildpower * 15/16)) * (1-percentdone);
	}
	L("Error, buildpower <= 0");
	return 1000000000000000000.0; // Just to make shure other maths dont fail
}


float CMaths::GetUnitCost(const UnitDef* unit)
{
	return unit->metalCost * METAL2ENERGY + unit->energyCost;
}

float CMaths::GetUnitCost(int unit)
{
	return ai->cb->GetUnitDef(unit)->metalCost * METAL2ENERGY +  ai->cb->GetUnitDef(unit)->energyCost;
}

float CMaths::RandNormal(float m, float s, bool positiveonly)
{
  // normal distribution with mean m and standard deviation s
  float normal_x1,normal_x2,w;  
  // make two normally distributed variates by Box-Muller transformation
  do {
    normal_x1 = 2. * RANDFLOAT - 1.;
    normal_x2 = 2. * RANDFLOAT - 1.;
    w = normal_x1*normal_x1 + normal_x2*normal_x2;}
  while (w >= 1. || w < 1E-30);
  w = sqrt(log(w)*(-2./w));
  normal_x1 *= w;   // normal_x1 and normal_x2 are independent normally distributed variates
  if(!positiveonly)
	return normal_x1 * s + m;
  else
	return max(0.0f,normal_x1 * s + m);
}

float CMaths::RandFloat()
{
	return MTRandFloat();
}
unsigned int CMaths::RandInt()
{
	return MTRandInt();
}

void CMaths::TimerStart()
{
#ifdef WIN32
	QueryPerformanceCounter(&tick_start);
	tick_laststop = tick_start;
#else
	gettimeofday(&tick_start, NULL);
	tick_laststop = tick_start;
#endif
}

int	CMaths::TimerTicks()
{
#ifdef WIN32
	QueryPerformanceCounter(&tick_end);
	tick_laststop = tick_end;
	return tick_end.QuadPart - tick_start.QuadPart;
#else
	gettimeofday(&tick_end, NULL);
	tick_laststop = tick_end;
	return (tick_end.tv_sec - tick_start.tv_sec) * 1000000 + (tick_end.tv_usec - tick_start.tv_usec);
#endif
}


float CMaths::TimerSecs()
{
#ifdef WIN32
	QueryPerformanceCounter(&tick_end);
	tick_laststop = tick_end;
	return (float(tick_end.QuadPart) - float(tick_start.QuadPart))/float(ticksPerSecond.QuadPart);
#else
	gettimeofday(&tick_end, NULL);
	tick_laststop = tick_end;
	return (tick_end.tv_sec - tick_start.tv_sec) + (tick_end.tv_usec - tick_start.tv_usec) * 1.0e-6f;
#endif
}

int CMaths::GetNewTimerGroupNumber(string timerName)
{
	int num = groupNumberCounter;
	timerStruct[num].sumTime = 0;
	sumTimeNameList[num] = timerName;
	if(groupNumberCounter < 999)
		groupNumberCounter++;
	return num;
}

void CMaths::PrintAllTimes()
{
	float sumTimeAll = timerStruct[0].sumTime;
	float sumTimeCountUp = 0;
	for(int i = 1; i < groupNumberCounter; i++)
	{
		sumTimeCountUp += timerStruct[i].sumTime;
	}
	L("sumTimeAll: " << sumTimeAll);
	if(sumTimeAll <= 0)
		return;
	L("Overlap: " << (100 * ((sumTimeCountUp  / sumTimeAll) -1) ) << "%");
	char textArray[512];
	
	for(int i = 0; i < groupNumberCounter; i++)
	{
		float sumTime = timerStruct[i].sumTime;
		float fraction = 100 * sumTime / sumTimeAll;
		sprintf(textArray, "%8.4f%% (%9.3f):  %s", fraction, sumTime, sumTimeNameList[i].c_str());
		L(textArray);
	}
}

