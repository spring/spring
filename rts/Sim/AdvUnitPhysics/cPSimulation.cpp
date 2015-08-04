#include "cPSimulation.h"


cPSimulation::cPSimulation(int simStepTime)
{
  	  //cPSimulation init()
	//the timeintervall occupied by a frame
	this->simStepTime=simStepTime;
	// create a Instance of GravityForce
	
	/*
	const float3 direction,
 	const struct Saccelearation acceleRation, 
	bool globalForce
	
	
	*//
	struct Sacceleration gravityTemp= new Sacceleration(); 								//Fix malloc and alloc, & are structs with null initialised valid
	gravityTemp.linearAcceleration=new float3(0,Game.Gravity/1000*this->simStepTime,0);		// @TODO: Search Source for the related Value, break Game.Gravity down to the simframe 
	
	cForce Gravity= new cForce(gravityTemp,new float3(0,-1,0),true);
	g_ForceTable.push(Gravity);
	
	// create the GroundObject
	cPObject Ground= new cPObject();
	g_ObjectTable.push(Ground);
}

cPSimulation::PhysicsPipeline()
{
//caluclateOffsets
//applyForces
cPSimulation::applyForces();
//solveForcesForObjects - we need the Future Positions to check on Collissions - on Collission we just extrapolate  backwards

//RawCollission
//n^2
for (int i=0;i<g_ObjectTable.size;i++)
	{
	for (int j=0;j<g_ObjectTable.size;j++)
		{
		//{<BuGmAnDiBlEs> 
			if (i!=j && g_ObjectTable[i]. rawCollide(g_ObjectTable[j]))
			{
		//}</BuGmAnDiBlEs>
			 g_ObjectTable[i].detailCollide(g_ObjectTable[j]);
				
			}			
		}
	}

//!@TODO for@0 for@1 m_ObjectTable if cPSimulation::RawCollission()
//DetailCollission
//DragForces
//generateMovements, Rolls
//Resets of temps, checking if all Forces are still alive

}


void cPSimulation::applyForces()
{
for (int i=0; i< g_ForceTable.size;i++)
	{
	for (int j=0; j< g_ObjectTable.size;j++)
		{
		if (g_ForceTable[i].isGlobal()==true || g_ForceTable[i].isObjectOnListAndInForceField(g_ObjectTable[j])
			{
			g_ObjectTable[j].addTemporaryForce(getForce( g_ObjectTable[j].getPosition(),
		 	//*@TODO getCurrentTime()-g_ForceTable[i]startTime););
			}		
		}	
	}
}

cPSimulation::~cPSimulation()
{
    //dtor
}

