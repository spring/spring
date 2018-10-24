
#include "IkChain.h"
#include "Unit.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include <iostream>

// See end of source for member bindings
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


//checks wether a IK-Chain contains a piece 
bool IkChain::isValidIKPiece(float pieceID){
		if (segments.empty()) {return false;}

		for (auto seg = segments.begin(); seg !=  segments.end(); ++seg) 
		{
		  if ((*seg).pieceID == pieceID) return true;
		}

return false;
}

void IkChain::SetActive(bool isActive)
{
	IKActive  = isActive;
}

void IkChain::SetTransformation(float valX, float valY, float valZ)
{
		//Initiator
	    for(int i=0; i<3*segments.size(); i+=3) {
			// apply the change to the theta angle
		    segments[i/3].apply_angle_change(valX ,segments[i/3].get_right());
			// apply the change to the phi angle
			segments[i/3].apply_angle_change(valY, segments[i/3].get_up());
			// apply the change to the z angle
		    segments[i/3].apply_angle_change(valZ, segments[i/3].get_z());
		}	
	
}

void IkChain::determinateInitialDirection(void){
	bFirstSegment = false;
	
	vecDefaultlDirection =  segments[1].get_end_point() - segments[0].get_end_point();
	}
	

//recursive explore the model and find the end of the IK-Chain
bool IkChain::recPiecePathExplore(  LocalModelPiece* parentLocalModelPiece, 
									unsigned int parentPiece,
									unsigned int endPieceNumber, 
									int depth){
										
									
	//Get DecendantsNumber
	for (auto piece = (*parentLocalModelPiece).children.begin(); piece !=  (*parentLocalModelPiece).children.end(); ++piece) 
		{
		   //we found the last piece of the kinematikChain --unsigned int compared with
			if ((*piece)->scriptPieceIndex ==  endPieceNumber){
			//	std::cout<<"Piece at :"<< depth << " piecnr - > "<<((*piece))->scriptPieceIndex <<std::endl;

				//lets gets size as a float3
				float3 scale = (*piece)->GetCollisionVolume()->GetScales();
				
				segments.resize(depth+1);
				assert(!segments.empty() && (segments.size() == (depth+1)));
				segments[depth] = Segment((*piece)->scriptPieceIndex, (*piece), scale.y, BALLJOINT);

				return true;
			}
			
			//if one of the pieces children is the endpiece backtrack
			if (recPiecePathExplore((*piece), 
									(*piece)->scriptPieceIndex , 
									endPieceNumber, 
									depth+1 ) == true)
			{
			//   std::cout<<"Piece at :"<< depth << " piecnr - > "<<((*piece))->scriptPieceIndex <<std::endl;
					
					//we assume correctness and determinate the initial armconfig
			if (bFirstSegment)  determinateInitialDirection()
	
				
				
				//Get the magnitude of the bone - extract the startPoint of the successor
				float3 posBase = segments[depth+1].piece->GetAbsolutePos();

				Point3f pUnitNextPieceBasePointOffset= Point3f(posBase.x,posBase.y,posBase.z);

				assert(!segments.empty() );
				segments[depth] =Segment((*piece)->scriptPieceIndex, (*piece), pUnitNextPieceBasePointOffset, BALLJOINT);

				return true;		
			}
		}
return false;
}

bool IkChain::initializePiecePath(LocalModelPiece* startPiece, unsigned int startPieceID,unsigned int endPieceID){
	bool initializationComplete=false;
	//Check for 
	if ( startPieceID < 1 || endPieceID < 1 ) return initializationComplete;

	initializationComplete = recPiecePathExplore(startPiece, startPieceID, endPieceID, 0);
	
	//Lets correct the direction of the last piece 
	segments[segments.size()].orgDirVec = segments[segments.size()-1].orgDirVec.normalized() * segments[segments.size()].mag;
	
	
	return initializationComplete;
}

IkChain::IkChain(int id, CUnit* unit, LocalModelPiece* startPiece, unsigned int startPieceID, unsigned int endPieceID, unsigned int animationLengthInFrames)
{
	this->unit = unit;
	
	//std::cout<< "start,endpiece"<<startPieceID <<" / " <<endPieceID <<std::endl;
	  if( initializePiecePath(startPiece, (unsigned int) startPieceID, (unsigned int) endPieceID) == false) {
		std::cout<<"Startpiece is beneath Endpiece - Endpiece could not be found"<<std::endl;
	  }
	 
	// pre initialize all the Points - and the 
	float3 piecePos= startPiece->GetAbsolutePos();
	startPoint = Point3f(piecePos.x,piecePos.y,piecePos.z);
	goalPoint   = Point3f(0,0,0);

	IKActive= false;
	IkChainID = id;
	
	SetTransformation(0,0,0);
	
}
void IkChain::printPoint( const char* name, Point3f point)
{
	printPoint(name, point[0], point[1], point[2]);
}


void IkChain::printPoint( const char* name, float x, float y, float z)
{
	std::cout << "	<<--------------------------------"<<std::endl;
	std::cout << "	"<<name<< "=	X: ("<<x<<") "<<" Y: ("<<y<<") "<<" Z: ("<<z<<") "<<std::endl;
	std::cout << "	---------------------------------->> "<<std::endl;
}


void IkChain::print()
{
	
	Point3f transformed;
	transformed = goalPoint.normalized() * getMaxLength();

	std::cout<<  "============================================== "<<std::endl;
	std::cout<<  " IkChain = "<<std::endl;
	std::cout<<  "	[[ "<<std::endl;
	printPoint("GoalPoint:", goalPoint(0,0),goalPoint(1,0),goalPoint(2,0));
	printPoint("Clamped Goal:", transformed[0],transformed[1],transformed[2]);
	printPoint("StartPoint:", startPoint(0,0),startPoint(1,0), startPoint(2,0));
	std::cout<<"MaxLength: "<< getMaxLength()<<std::endl;
	std::cout<<"isWorldCoordinate: "<<isWorldCoordinate <<std::endl;

		for (auto seg = segments.begin(); seg != segments.end(); ++seg) 
		{
			seg->print();
		}
	std::cout<< "	]] "<<std::endl;
	std::cout<<  "============================================== "<<std::endl;

}

IkChain::~IkChain()
{
	//Release the Pieces
}
//////////////////////////////////////////////////////////////////////
// Template for the pseudo Inverse
//////////////////////////////////////////////////////////////////////
template<typename _Matrix_Type_>
_Matrix_Type_ pseudoInverse(const _Matrix_Type_ &a, double epsilon =
std::numeric_limits<double>::epsilon())
{
	Eigen::JacobiSVD< _Matrix_Type_ > svd(a ,Eigen::ComputeThinU |
	Eigen::ComputeThinV);

	double tolerance =  epsilon * std::max(a.cols(), a.rows()) *svd.singularValues().array().abs()(0);
	return svd.matrixV() *  (svd.singularValues().array().abs() >
	tolerance).select(svd.singularValues().array().inverse(),
	0).matrix().asDiagonal() * svd.matrixU().adjoint();
}
//////////////////////////////////////////////////////////////////////
float IkChain::getMaxLength(){
	float totalDistance=0.0001;
  //Get DecendantsNumber
	for (auto seg = segments.begin(); seg != segments.end(); ++seg) 
	{
		totalDistance += seg->get_mag();
	}
		
return totalDistance;
}

Point3f IkChain::TransformGoalToUnitspace(Point3f goal){
	float3 fGoal= float3(goal(0,0),goal(1,0),goal(2,0));
	float3 unitPoint =unit->GetTransformMatrix()*fGoal;
	return Point3f(unitPoint.x ,unitPoint.y,unitPoint.z);
}

void IkChain::solve( float  frames) 
{	
	if (!IKActive) return;
	
	// prev and curr are for use of halving
	// last is making sure the iteration gets a better solution than the last iteration,
	// otherwise revert changes
	Point3f goal_point;
	goal_point = this->goalPoint;
    float prev_err, curr_err, last_err = 9999;
    Point3f current_point;
    int max_iterations = 200;
    int count = 0;
    float err_margin = 0.01;

    goal_point -= base;
    if (goal_point.norm() > this->getMaxLength()) {
		std::cout<<"Goal Point out of reachable sphere! Normalied to" << this->getMaxLength()<<std::endl;
	    goal_point =  ( this->goalPoint.normalized() * this->getMaxLength());
	}
	
    current_point = calculate_end_effector();
	printPoint("Base Point:",base);
	printPoint("Start Point:",current_point);
	printPoint("Goal  Point:",goal_point);
	// save the first err
    prev_err = (goal_point - current_point).norm();
    curr_err = prev_err;
    last_err = curr_err;

	// while the current point is close enough, stop iterating
    while (curr_err > err_margin) {
		// calculate the difference between the goal_point and current_point
	    Vector3f dP = goal_point - current_point;

		// create the jacovian
	    int segment_size = segments.size();

		// build the transpose matrix (easier for eigen matrix construction)
	    MatrixXf jac_t(3*segment_size, 3);
	    for(int i=0; i<3*segment_size; i+=3) {
		    Matrix<float, 1, 3> row_theta =compute_jacovian_segment(i/3, goal_point, segments[i/3].get_right());
		    Matrix<float, 1, 3> row_phi = compute_jacovian_segment(i/3, goal_point, segments[i/3].get_up());
		    Matrix<float, 1, 3> row_z = compute_jacovian_segment(i/3, goal_point, segments[i/3].get_z());

		    jac_t(i, 0) = row_theta(0, 0);
		    jac_t(i, 1) = row_theta(0, 1);
		    jac_t(i, 2) = row_theta(0, 2);

		    jac_t(i+1, 0) = row_phi(0, 0);
		    jac_t(i+1, 1) = row_phi(0, 1);
		    jac_t(i+1, 2) = row_phi(0, 2);

		    jac_t(i+2, 0) = row_z(0, 0);
		    jac_t(i+2, 1) = row_z(0, 1);
		    jac_t(i+2, 2) = row_z(0, 2);
		}

		// compute the final jacovian
	    MatrixXf jac(3, 3*segment_size);
	    jac = jac_t.transpose();

	    Matrix<float, Dynamic, Dynamic> pseudo_ijac;
	    MatrixXf pinv_jac(3*segment_size, 3);
	    pinv_jac = pseudoInverse(jac);

	    Matrix<float, Dynamic, 1> changes = pinv_jac * dP;


	    for(int i=0; i<3*segment_size; i+=3) {
			// save the current transformation on the segments
		    segments[i/3].save_transformation();

			// apply the change to the theta angle
		    segments[i/3].apply_angle_change(changes[i], segments[i/3].get_right());
			// apply the change to the phi angle
			//segments[i/3].apply_angle_change(3.1415/3, segments[i/3].get_up());
			segments[i/3].apply_angle_change(changes[i+1], segments[i/3].get_up());
			// apply the change to the z angle
		    segments[i/3].apply_angle_change(changes[i+2], segments[i/3].get_z());
		}

		// compute current_point after making changes
	    current_point = calculate_end_effector();

		//cout << "current_point: " << vectorString(current_point) << endl;
		//cout << "goal_point: " << vectorString(goal_point) << endl;

	    prev_err = curr_err;
	    curr_err = (goal_point - current_point).norm();

	    int halving_count = 0;

		// make sure we aren't iterating past the solution
	    while (curr_err > last_err) {
			// undo changes
		    for(int i=0; i<segment_size; i++) {
				// unapply the change to the saved angle
			    segments[i].load_transformation();
			}
		    current_point = calculate_end_effector();
		    changes *= 0.5;
			// reapply halved changes
		    for(int i=0; i<3*segment_size; i+=3) {
				// save the current transformation on the segments
			    segments[i/3].save_transformation();

				// apply the change to the theta angle
			   // segments[i/3].apply_angle_change(3.1415/8, segments[i/3].get_right());
				segments[i/3].apply_angle_change(changes[i], segments[i/3].get_right());
				// apply the change to the phi angle
			    segments[i/3].apply_angle_change(changes[i+1], segments[i/3].get_up());
				// apply the change to the z angle
			    segments[i/3].apply_angle_change(changes[i+2], segments[i/3].get_z());
			}

			// compute the end_effector and measure error
		    current_point = calculate_end_effector();
		    prev_err = curr_err;
		    curr_err = (goal_point - current_point).norm();

		    halving_count++;
		    if (halving_count > 100)
			    break;
		}

	    if (curr_err > last_err) {
			// undo changes
		    for(int i=0; i<segment_size; i++) {
				// unapply the change to the saved angle
			    segments[i].load_last_transformation();
			}
		    current_point = calculate_end_effector();
		    curr_err = (goal_point - current_point).norm();
		    break;
		}
	    for(int i=0; i<segment_size; i++) {
			// unapply the change to the saved angle
		    segments[i].save_last_transformation();
		}
	    last_err = curr_err;


		// make sure we don't infinite loop
	    count++;
	    if (count > max_iterations) {
		    break;
		}
	}

	applyIkTransformation(OVERRIDE);
   }

//Returns the Negated Accumulated Rotation
Point3f IkChain::GetBoneBaseRotation()
{
	Point3f accumulatedRotation = Point3f(0,0,0);
	float3  modelRot;
	LocalModelPiece * parent = segments[0].piece;
	//if the goalPoint is a world coordinate, we need the units rotation out of the picture
  
	while (parent != NULL){
		modelRot= parent->GetRotation();
		accumulatedRotation[0] -= modelRot.x;
		accumulatedRotation[1] -= modelRot.y;
		accumulatedRotation[2] -= modelRot.z;
		
		parent = (parent->parent != NULL? parent->parent: NULL);
			
	}

	//add unit rotation on top
	if (isWorldCoordinate){
		const CMatrix44f& matrix = unit->GetTransformMatrix(true);
		assert(matrix.IsOrthoNormal());
		const float3 angles = matrix.GetEulerAnglesLftHand();

		accumulatedRotation(0,0) += angles.x;
		accumulatedRotation(1,0) += angles.y;
		accumulatedRotation(2,0) += angles.z;
	}
	
return accumulatedRotation;
}
	
void IkChain::applyIkTransformation(MotionBlend motionBlendMethod){

	GoalChanged=false;


	//The Rotation the Pieces accumulate, so each piece can roate as if in world
	Point3f pAccRotation= GetBoneBaseRotation();
	pAccRotation= Point3f(0,0,0);
	
		//Get the Unitscript for the Unit that holds the segment
		for (auto seg = segments.begin(); seg !=  segments.end(); ++seg) {
			seg->alteredInSolve = true;

			Point3f velocity = seg->velocity;
			Point3f rotation = seg->get_rotation();

			rotation -= pAccRotation;
			pAccRotation+= rotation;

			unit->script->AddAnim(   CUnitScript::ATurn,
									(int)(seg->pieceID),  //pieceID 
									xAxis,//axis  
									1.0,//velocity(0,0),// speed
									rotation[0], //TODO jointclamp this
									0.0f //acceleration
									);

			unit->script->AddAnim( CUnitScript::ATurn,
									(int)(seg->pieceID),  //pieceID 
									yAxis,//axis  
									1.0,//,// speed
									rotation[1], //TODO jointclamp this
									0.0f //acceleration
									);

			unit->script->AddAnim(  CUnitScript::ATurn,
									(int)(seg->pieceID),  //pieceID 
									zAxis,//axis  
									1.0,// speed
									rotation[2], //TODO jointclamp this
									0.0f //acceleration
									);
		}
}


// computes end_effector up to certain number of segments
Point3f IkChain::calculate_end_effector(int segment_num /* = -1 */) {
	Point3f reti;

	int segment_num_to_calc = segment_num;
	// if default value, compute total end effector
	if (segment_num == -1) {
		segment_num_to_calc = segments.size() - 1;
	}
	// else don't mess with it

	// start with base
	reti = base;
	for(int i=0; i<=segment_num_to_calc; i++) {
		// add each segments end point vector to the base
		reti += segments[i].get_end_point();
	}
	// return calculated end effector
	return reti ;
}


//Returns a Jacovian Segment a row of 3 Elements
Matrix<float, 1, 3>  IkChain::compute_jacovian_segment(int seg_num, Vector3f  goalPoint, Point3f angle) 
{
	Segment *s = &(segments.at(seg_num));
	// mini is the amount of angle you go in the direction for numerical calculation
	float mini = 0.0005;

	Point3f transformed_goal = goalPoint;
	for(int i=segments.size()-1; i>seg_num; i--) {
		// transform the goal point to relevence to this segment
		// by removing all the transformations the segments afterwards
		// apply on the current segment
		transformed_goal -= segments[i].get_end_point();
	}

	Point3f my_end_effector = calculate_end_effector(seg_num);

	// transform them both to the origin
	if (seg_num-1 >= 0) {
		my_end_effector -= calculate_end_effector(seg_num-1);
		transformed_goal -= calculate_end_effector(seg_num-1);
	}

	// original end_effector
	Point3f original_ee = calculate_end_effector();

	// angle input is the one you rotate around
	// remove all the rotations from the previous segments by applying them
	AngleAxisf t = AngleAxisf(mini, angle);

	// transform the segment by some delta(theta)
	s->transform(t);
	// new end_effector
	Point3f new_ee = calculate_end_effector();
	
	// reverse the transformation afterwards
	s->transform(t.inverse());

		// difference between the end_effectors
	// since mini is very small, it's an approximation of
	// the derivative when divided by mini
	Vector3f  diff = new_ee - original_ee;

	// return the row of dx/dtheta, dy/dtheta, dz/dtheta
	Matrix<float, 1, 3> ret;
	ret << diff[0]/mini, diff[1]/mini, diff[2]/mini;
	return ret;
}

// computes end_effector up to certain number of segments
Point3f IkChain::calculateEndEffector(int segment_num /* = -1 */) {
	Point3f ret;

	int segment_num_to_calc = segment_num;
	// if default value, compute total end effector
	if (segment_num == -1) {
		segment_num_to_calc = segments.size() - 1;
	}
	// else don't mess with it

	// start with base
	ret = base;
	for(int i=0; i<=segment_num_to_calc; i++) {
		// add each segments end point vector to the base
		ret += segments[i].get_end_point();
	}
	// return calculated end effector
	return ret;
}



/******************************************************************************/
