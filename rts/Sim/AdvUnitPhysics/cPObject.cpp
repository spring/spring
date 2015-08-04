#include "cPObject.h"

	bool isPointInSphere(cPObject& A, float3 pointB)
	{
	pointB.x 	-= A.getPosition().x+A.m_tempOraryForce.x/A.mass; 
	pointB.y	-= A.getPosition().y+A.m_tempOraryForce.y/A.mass; 
	pointB.z	-= A.getPosition().z+A.m_tempOraryForce.y/A.mass;
	
	pointB.x =pointB.x/2;pointB.y =pointB.y/2;pointB.z=pointB.z/2;
		
	if (pointB.Length() < A.simpleSphericCollision.radius) return true;
	//calc the steigung - estimate the centerpoint - if distance to centerpoint is < range of	
	return false;	
	}
	
	
	float3  cPObject::getFutPos()
	{
	return new float3(this->m_Position.x+this->getForce().x/this->Mass,this->m_Position.y+this->getForce().y/this->Mass,this->m_Position.z+this->getForce().z/this->Mass)	
	}

	void  cPObject::solveForce()
	{

	this->m_Position.x +=
	this-> m_Position.x += this->getForce().x/this->mass;
	this-> m_Position.y += this->getForce().y/this->mass;
	this-> m_Position.z += this->getForce().z/this->mass;
	//reset
	this->m_tempOraryForce.x=0.0f;
	this->m_tempOraryForce.y=0.0f;
	this->m_tempOraryForce.z=0.0f;
	//*{<BuGmAnDiBlEs>//*{</BuGmAnDiBlEs>
	//@TODO Check if Velocity is not negative
	}

	bool cPObject::rawCollide(cPObject&  A)
	{
	//*{<BuGmAnDiBlEs>
		if (this->IsGround==true || A.isGround==true)
		{
			if (this->Ground==true && rawCollideGround(A)==true)
					this-> detailCollideGround(A);
			else
				{
				A->detailCollideGround(this);
				}
				

				
		}
		//*{</BuGmAnDiBlEs>
	return isPointInSphere(A,this->m_Position);
	}	
	
	void cPObject::detailCollide(& cPObject A)
	{
	//*{<BuGmAnDiBlEs>//*{</BuGmAnDiBlEs>
	//!@PlaceHolder:
	//calculate ContactPoint
	//ForceMirror
		
		
	//reVertVelocity
	//estimateTorque
	//
		
		
		
	}

	
	// default Constructor - returns pObjID
	int cPObject(int UnitID, int objectPiecename, float Mass, struct SshapeStruct collisionType,float3 startAcceleration,bool passivePiece)
		{
		//!@TODO: Store Information in the corresponding structs
		//!@TODO: Check if Initial Spot is not inside another cPObject
		//!@TODO: Register at the PhysicsSimulation- gain pObjID
		return pObjID;
		}

	//infectProj Constructor - returns pObjID
	int cPObject(int UnitID, int projectileID, float Mass, struct SshapeStruct collisionType,bool infectsHitObjects)
		{
		
		return pObjID;
		}	
	//attached Constructor - returns pObjID
	int cPObject(int UnitID, int objectPiecename, float Mass, struct SshapeStruct collisionType,float3 startAcceleration,bool passivePiece,int pObjAttach2ID)
		{
		
		return pObjID;
		}

	
	
	//Groundconstructor
	int cPObject::cPObject()
	{
	//*{<BuGmAnDiBlEs>//*{</BuGmAnDiBlEs>
	return pObjID;
	}





	cPObject::~cPObject()
	{
	  //*{<BuGmAnDiBlEs>//*{</BuGmAnDiBlEs>
	}

