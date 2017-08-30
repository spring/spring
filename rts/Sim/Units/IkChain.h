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
	
	//Create the segments
	IkChain(int id, CUnit* unit, LocalModelPiece* startPiece, unsigned int startPieceID, unsigned int endPieceID);

	//Was the Goal altered
	bool GoalChanged=true;

	//is the GoalPoint a World Coordinate?
	bool isWorldCoordinate =false;

	//Helper Function to inialize the Path recursive
	bool recPiecePathExplore(LocalModelPiece* parentLocalModel, unsigned int parentPiece, unsigned int endPieceNumber, int depth);
	bool initializePiecePath(LocalModelPiece* startPiece, unsigned int startPieceID, unsigned int endPieceID);
	
	//Checks wether a Piece is part of this chain
	bool isValidIKPiece(float pieceID);

	//Transfers a global Worldspace coordinate into a unitspace coordinate
	Point3f TransformGoalToUnitspace(Point3f goal);

	//IK is active or paused
	bool IKActive ;

	//Setter
	void SetActive (bool isActive);

	//Solves the Inverse Kinematik Chain for a new goal point
	//Returns wether a ik-solution could be found
	void solve(float frames );
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
	//TODO it would be a great performance saviour if there was a flag 
	Point3f GetBoneBaseRotation(void);

	//unit this IKChain belongs too
	CUnit* unit;

	//Identifier of the Kinematik Chain
	float IkChainID;

	//The baseposition in WorldCoordinats
	Point3f base;

	//Set the Anglke for the Transformation matrice
	void SetTransformation(float valX, float valY, float valZ);

	// the goal Point also in World Coordinats
	Point3f goalPoint;

	//Vector containing the Segments
	std::vector <Segment> segments;
	
	// determinates the initial direction of the  ik-system
	void determinateInitialDirection(void);
	
	//First Segment
	bool bFirstSegment = true;
	
	//Initial Default direction of the Kinematik system
	Point3f vecDefaultlDirection;

	//Size of Segment
	//int segment_size ;

	//Plots the whole IK-Chain
	void print();
	
	//Debug function
	void printPoint( const char* name, float x, float y, float z);
	void printPoint( const char* name, Point3f point);
	//Destructor
	~IkChain();
private:

	//TODO find out what this one does
	Point3f calculateEndEffector(int Segment = -1);

	//Get the max Lenfgth of the IK Chain
	float getMaxLength();
};

#endif // IKCHAIN
