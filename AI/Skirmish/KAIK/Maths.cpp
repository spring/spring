#include <ctime>

#include "IncExternAI.h"
#include "IncGlobalAI.h"

CMaths::CMaths(AIClasses* ai) {
	this->ai = ai;

	mapfloat3height = ai->cb->GetMapHeight() * MAPUNIT2POS;
	mapfloat3width = ai->cb->GetMapWidth() * MAPUNIT2POS;

	MTRandInt.seed(time(NULL));
	MTRandFloat.seed(MTRandInt());

#ifdef WIN32
	QueryPerformanceFrequency(&ticksPerSecond);
#endif
}

CMaths::~CMaths() {
}


void CMaths::F3MapBound(float3& pos) {
	if (pos.x < 65)
		pos.x = 65;
	else if (pos.x > mapfloat3width - 65)
		pos.x = mapfloat3width - 65;

	if (pos.z < 65)
		pos.z = 65;
	else if (pos.z > mapfloat3height - 65)
		pos.z = mapfloat3height - 65;
}

float3 CMaths::F3Randomize(const float3& pos, float radius) {
	float3 p;
	p.x = pos.x + sin(float(RANDINT / 1000)) * radius;
	p.z = pos.z + sin(float(RANDINT / 1000)) * radius;

	return p;
}

void  CMaths::F32XY(const float3& pos, int* x, int* y, int resolution) {
	*x = int(pos.x / 8 / resolution);
	*y = int(pos.z / 8 / resolution);
}

float3  CMaths::XY2F3(int x ,int y, int resolution) {
	float3 testpos;
	testpos.x = x * 8 * resolution;
	testpos.y = 0;
	testpos.z = y * 8 * resolution;

	return testpos;
}


float CMaths::BuildMetalPerSecond(const UnitDef* builder,const UnitDef* built) {
	if (builder->buildSpeed) {
		float buildtime = built->buildTime / builder->buildSpeed;

		return ((built->metalCost) / buildtime);
	}

	// MPS FAILED, unit has no buildspeed
	return -1;
}

float CMaths::BuildEnergyPerSecond(const UnitDef* builder,const UnitDef* built) {
	if (builder->buildSpeed) {
		float buildtime = built->buildTime / builder->buildSpeed;

		return ((built->energyCost) / buildtime);
	}

	// EPS FAILED, unit has no buildspeed
	return -1;
}


float CMaths::BuildTime(const UnitDef* builder,const UnitDef* built) {
	if (builder->buildSpeed)
		return ((built->buildTime) / (builder->buildSpeed));

	return -1;
}

/*
bool CMaths::FeasibleConstruction(const UnitDef* builder, const UnitDef* built, float MinMpc, float MinEpc) {
	if (builder->buildSpeed) {
		float buildtime = (built->buildTime) / (builder->buildSpeed);
		float Echange = ((ai->cb->GetEnergyIncome()) * ECONRATIO) - (ai->cb->GetEnergyUsage()) - (built->energyCost / buildtime);
		float ResultingRatio = (ai->cb->GetEnergy() + (Echange * buildtime)) / ai->cb->GetEnergyStorage();

		if (ResultingRatio > MinEpc) {
			float Mchange = ai->cb->GetMetalIncome() * ECONRATIO - ai->cb->GetMetalUsage() - built->metalCost / buildtime;
			ResultingRatio = (ai->cb->GetMetal() + (Mchange * buildtime)) / (ai->cb->GetMetalStorage());

			if (ResultingRatio > MinMpc) {
				return true;
			}
		}
	}

	return false;
}
*/

bool CMaths::MFeasibleConstruction(const UnitDef* builder, const UnitDef* built, float MinMpc) {
	if (builder->buildSpeed) {
		float buildtime = (built->buildTime) / (builder->buildSpeed);
		float Mchange = ((ai->cb->GetMetalIncome()) * ECONRATIO) - (ai->cb->GetMetalUsage()) - (built->metalCost / buildtime);
		// KLOOTNOTE: dividing by actual metal storage
		// means things become infeasible with greater
		// M storage capacity
		float denom = 1.0f; // ai->cb->GetMetalStorage();
		float ResultingRatio = (ai->cb->GetMetal() + (Mchange * buildtime)) / denom;

		if (ResultingRatio > MinMpc) {
			return true;
		}
	}

	return false;
}

bool CMaths::EFeasibleConstruction(const UnitDef* builder, const UnitDef* built, float MinEpc) {
	if (builder->buildSpeed) {
		float buildtime = (built->buildTime) / (builder->buildSpeed);
		float Echange = ((ai->cb->GetEnergyIncome()) * ECONRATIO) - (ai->cb->GetEnergyUsage()) - (built->energyCost / buildtime);
		// KLOOTNOTE: dividing by actual energy storage
		// means things become infeasible with greater
		// E storage capacity
		float denom = 1.0f; // ai->cb->GetEnergyStorage();
		float ResultingRatio = (ai->cb->GetEnergy() + (Echange * buildtime)) / denom;

		if (ResultingRatio > MinEpc) {
			return true;
		}
	}

	return false;
}


float CMaths::ETA(int unit, const float3& destination) {
	float speed = ai->cb->GetUnitDef(unit)->speed;
	float distance = destination.distance2D(ai->cb->GetUnitPos(unit));

	return (distance / speed * 2);
}

float CMaths::ETT(BuildTask& bt) {
	float percentdone = (ai->cb->GetUnitHealth(bt.id)) / (ai->cb->GetUnitMaxHealth(bt.id));
	float buildpower = 0.0f;
	std::list<int> killbuilders;

	for (std::list<int>::iterator i = bt.builders.begin(); i != bt.builders.end(); i++) {
		if (ai->cb->GetUnitDef(*i))
			buildpower += ai->cb->GetUnitDef(*i)->buildSpeed;
		else
			killbuilders.push_back(*i);
	}

	for (std::list<int>::iterator i = killbuilders.begin(); i != killbuilders.end(); i++) {
		bt.builders.remove(*i);
	}

	if (buildpower > 0.0f) {
		return ((ai->cb->GetUnitDef(bt.id)->buildTime) / buildpower) * (1 - percentdone);
	}

	return MY_FLT_MAX;
}


float CMaths::GetUnitCost(const UnitDef* unit) {
	return ((unit->metalCost * METAL2ENERGY) + (unit->energyCost));
}

float CMaths::GetUnitCost(int unit) {
	return (ai->cb->GetUnitDef(unit)->metalCost * METAL2ENERGY) + (ai->cb->GetUnitDef(unit)->energyCost);
}

float CMaths::RandNormal(float m, float s, bool positiveonly) {
	// normal distribution with mean m and standard deviation s
	float normal_x1, normal_x2, w;
	// make two normally distributed variates by Box-Muller transformation

	do {
		normal_x1 = 2.0 * RANDFLOAT - 1.0;
		normal_x2 = 2.0 * RANDFLOAT - 1.0;
		w = normal_x1 * normal_x1 + normal_x2 * normal_x2;
	}
	while (w >= 1.0 || w < 1E-30);

	w = sqrt(log(w) * (-2.0 / w));

	// normal_x1 and normal_x2 are independent normally distributed variates
	normal_x1 *= w;

	if (!positiveonly)
		return (normal_x1 * s + m);
	else
		return std::max(0.0f, normal_x1 * s + m);
}


float CMaths::RandFloat() {
	return MTRandFloat();
}
unsigned int CMaths::RandInt() {
	return MTRandInt();
}

void CMaths::TimerStart() {
#ifdef WIN32
	QueryPerformanceCounter(&tick_start);
	tick_laststop = tick_start;
#else
	gettimeofday(&tick_start, NULL);
	tick_laststop = tick_start;
#endif
}

int CMaths::TimerTicks() {
#ifdef WIN32
	QueryPerformanceCounter(&tick_end);
	tick_laststop = tick_end;

	return (tick_end.QuadPart - tick_start.QuadPart);
#else
	gettimeofday(&tick_end, NULL);
	tick_laststop = tick_end;

	return ((tick_end.tv_sec - tick_start.tv_sec) * 1000000 + (tick_end.tv_usec - tick_start.tv_usec));
#endif
}


float CMaths::TimerSecs() {
#ifdef WIN32
	QueryPerformanceCounter(&tick_end);
	tick_laststop = tick_end;

	return ((float(tick_end.QuadPart) - float(tick_start.QuadPart)) / float(ticksPerSecond.QuadPart));
#else
	gettimeofday(&tick_end, NULL);
	tick_laststop = tick_end;

	return ((tick_end.tv_sec - tick_start.tv_sec) + (tick_end.tv_usec - tick_start.tv_usec) * 1.0e-6f);
#endif
}
