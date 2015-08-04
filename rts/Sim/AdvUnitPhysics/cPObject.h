#ifndef CPOBJECT_H
    #define CPOBJECT_H
    #include <vector>

class cPObject
{
public:
	// default Constructor - returns pObjID
	int cPObject(
		int UnitID, 
		int objectPiecename, 
		float Mass, 
		struct SshapeStruct collisionType,
		float3 startAcceleration,
		bool passivePiece
		);

	//infectProj Constructor - returns pObjID
	int cPObject(
		int UnitID, 
		int projectileID, 
		float Mass, 
		struct SshapeStruct collisionType,
		bool infectsHitObjects
		);
		
	//attached Constructor - returns pObjID
	int cPObject(
		int UnitID, 
		int objectPiecename, 
		float Mass, 
		struct SshapeStruct collisionType,
		float3 startAcceleration,
		bool passivePiece,
		int pObjAttach2ID
		);

	//ground Constructor
	int cPObject();
	
	//Getters and Setters
	
	void setPosition(float3 Pos)this->m_Position=Pos;
	float3& getPosition(void) return const float3& pos= this->m_Position;

	void setForce(float3 add) this->m_tempOraryForce=add;
	float3& getForce() return const float3 & force= this->m_tempOraryForce;
	void resetForce()this->m_tempOraryForce= new float3(0,0,0);
	void addForce(float3 add)  this->m_tempOraryForce=this->m_tempOraryForce+add;
	
	float3 getPosition() return this->m_Position+m_offSetPos;
	void setPosition(float3 Position) this->m_Position=Position;
	
	
	//
	float3 getFutPos();								//only one SimStep ahead
	void solveForce();								//!Warning, final Step, after all Collission, will set Obj to new Position
	
	bool rawCollideGround(&cPObject A); 		
	void detailCollideGround(&cPObject A);			// calculates detailed Collission  with the Landscape (gets current Heightmap for that) 

	bool rawCollide(&cPObject A);
	void detailCollide(&cPObjectA);					//calculates the Results of a DetailedCollission based upon


private:
	//Offset
	float3 m_offSetPos;								//UnitMovement
	float3 m_offSetRot;								//UnitRotation
	
	//PhysicsObjectID
	int pObjID;
	
	
	//Position
	float3 m_Position;

	//Direction
	float3 m_dirEction;

	//Mass 
	float mass;

	//Torque (in Radiants/perSimStep)
	float3 m_Torque;	
	
	//Forces
	float3 m_tempOraryForce;	//decendant of the legendary temp O`Rary- first amr settler to be thrown offerboard when useless

	//rotationLocks
	vector<SrotDirLock> rotLocks;
	
	//Booleans
	bool isGround;
	bool isProjectile;
	
	//Collissionproperties
	bool m_passivObject;		//Object does not react on forces applied to it
	bool m_complexCollission;		//Uses the polygonmodell to estimate Collission! EXPENSIVE
	struct SshapeStruct simpleSphericCollision
 	float m_ojbectElasticConstant;
};

//locked direction is zone surrounded by rad coords
struct SrotDirLock
{
	float3 lock1,	
	float3 lock2,
	float3 lock3;
};

struct SshapeStruct
{
float3 center, //Relative to ObjectMatriceCenter
float3 scale,
float radius;
};
#endif // CPOBJECT_H
