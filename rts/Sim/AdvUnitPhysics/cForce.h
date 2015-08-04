#ifndef CFORCE_H
	#define CFORCE_H
	#define FORCE_MIN 0.0000042
	#include <vector>

	//*defaults to -1 if the cForce is not constantly dragging towards a target
struct  SdirUnitPiece
{   
	bool rigidSystem,
	int pieceID,
	int unitID,
	float suspensionConstant, 		//*Fsc=Sc(suspensionLength-(1-suspensionLenght)
	float suspensionLenght, 			//*aus total Percentage
   	float dampenerConstant; 		//* FD= dC(vel1-vel2)
  
};

	//*defaults to spheric Fields if luaFormulaHandle is NULL
struct SfieldGeometry
{
	int luaFormulaHandle,
	float3 position,
	float radius;
};
	//*defaults to linear Acceleration if luaFormulaHandle is NULL
struct Sforce
{
	int luaFormulahandle,
	float3 linearForce,
	float suspensionConstant,
	float suspensionLenght,
	float dampeningConstant;
};

class CForce
{
	public:
	//*Basic Constructor- Force has to have a Direction and a Acceleration
	cForce(
	const float3 direction,
 	const struct Sforce forceBasics, 
	bool globalForce); //*@TODO Constructor + ObjRegistration

	//*Alternative Piecebound Constructor- Force drags towards - or away from this piece
	cForce(
	const float3 direction, 
 	const struct Sforce forceBasics, 
	const SdirUnitPiece piecePointer, 
	float suspensionConstant, 
	float suspensionLength,
	float dampeningConstant,
	bool globalcForce);	//*@TODO Constructor + ObjRegistration

	//*Alternative Constructor-  for absolute Attaching a Piece to another
	cForce(
	const unit_piece Alt,
	const float suspensionConstant, 
	float suspensionLength,
	const float dampeningConstant);//*@TODO Constructor + ObjRegistration
		//*ConstantDefinition:suspensionConstant= (1-(SC^Distance))

		virtual ~cForce();


		//*Getter & Setter
		 float3 GetDirection()
			{
			//*{<BuGmAnDiBlEs>
			if (m_direction == NULL)return GetDirectionViaUnitPiece();
			//*}</BuGmAnDiBlEs>
			return m_direction;
			}

		 void SetDirection(float3 newDirection, float scalar )
			{
			//*{<BuGmAnDiBlEs>
			if (scalar != NULL) m_direction= this.m_direction+(newDirection*scalar);
			//*}</BuGmAnDiBlEs>
			this->m_direction= newDirection;
			}

		 float3 GetDirectionViaUnitPiece();
		 float3 GetAcceleration(&cPObject massTurbAtion);

		 void SetForce( int luaFormulahandle, float3 linearAcceleartion )
			   {
				//*{<BuGmAnDiBlEs> //*}</BuGmAnDiBlEs>
			   if (luaFormulahandle != NULL) this.m_Acceleration.luaFormulahandle=luaFormulahandle;
			   else this.m_Acceleration.linearAcceleartion=linearAcceleartion;
			   }

		 void SetFieldGeometry(int luaFormulaHandle, float3 position, float radius)
			{
			   //*{<BuGmAnDiBlEs> //*}</BuGmAnDiBlEs>
			   if (luaFormulaHandle!=NULL)
					this.m_ForceFieldShape.luaFormulaHandle=luaFormulaHandle;

			   if (position!= NULL &&  radius != NULL)
				 {
				  this.m_ForceFieldShape.position=position;
				  this.m_ForceFieldShape.radius=radius;
				 }
			}

		 float GetPointInField(float3 Point); //*@TODO: Spherical Check & Luahandlecall
		 void isForceDead()
			{
			if (this->lifeTime <= 0 || luaFormulaWrapper.isLuaFormulaValid(this->) == false)
				{
					this->~cForce();
					return true
				}
				else return false;
			}



		void getForceInfo(); //*@TODO LUA:: Spring.GetForceInfo(ForceID)
		bool isGlobal() return this->globalForce;
		bool isObjectOnListAndInForceField(&cPObject);
		float3 getForce(float3 positionOfObject, float lookAtTheTime);  //calls the LuaWrapper with the current Time of the Force
	protected:
	private:
	//*direction
	float3		m_direction;
	unit_piece	m_pointingTowardsPiece;
	float3 m_directionOld;
	&cPObject  aimedAtObject;

	vector <&cPObject>	m_forceAppliesToObject; 	//List of Objects this Force Applies too

	//*position
	float3 m_position;
	float3 m_positionOld;
	
	float3 m_positionOffset;
	float3 m_directionOffset;

	float3 m_drag;  //*Drag consists of a upper Limit, and a constant ^ third value
					//*Dragtypeforces are applied last

	//*lifetime
	int lifeTime=1;
	float startTime;
	//*acceleration
	Sforce m_Acceleration;

	//*fieldGeometry
	bool globalForce;
	SfieldGeometry m_ForceFieldShape;
};

#endif //* CFORCE_H
