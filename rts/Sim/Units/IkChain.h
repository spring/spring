/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IKCHAIN
#define IKCHAIN

#include <vector>
#include "Segment.h"
#include "point3f.h"

using namespace Eigen;

class LocalModelPiece;
class LocalModel;
class CUnit;


typedef enum {
	OVERRIDE,
	BLENDIN
} MotionBlend;


///Class Ikchain- represents a Inverse Kinmatik Chain
class IkChain
{

public:
	enum AnimType {ANone = -1, ATurn = 0, ASpin = 1, AMove = 2};
	
	
	enum Axis {xAxis = 0, yAxis = 1, zAxis =2 };
	///Constructors 
	IkChain();
	
	//Create the segments from a startPieceId and an endPieceID
	IkChain(int id, 
		CUnit* unit, 
		LocalModelPiece* startPiece, 
		unsigned int startPieceID, 
		unsigned int endPieceID,
	        unsigned int frameLength);

	//Flag set from outside of the IK-Chain - wether the target has changed, is set to false by the IK-Chain
	bool GoalChanged=true;

	//is the transfered new GoalPoint a World Coordinate?
	bool isWorldCoordinate =false;

	//Helper Function to inialize the Path recursive
	bool recPiecePathExplore(LocalModelPiece* parentLocalModel, unsigned int parentPiece, unsigned int endPieceNumber, int depth);
	bool initializePiecePath(LocalModelPiece* startPiece, unsigned int startPieceID, unsigned int endPieceID);
	
	//Checks wether a Piece is part of this IK-Chain
	bool isValidIKPiece(float pieceID);

	//Transfers a global Worldspace coordinate into a unitspace coordinate
	Point3f TransformGoalToUnitspace(Point3f goal);

	//IK is active or paused- paused IK-Chains can jolt 
	bool IKActive ;
	
	//Setter
	void SetActive (bool isActive);

	//Solves the Inverse Kinematik Chain for a goal point
	//arguments: frame - current active frame, becomes Startframe on  new motion
	void solve(float frame );
	
	//apply the resolved Kinematics to the actual Model
	void applyIkTransformation(MotionBlend motionBlendMethod);

	//Set Piece Limitations
	//Get the Next PieceNumber while building the chain
	int GetNextPieceNumber(float PieceNumber);

	//Creates a Jacobi Matrice
	Matrix<float,1,3> compute_jacovian_segment(int seg_num, Point3f goal_point, Vector3f  angle);

	// computes end_effector up to certain number of segments
	Point3f calculate_end_effector(int segment_num = -1);

	//Gets the basic bone rotation for the piece 
	Point3f GetBoneBaseRotation(void);

	//unit this IKChain belongs too
	CUnit* unit;

	//Identifier of the kinematik Chain
	float IkChainID;

	//Set the Angle for the Transformation matrice
	void SetTransformation(float valX, float valY, float valZ);

	// the goal Point also in World Coordinats
	Point3f goalPoint;
	
	//the start Point in World Coordinates of the Animation
	Point3f startPoint;

	//Vector containing the Segments
	std::vector <Segment> segments;
	
	// determinates the initial direction of the  ik-system
	void determinateInitialDirection(void);
	
	//First Segment
	bool bFirstSegment = true;
	
	//Initial Default direction of the Kinematik system
	Point3f vecDefaultlDirection;
	
	//StartFrame of the Animation
	unsigned int startFrame = 0;
	
	//ArivalFrame of the Animation
	unsigned int goalFrame = 0;
	
	//Last Frame to solve from
	unsigned int lastSolveFrame = 0;
	
	//Plots the whole IK-Chain
	void print();
	
	//Debug function
	void printPoint( const char* name, float x, float y, float z);
	void printPoint( const char* name, Point3f point);
	//Destructor
	~IkChain();
private:

	//Calculates the endpoint in 3d Coords for a given Piece after the movement
	Point3f calculateEndEffector(int Segment = -1);

	//Get the max Lenfgth of the IK Chain
	float getMaxLength();
};

#endif // IKCHAIN
