
/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SEGMENT
#define SEGMENT

#include <vector>
#include <math.h>
#include "point3f.h"


typedef enum {
	BALLJOINT,
	LIMJOINT
} JointType;



class LocalModelPiece;

class Segment
{


private:

		// transformation matrix (rotation) of the segment
		AngleAxisf T, saved_T, last_T;

		// save the angle when computing the changes
		Point3f saved_angle;
		
		// the type of joint the origin of the segment is
		// connected to
		JointType joint;

		//next Segments Starting Point aka BoneEnd
		Point3f pUnitNextPieceBasePoint;

		//UnitOrigin Position in UnitSpace
		Point3f pUnitPieceBasePoint;
		

		
public:
		// magnitude of the segment
		float mag;
		
		//Original Direction  Vector
		Point3f orgDirVec;
		
		bool alteredInSolve;
	
		// constructors
		Segment();
		Segment(unsigned int pieceID, LocalModelPiece* lPiece, float magnitude, JointType jt);
		Segment(unsigned int pieceID, LocalModelPiece* lPiece, Point3f pUnitNextPieceBasePointOffset, JointType jt);
		~Segment();

		Point3f velocity;
		void setLimitJoint( float limX, 
						float limUpX,
						float limY, 
						float limUpY,	
						float limZ,
						float limUpZ);

		Point3f jointUpLim; //upper Limit in Radians
		Point3f jointLowLim;//Lower Limit in Radians

		Point3f clampJoint(Point3f Value);
		float distance(Point3f a, Point3f b);
		//corresponding piece
		LocalModelPiece* piece;
		//yes, this is doubling (unchangeable) Information, but reducing pointeritis
		unsigned int pieceID;
 		Point3f lastValidRotation;

		// returns end point in object space
		Point3f get_end_point();

		Vector3f get_rotation();

		Vector3f  get_right();
		Vector3f  get_up();
		Vector3f  get_z();

		AngleAxisf get_T();
        
		float get_mag();
		void print();

		void save_transformation();
		void load_transformation();

		void save_last_transformation();
		void load_last_transformation();

		void apply_angle_change(float rad_change, Vector3f  angle);

		// clear transformations
		void reset();
		// randomize transformation
		void randomize();

		// apply transformation
		void transform(AngleAxisf t);
};

#endif // Segment
