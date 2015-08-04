#ifndef CPSIMULATION_H
	
#include <vector>

static vector<&cForce>  g_ForceTable = new vector<&cForce>;
static vector<&cPObject>  g_ObjectTable = new vector<&cPObject>;
static vector <int> g_ObjectIDs= new vector <int>;
static vector <int> g_ForceIDs= new vector <int>;


int getNrOfObjects()
{
return g_ObjectTable.size;
}

&cPObject getObjectNr( int Nr)	
{
//{<BuGmAnDiBlEs> //}</BuGmAnDiBlEs>
return g_ObectTable[Nr];
}

struct collissionDetails
{
float3 PointOfCollision,
Force onA,
Force onB,
torque onA,
torque onB,
};

class cPSimulation
{
public:
cPSimulation(); 							//*ConStructor: Pushes basic Forces
~cPSimulation(bool Reset);					//*Destructor: If PhySim is destroyed, all attached Objects and Units can be reset 	

int registerForce(cForce & mayTheForce);
int registerObject(cPObject & Objectified);

void PhysicsPipeline();

int getSimStep() return this->simStepTime;
void setSimStep(int simFrames) this->simStepTime=simFrames;

 
										//Objectchoosen Collissionmodell and applys resulting Forces



bool applyForces();							//applies forces to all objects

		void mapOHierarchyToFO(
		int UnitID,
		int ObjectPieceName;
		);

private:
int simStepTime;

};


#endif // CPSIMULATION_H
