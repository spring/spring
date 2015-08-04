//this file is autogenerated using stringify.bat (premake --stringify) in the build folder of this project
static const char* solveConstraintRowsCL= \
"/*\n"
"Copyright (c) 2013 Advanced Micro Devices, Inc.  \n"
"This software is provided 'as-is', without any express or implied warranty.\n"
"In no event will the authors be held liable for any damages arising from the use of this software.\n"
"Permission is granted to anyone to use this software for any purpose, \n"
"including commercial applications, and to alter it and redistribute it freely, \n"
"subject to the following restrictions:\n"
"1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.\n"
"2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.\n"
"3. This notice may not be removed or altered from any source distribution.\n"
"*/\n"
"//Originally written by Erwin Coumans\n"
"#define B3_CONSTRAINT_FLAG_ENABLED 1\n"
"#define B3_GPU_POINT2POINT_CONSTRAINT_TYPE 3\n"
"#define B3_GPU_FIXED_CONSTRAINT_TYPE 4\n"
"#define MOTIONCLAMP 100000 //unused, for debugging/safety in case constraint solver fails\n"
"#define B3_INFINITY 1e30f\n"
"#define mymake_float4 (float4)\n"
"__inline float dot3F4(float4 a, float4 b)\n"
"{\n"
"	float4 a1 = mymake_float4(a.xyz,0.f);\n"
"	float4 b1 = mymake_float4(b.xyz,0.f);\n"
"	return dot(a1, b1);\n"
"}\n"
"typedef float4 Quaternion;\n"
"typedef struct\n"
"{\n"
"	float4 m_row[3];\n"
"}Matrix3x3;\n"
"__inline\n"
"float4 mtMul1(Matrix3x3 a, float4 b);\n"
"__inline\n"
"float4 mtMul3(float4 a, Matrix3x3 b);\n"
"__inline\n"
"float4 mtMul1(Matrix3x3 a, float4 b)\n"
"{\n"
"	float4 ans;\n"
"	ans.x = dot3F4( a.m_row[0], b );\n"
"	ans.y = dot3F4( a.m_row[1], b );\n"
"	ans.z = dot3F4( a.m_row[2], b );\n"
"	ans.w = 0.f;\n"
"	return ans;\n"
"}\n"
"__inline\n"
"float4 mtMul3(float4 a, Matrix3x3 b)\n"
"{\n"
"	float4 colx = mymake_float4(b.m_row[0].x, b.m_row[1].x, b.m_row[2].x, 0);\n"
"	float4 coly = mymake_float4(b.m_row[0].y, b.m_row[1].y, b.m_row[2].y, 0);\n"
"	float4 colz = mymake_float4(b.m_row[0].z, b.m_row[1].z, b.m_row[2].z, 0);\n"
"	float4 ans;\n"
"	ans.x = dot3F4( a, colx );\n"
"	ans.y = dot3F4( a, coly );\n"
"	ans.z = dot3F4( a, colz );\n"
"	return ans;\n"
"}\n"
"typedef struct\n"
"{\n"
"	Matrix3x3 m_invInertiaWorld;\n"
"	Matrix3x3 m_initInvInertia;\n"
"} BodyInertia;\n"
"typedef struct\n"
"{\n"
"	Matrix3x3 m_basis;//orientation\n"
"	float4	m_origin;//transform\n"
"}b3Transform;\n"
"typedef struct\n"
"{\n"
"//	b3Transform		m_worldTransformUnused;\n"
"	float4		m_deltaLinearVelocity;\n"
"	float4		m_deltaAngularVelocity;\n"
"	float4		m_angularFactor;\n"
"	float4		m_linearFactor;\n"
"	float4		m_invMass;\n"
"	float4		m_pushVelocity;\n"
"	float4		m_turnVelocity;\n"
"	float4		m_linearVelocity;\n"
"	float4		m_angularVelocity;\n"
"	union \n"
"	{\n"
"		void*	m_originalBody;\n"
"		int		m_originalBodyIndex;\n"
"	};\n"
"	int padding[3];\n"
"} b3GpuSolverBody;\n"
"typedef struct\n"
"{\n"
"	float4 m_pos;\n"
"	Quaternion m_quat;\n"
"	float4 m_linVel;\n"
"	float4 m_angVel;\n"
"	unsigned int m_shapeIdx;\n"
"	float m_invMass;\n"
"	float m_restituitionCoeff;\n"
"	float m_frictionCoeff;\n"
"} b3RigidBodyCL;\n"
"typedef struct\n"
"{\n"
"	float4		m_relpos1CrossNormal;\n"
"	float4		m_contactNormal;\n"
"	float4		m_relpos2CrossNormal;\n"
"	//float4		m_contactNormal2;//usually m_contactNormal2 == -m_contactNormal\n"
"	float4		m_angularComponentA;\n"
"	float4		m_angularComponentB;\n"
"	\n"
"	float	m_appliedPushImpulse;\n"
"	float	m_appliedImpulse;\n"
"	int	m_padding1;\n"
"	int	m_padding2;\n"
"	float	m_friction;\n"
"	float	m_jacDiagABInv;\n"
"	float		m_rhs;\n"
"	float		m_cfm;\n"
"	\n"
"    float		m_lowerLimit;\n"
"	float		m_upperLimit;\n"
"	float		m_rhsPenetration;\n"
"	int			m_originalConstraint;\n"
"	int	m_overrideNumSolverIterations;\n"
"    int			m_frictionIndex;\n"
"	int m_solverBodyIdA;\n"
"	int m_solverBodyIdB;\n"
"} b3SolverConstraint;\n"
"typedef struct \n"
"{\n"
"	int m_bodyAPtrAndSignBit;\n"
"	int m_bodyBPtrAndSignBit;\n"
"	int m_originalConstraintIndex;\n"
"	int m_batchId;\n"
"} b3BatchConstraint;\n"
"typedef struct \n"
"{\n"
"	int				m_constraintType;\n"
"	int				m_rbA;\n"
"	int				m_rbB;\n"
"	float			m_breakingImpulseThreshold;\n"
"	float4 m_pivotInA;\n"
"	float4 m_pivotInB;\n"
"	Quaternion m_relTargetAB;\n"
"	int	m_flags;\n"
"	int m_padding[3];\n"
"} b3GpuGenericConstraint;\n"
"/*b3Transform	getWorldTransform(b3RigidBodyCL* rb)\n"
"{\n"
"	b3Transform newTrans;\n"
"	newTrans.setOrigin(rb->m_pos);\n"
"	newTrans.setRotation(rb->m_quat);\n"
"	return newTrans;\n"
"}*/\n"
"__inline\n"
"float4 cross3(float4 a, float4 b)\n"
"{\n"
"	return cross(a,b);\n"
"}\n"
"__inline\n"
"float4 fastNormalize4(float4 v)\n"
"{\n"
"	v = mymake_float4(v.xyz,0.f);\n"
"	return fast_normalize(v);\n"
"}\n"
"__inline\n"
"Quaternion qtMul(Quaternion a, Quaternion b);\n"
"__inline\n"
"Quaternion qtNormalize(Quaternion in);\n"
"__inline\n"
"float4 qtRotate(Quaternion q, float4 vec);\n"
"__inline\n"
"Quaternion qtInvert(Quaternion q);\n"
"__inline\n"
"Quaternion qtMul(Quaternion a, Quaternion b)\n"
"{\n"
"	Quaternion ans;\n"
"	ans = cross3( a, b );\n"
"	ans += a.w*b+b.w*a;\n"
"//	ans.w = a.w*b.w - (a.x*b.x+a.y*b.y+a.z*b.z);\n"
"	ans.w = a.w*b.w - dot3F4(a, b);\n"
"	return ans;\n"
"}\n"
"__inline\n"
"Quaternion qtNormalize(Quaternion in)\n"
"{\n"
"	return fastNormalize4(in);\n"
"//	in /= length( in );\n"
"//	return in;\n"
"}\n"
"__inline\n"
"float4 qtRotate(Quaternion q, float4 vec)\n"
"{\n"
"	Quaternion qInv = qtInvert( q );\n"
"	float4 vcpy = vec;\n"
"	vcpy.w = 0.f;\n"
"	float4 out = qtMul(qtMul(q,vcpy),qInv);\n"
"	return out;\n"
"}\n"
"__inline\n"
"Quaternion qtInvert(Quaternion q)\n"
"{\n"
"	return (Quaternion)(-q.xyz, q.w);\n"
"}\n"
"__inline void internalApplyImpulse(__global b3GpuSolverBody* body,  float4 linearComponent, float4 angularComponent,float impulseMagnitude)\n"
"{\n"
"	body->m_deltaLinearVelocity += linearComponent*impulseMagnitude*body->m_linearFactor;\n"
"	body->m_deltaAngularVelocity += angularComponent*(impulseMagnitude*body->m_angularFactor);\n"
"}\n"
"void resolveSingleConstraintRowGeneric(__global b3GpuSolverBody* body1, __global b3GpuSolverBody* body2, __global b3SolverConstraint* c)\n"
"{\n"
"	float deltaImpulse = c->m_rhs-c->m_appliedImpulse*c->m_cfm;\n"
"	float deltaVel1Dotn	=	dot3F4(c->m_contactNormal,body1->m_deltaLinearVelocity) 	+ dot3F4(c->m_relpos1CrossNormal,body1->m_deltaAngularVelocity);\n"
"	float deltaVel2Dotn	=	-dot3F4(c->m_contactNormal,body2->m_deltaLinearVelocity) + dot3F4(c->m_relpos2CrossNormal,body2->m_deltaAngularVelocity);\n"
"	deltaImpulse	-=	deltaVel1Dotn*c->m_jacDiagABInv;\n"
"	deltaImpulse	-=	deltaVel2Dotn*c->m_jacDiagABInv;\n"
"	float sum = c->m_appliedImpulse + deltaImpulse;\n"
"	if (sum < c->m_lowerLimit)\n"
"	{\n"
"		deltaImpulse = c->m_lowerLimit-c->m_appliedImpulse;\n"
"		c->m_appliedImpulse = c->m_lowerLimit;\n"
"	}\n"
"	else if (sum > c->m_upperLimit) \n"
"	{\n"
"		deltaImpulse = c->m_upperLimit-c->m_appliedImpulse;\n"
"		c->m_appliedImpulse = c->m_upperLimit;\n"
"	}\n"
"	else\n"
"	{\n"
"		c->m_appliedImpulse = sum;\n"
"	}\n"
"	internalApplyImpulse(body1,c->m_contactNormal*body1->m_invMass,c->m_angularComponentA,deltaImpulse);\n"
"	internalApplyImpulse(body2,-c->m_contactNormal*body2->m_invMass,c->m_angularComponentB,deltaImpulse);\n"
"}\n"
"__kernel void solveJointConstraintRows(__global b3GpuSolverBody* solverBodies,\n"
"					  __global b3BatchConstraint* batchConstraints,\n"
"					  	__global b3SolverConstraint* rows,\n"
"						__global unsigned int* numConstraintRowsInfo1, \n"
"						__global unsigned int* rowOffsets,\n"
"						__global b3GpuGenericConstraint* constraints,\n"
"						int batchOffset,\n"
"						int numConstraintsInBatch\n"
"                      )\n"
"{\n"
"	int b = get_global_id(0);\n"
"	if (b>=numConstraintsInBatch)\n"
"		return;\n"
"	__global b3BatchConstraint* c = &batchConstraints[b+batchOffset];\n"
"	int originalConstraintIndex = c->m_originalConstraintIndex;\n"
"	if (constraints[originalConstraintIndex].m_flags&B3_CONSTRAINT_FLAG_ENABLED)\n"
"	{\n"
"		int numConstraintRows = numConstraintRowsInfo1[originalConstraintIndex];\n"
"		int rowOffset = rowOffsets[originalConstraintIndex];\n"
"		for (int jj=0;jj<numConstraintRows;jj++)\n"
"		{\n"
"			__global b3SolverConstraint* constraint = &rows[rowOffset+jj];\n"
"			resolveSingleConstraintRowGeneric(&solverBodies[constraint->m_solverBodyIdA],&solverBodies[constraint->m_solverBodyIdB],constraint);\n"
"		}\n"
"	}\n"
"};\n"
"__kernel void initSolverBodies(__global b3GpuSolverBody* solverBodies,__global b3RigidBodyCL* bodiesCL, int numBodies)\n"
"{\n"
"	int i = get_global_id(0);\n"
"	if (i>=numBodies)\n"
"		return;\n"
"	__global b3GpuSolverBody* solverBody = &solverBodies[i];\n"
"	__global b3RigidBodyCL* bodyCL = &bodiesCL[i];\n"
"	solverBody->m_deltaLinearVelocity = (float4)(0.f,0.f,0.f,0.f);\n"
"	solverBody->m_deltaAngularVelocity  = (float4)(0.f,0.f,0.f,0.f);\n"
"	solverBody->m_pushVelocity = (float4)(0.f,0.f,0.f,0.f);\n"
"	solverBody->m_pushVelocity = (float4)(0.f,0.f,0.f,0.f);\n"
"	solverBody->m_invMass = (float4)(bodyCL->m_invMass,bodyCL->m_invMass,bodyCL->m_invMass,0.f);\n"
"	solverBody->m_originalBodyIndex = i;\n"
"	solverBody->m_angularFactor = (float4)(1,1,1,0);\n"
"	solverBody->m_linearFactor = (float4) (1,1,1,0);\n"
"	solverBody->m_linearVelocity = bodyCL->m_linVel;\n"
"	solverBody->m_angularVelocity = bodyCL->m_angVel;\n"
"}\n"
"__kernel void breakViolatedConstraintsKernel(__global b3GpuGenericConstraint* constraints, __global unsigned int* numConstraintRows, __global unsigned int* rowOffsets, __global b3SolverConstraint* rows, int numConstraints)\n"
"{\n"
"	int cid = get_global_id(0);\n"
"	if (cid>=numConstraints)\n"
"		return;\n"
"	int numRows = numConstraintRows[cid];\n"
"	if (numRows)\n"
"	{\n"
"		for (int i=0;i<numRows;i++)\n"
"		{\n"
"			int rowIndex = rowOffsets[cid]+i;\n"
"			float breakingThreshold = constraints[cid].m_breakingImpulseThreshold;\n"
"			if (fabs(rows[rowIndex].m_appliedImpulse) >= breakingThreshold)\n"
"			{\n"
"				constraints[cid].m_flags =0;//&= ~B3_CONSTRAINT_FLAG_ENABLED;\n"
"			}\n"
"		}\n"
"	}\n"
"}\n"
"__kernel void getInfo1Kernel(__global unsigned int* infos, __global b3GpuGenericConstraint* constraints, int numConstraints)\n"
"{\n"
"	int i = get_global_id(0);\n"
"	if (i>=numConstraints)\n"
"		return;\n"
"	__global b3GpuGenericConstraint* constraint = &constraints[i];\n"
"	switch (constraint->m_constraintType)\n"
"	{\n"
"		case B3_GPU_POINT2POINT_CONSTRAINT_TYPE:\n"
"		{\n"
"			infos[i] = 3;\n"
"			break;\n"
"		}\n"
"		case B3_GPU_FIXED_CONSTRAINT_TYPE:\n"
"		{\n"
"			infos[i] = 6;\n"
"			break;\n"
"		}\n"
"		default:\n"
"		{\n"
"		}\n"
"	}\n"
"}\n"
"__kernel void initBatchConstraintsKernel(__global unsigned int* numConstraintRows, __global unsigned int* rowOffsets, \n"
"										__global b3BatchConstraint* batchConstraints, \n"
"										__global b3GpuGenericConstraint* constraints,\n"
"										__global b3RigidBodyCL* bodies,\n"
"										int numConstraints)\n"
"{\n"
"	int i = get_global_id(0);\n"
"	if (i>=numConstraints)\n"
"		return;\n"
"	int rbA = constraints[i].m_rbA;\n"
"	int rbB = constraints[i].m_rbB;\n"
"	batchConstraints[i].m_bodyAPtrAndSignBit = bodies[rbA].m_invMass? rbA : -rbA;\n"
"	batchConstraints[i].m_bodyBPtrAndSignBit = bodies[rbB].m_invMass? rbB : -rbB;\n"
"	batchConstraints[i].m_batchId = -1;\n"
"	batchConstraints[i].m_originalConstraintIndex = i;\n"
"}\n"
"typedef struct\n"
"{\n"
"	// integrator parameters: frames per second (1/stepsize), default error\n"
"	// reduction parameter (0..1).\n"
"	float fps,erp;\n"
"	// for the first and second body, pointers to two (linear and angular)\n"
"	// n*3 jacobian sub matrices, stored by rows. these matrices will have\n"
"	// been initialized to 0 on entry. if the second body is zero then the\n"
"	// J2xx pointers may be 0.\n"
"	union \n"
"	{\n"
"		__global float4* m_J1linearAxisFloat4;\n"
"		__global float* m_J1linearAxis;\n"
"	};\n"
"	union\n"
"	{\n"
"		__global float4* m_J1angularAxisFloat4;\n"
"		__global float* m_J1angularAxis;\n"
"	};\n"
"	union\n"
"	{\n"
"	__global float4* m_J2linearAxisFloat4;\n"
"	__global float* m_J2linearAxis;\n"
"	};\n"
"	union\n"
"	{\n"
"		__global float4* m_J2angularAxisFloat4;\n"
"		__global float* m_J2angularAxis;\n"
"	};\n"
"	// elements to jump from one row to the next in J's\n"
"	int rowskip;\n"
"	// right hand sides of the equation J*v = c + cfm * lambda. cfm is the\n"
"	// \"constraint force mixing\" vector. c is set to zero on entry, cfm is\n"
"	// set to a constant value (typically very small or zero) value on entry.\n"
"	__global float* m_constraintError;\n"
"	__global float* cfm;\n"
"	// lo and hi limits for variables (set to -/+ infinity on entry).\n"
"	__global float* m_lowerLimit;\n"
"	__global float* m_upperLimit;\n"
"	// findex vector for variables. see the LCP solver interface for a\n"
"	// description of what this does. this is set to -1 on entry.\n"
"	// note that the returned indexes are relative to the first index of\n"
"	// the constraint.\n"
"	__global int *findex;\n"
"	// number of solver iterations\n"
"	int m_numIterations;\n"
"	//damping of the velocity\n"
"	float	m_damping;\n"
"} b3GpuConstraintInfo2;\n"
"void	getSkewSymmetricMatrix(float4 vecIn, __global float4* v0,__global float4* v1,__global float4* v2)\n"
"{\n"
"	*v0 = (float4)(0.		,-vecIn.z		,vecIn.y,0.f);\n"
"	*v1 = (float4)(vecIn.z	,0.			,-vecIn.x,0.f);\n"
"	*v2 = (float4)(-vecIn.y	,vecIn.x	,0.f,0.f);\n"
"}\n"
"void getInfo2Point2Point(__global b3GpuGenericConstraint* constraint,b3GpuConstraintInfo2* info,__global b3RigidBodyCL* bodies)\n"
"{\n"
"	float4 posA = bodies[constraint->m_rbA].m_pos;\n"
"	Quaternion rotA = bodies[constraint->m_rbA].m_quat;\n"
"	float4 posB = bodies[constraint->m_rbB].m_pos;\n"
"	Quaternion rotB = bodies[constraint->m_rbB].m_quat;\n"
"		// anchor points in global coordinates with respect to body PORs.\n"
"   \n"
"    // set jacobian\n"
"    info->m_J1linearAxis[0] = 1;\n"
"	info->m_J1linearAxis[info->rowskip+1] = 1;\n"
"	info->m_J1linearAxis[2*info->rowskip+2] = 1;\n"
"	float4 a1 = qtRotate(rotA,constraint->m_pivotInA);\n"
"	{\n"
"		__global float4* angular0 = (__global float4*)(info->m_J1angularAxis);\n"
"		__global float4* angular1 = (__global float4*)(info->m_J1angularAxis+info->rowskip);\n"
"		__global float4* angular2 = (__global float4*)(info->m_J1angularAxis+2*info->rowskip);\n"
"		float4 a1neg = -a1;\n"
"		getSkewSymmetricMatrix(a1neg,angular0,angular1,angular2);\n"
"	}\n"
"	if (info->m_J2linearAxis)\n"
"	{\n"
"		info->m_J2linearAxis[0] = -1;\n"
"		info->m_J2linearAxis[info->rowskip+1] = -1;\n"
"		info->m_J2linearAxis[2*info->rowskip+2] = -1;\n"
"	}\n"
"	\n"
"	float4 a2 = qtRotate(rotB,constraint->m_pivotInB);\n"
"   \n"
"	{\n"
"	//	float4 a2n = -a2;\n"
"		__global float4* angular0 = (__global float4*)(info->m_J2angularAxis);\n"
"		__global float4* angular1 = (__global float4*)(info->m_J2angularAxis+info->rowskip);\n"
"		__global float4* angular2 = (__global float4*)(info->m_J2angularAxis+2*info->rowskip);\n"
"		getSkewSymmetricMatrix(a2,angular0,angular1,angular2);\n"
"	}\n"
"    \n"
"    // set right hand side\n"
"//	float currERP = (m_flags & B3_P2P_FLAGS_ERP) ? m_erp : info->erp;\n"
"	float currERP = info->erp;\n"
"	float k = info->fps * currERP;\n"
"    int j;\n"
"	float4 result = a2 + posB - a1 - posA;\n"
"	float* resultPtr = &result;\n"
"	for (j=0; j<3; j++)\n"
"    {\n"
"        info->m_constraintError[j*info->rowskip] = k * (resultPtr[j]);\n"
"    }\n"
"}\n"
"Quaternion nearest( Quaternion first, Quaternion qd)\n"
"{\n"
"	Quaternion diff,sum;\n"
"	diff = first- qd;\n"
"	sum = first + qd;\n"
"	\n"
"	if( dot(diff,diff) < dot(sum,sum) )\n"
"		return qd;\n"
"	return (-qd);\n"
"}\n"
"float b3Acos(float x) \n"
"{ \n"
"	if (x<-1)	\n"
"		x=-1; \n"
"	if (x>1)	\n"
"		x=1;\n"
"	return acos(x); \n"
"}\n"
"float getAngle(Quaternion orn)\n"
"{\n"
"	if (orn.w>=1.f)\n"
"		orn.w=1.f;\n"
"	float s = 2.f * b3Acos(orn.w);\n"
"	return s;\n"
"}\n"
"void calculateDiffAxisAngleQuaternion( Quaternion orn0,Quaternion orn1a,float4* axis,float* angle)\n"
"{\n"
"	Quaternion orn1 = nearest(orn0,orn1a);\n"
"	\n"
"	Quaternion dorn = qtMul(orn1,qtInvert(orn0));\n"
"	*angle = getAngle(dorn);\n"
"	*axis = (float4)(dorn.x,dorn.y,dorn.z,0.f);\n"
"	\n"
"	//check for axis length\n"
"	float len = dot3F4(*axis,*axis);\n"
"	if (len < FLT_EPSILON*FLT_EPSILON)\n"
"		*axis = (float4)(1,0,0,0);\n"
"	else\n"
"		*axis /= sqrt(len);\n"
"}\n"
"void getInfo2FixedOrientation(__global b3GpuGenericConstraint* constraint,b3GpuConstraintInfo2* info,__global b3RigidBodyCL* bodies, int start_row)\n"
"{\n"
"	Quaternion worldOrnA = bodies[constraint->m_rbA].m_quat;\n"
"	Quaternion worldOrnB = bodies[constraint->m_rbB].m_quat;\n"
"	int s = info->rowskip;\n"
"	int start_index = start_row * s;\n"
"	// 3 rows to make body rotations equal\n"
"	info->m_J1angularAxis[start_index] = 1;\n"
"	info->m_J1angularAxis[start_index + s + 1] = 1;\n"
"	info->m_J1angularAxis[start_index + s*2+2] = 1;\n"
"	if ( info->m_J2angularAxis)\n"
"	{\n"
"		info->m_J2angularAxis[start_index] = -1;\n"
"		info->m_J2angularAxis[start_index + s+1] = -1;\n"
"		info->m_J2angularAxis[start_index + s*2+2] = -1;\n"
"	}\n"
"	\n"
"	float currERP = info->erp;\n"
"	float k = info->fps * currERP;\n"
"	float4 diff;\n"
"	float angle;\n"
"	float4 qrelCur = qtMul(worldOrnA,qtInvert(worldOrnB));\n"
"	\n"
"	calculateDiffAxisAngleQuaternion(constraint->m_relTargetAB,qrelCur,&diff,&angle);\n"
"	diff*=-angle;\n"
"		\n"
"	float* resultPtr = &diff;\n"
"	\n"
"	for (int j=0; j<3; j++)\n"
"    {\n"
"        info->m_constraintError[(3+j)*info->rowskip] = k * resultPtr[j];\n"
"    }\n"
"	\n"
"}\n"
"__kernel void writeBackVelocitiesKernel(__global b3RigidBodyCL* bodies,__global b3GpuSolverBody* solverBodies,int numBodies)\n"
"{\n"
"	int i = get_global_id(0);\n"
"	if (i>=numBodies)\n"
"		return;\n"
"	if (bodies[i].m_invMass)\n"
"	{\n"
"//		if (length(solverBodies[i].m_deltaLinearVelocity)<MOTIONCLAMP)\n"
"		{\n"
"			bodies[i].m_linVel += solverBodies[i].m_deltaLinearVelocity;\n"
"		}\n"
"//		if (length(solverBodies[i].m_deltaAngularVelocity)<MOTIONCLAMP)\n"
"		{\n"
"			bodies[i].m_angVel += solverBodies[i].m_deltaAngularVelocity;\n"
"		} \n"
"	}\n"
"}\n"
"__kernel void getInfo2Kernel(__global b3SolverConstraint* solverConstraintRows, \n"
"							__global unsigned int* infos, \n"
"							__global unsigned int* constraintRowOffsets, \n"
"							__global b3GpuGenericConstraint* constraints, \n"
"							__global b3BatchConstraint* batchConstraints, \n"
"							__global b3RigidBodyCL* bodies,\n"
"							__global BodyInertia* inertias,\n"
"							__global b3GpuSolverBody* solverBodies,\n"
"							float timeStep,\n"
"							float globalErp,\n"
"							float globalCfm,\n"
"							float globalDamping,\n"
"							int globalNumIterations,\n"
"							int numConstraints)\n"
"{\n"
"	int i = get_global_id(0);\n"
"	if (i>=numConstraints)\n"
"		return;\n"
"		\n"
"	//for now, always initialize the batch info\n"
"	int info1 = infos[i];\n"
"			\n"
"	__global b3SolverConstraint* currentConstraintRow = &solverConstraintRows[constraintRowOffsets[i]];\n"
"	__global b3GpuGenericConstraint* constraint = &constraints[i];\n"
"	__global b3RigidBodyCL* rbA = &bodies[ constraint->m_rbA];\n"
"	__global b3RigidBodyCL* rbB = &bodies[ constraint->m_rbB];\n"
"	int solverBodyIdA = constraint->m_rbA;\n"
"	int solverBodyIdB = constraint->m_rbB;\n"
"	__global b3GpuSolverBody* bodyAPtr = &solverBodies[solverBodyIdA];\n"
"	__global b3GpuSolverBody* bodyBPtr = &solverBodies[solverBodyIdB];\n"
"	if (rbA->m_invMass)\n"
"	{\n"
"		batchConstraints[i].m_bodyAPtrAndSignBit = solverBodyIdA;\n"
"	} else\n"
"	{\n"
"//			if (!solverBodyIdA)\n"
"//				m_staticIdx = 0;\n"
"		batchConstraints[i].m_bodyAPtrAndSignBit = -solverBodyIdA;\n"
"	}\n"
"	if (rbB->m_invMass)\n"
"	{\n"
"		batchConstraints[i].m_bodyBPtrAndSignBit = solverBodyIdB;\n"
"	} else\n"
"	{\n"
"//			if (!solverBodyIdB)\n"
"//				m_staticIdx = 0;\n"
"		batchConstraints[i].m_bodyBPtrAndSignBit = -solverBodyIdB;\n"
"	}\n"
"	if (info1)\n"
"	{\n"
"		int overrideNumSolverIterations = 0;//constraint->getOverrideNumSolverIterations() > 0 ? constraint->getOverrideNumSolverIterations() : infoGlobal.m_numIterations;\n"
"//		if (overrideNumSolverIterations>m_maxOverrideNumSolverIterations)\n"
"	//		m_maxOverrideNumSolverIterations = overrideNumSolverIterations;\n"
"		int j;\n"
"		for ( j=0;j<info1;j++)\n"
"		{\n"
"//			memset(&currentConstraintRow[j],0,sizeof(b3SolverConstraint));\n"
"			currentConstraintRow[j].m_angularComponentA = (float4)(0,0,0,0);\n"
"			currentConstraintRow[j].m_angularComponentB = (float4)(0,0,0,0);\n"
"			currentConstraintRow[j].m_appliedImpulse = 0.f;\n"
"			currentConstraintRow[j].m_appliedPushImpulse = 0.f;\n"
"			currentConstraintRow[j].m_cfm = 0.f;\n"
"			currentConstraintRow[j].m_contactNormal = (float4)(0,0,0,0);\n"
"			currentConstraintRow[j].m_friction = 0.f;\n"
"			currentConstraintRow[j].m_frictionIndex = 0;\n"
"			currentConstraintRow[j].m_jacDiagABInv = 0.f;\n"
"			currentConstraintRow[j].m_lowerLimit = 0.f;\n"
"			currentConstraintRow[j].m_upperLimit = 0.f;\n"
"			currentConstraintRow[j].m_originalConstraint = i;\n"
"			currentConstraintRow[j].m_overrideNumSolverIterations = 0;\n"
"			currentConstraintRow[j].m_relpos1CrossNormal = (float4)(0,0,0,0);\n"
"			currentConstraintRow[j].m_relpos2CrossNormal = (float4)(0,0,0,0);\n"
"			currentConstraintRow[j].m_rhs = 0.f;\n"
"			currentConstraintRow[j].m_rhsPenetration = 0.f;\n"
"			currentConstraintRow[j].m_solverBodyIdA = 0;\n"
"			currentConstraintRow[j].m_solverBodyIdB = 0;\n"
"							\n"
"			currentConstraintRow[j].m_lowerLimit = -B3_INFINITY;\n"
"			currentConstraintRow[j].m_upperLimit = B3_INFINITY;\n"
"			currentConstraintRow[j].m_appliedImpulse = 0.f;\n"
"			currentConstraintRow[j].m_appliedPushImpulse = 0.f;\n"
"			currentConstraintRow[j].m_solverBodyIdA = solverBodyIdA;\n"
"			currentConstraintRow[j].m_solverBodyIdB = solverBodyIdB;\n"
"			currentConstraintRow[j].m_overrideNumSolverIterations = overrideNumSolverIterations;		\n"
"		}\n"
"		bodyAPtr->m_deltaLinearVelocity = (float4)(0,0,0,0);\n"
"		bodyAPtr->m_deltaAngularVelocity = (float4)(0,0,0,0);\n"
"		bodyAPtr->m_pushVelocity = (float4)(0,0,0,0);\n"
"		bodyAPtr->m_turnVelocity = (float4)(0,0,0,0);\n"
"		bodyBPtr->m_deltaLinearVelocity = (float4)(0,0,0,0);\n"
"		bodyBPtr->m_deltaAngularVelocity = (float4)(0,0,0,0);\n"
"		bodyBPtr->m_pushVelocity = (float4)(0,0,0,0);\n"
"		bodyBPtr->m_turnVelocity  = (float4)(0,0,0,0);\n"
"		int rowskip = sizeof(b3SolverConstraint)/sizeof(float);//check this\n"
"		\n"
"		b3GpuConstraintInfo2 info2;\n"
"		info2.fps = 1.f/timeStep;\n"
"		info2.erp = globalErp;\n"
"		info2.m_J1linearAxisFloat4 = &currentConstraintRow->m_contactNormal;\n"
"		info2.m_J1angularAxisFloat4 = &currentConstraintRow->m_relpos1CrossNormal;\n"
"		info2.m_J2linearAxisFloat4 = 0;\n"
"		info2.m_J2angularAxisFloat4 = &currentConstraintRow->m_relpos2CrossNormal;\n"
"		info2.rowskip = sizeof(b3SolverConstraint)/sizeof(float);//check this\n"
"		///the size of b3SolverConstraint needs be a multiple of float\n"
"//		b3Assert(info2.rowskip*sizeof(float)== sizeof(b3SolverConstraint));\n"
"		info2.m_constraintError = &currentConstraintRow->m_rhs;\n"
"		currentConstraintRow->m_cfm = globalCfm;\n"
"		info2.m_damping = globalDamping;\n"
"		info2.cfm = &currentConstraintRow->m_cfm;\n"
"		info2.m_lowerLimit = &currentConstraintRow->m_lowerLimit;\n"
"		info2.m_upperLimit = &currentConstraintRow->m_upperLimit;\n"
"		info2.m_numIterations = globalNumIterations;\n"
"		switch (constraint->m_constraintType)\n"
"		{\n"
"			case B3_GPU_POINT2POINT_CONSTRAINT_TYPE:\n"
"			{\n"
"				getInfo2Point2Point(constraint,&info2,bodies);\n"
"				break;\n"
"			}\n"
"			case B3_GPU_FIXED_CONSTRAINT_TYPE:\n"
"			{\n"
"				getInfo2Point2Point(constraint,&info2,bodies);\n"
"				getInfo2FixedOrientation(constraint,&info2,bodies,3);\n"
"				break;\n"
"			}\n"
"			default:\n"
"			{\n"
"			}\n"
"		}\n"
"		///finalize the constraint setup\n"
"		for ( j=0;j<info1;j++)\n"
"		{\n"
"			__global b3SolverConstraint* solverConstraint = &currentConstraintRow[j];\n"
"			if (solverConstraint->m_upperLimit>=constraint->m_breakingImpulseThreshold)\n"
"			{\n"
"				solverConstraint->m_upperLimit = constraint->m_breakingImpulseThreshold;\n"
"			}\n"
"			if (solverConstraint->m_lowerLimit<=-constraint->m_breakingImpulseThreshold)\n"
"			{\n"
"				solverConstraint->m_lowerLimit = -constraint->m_breakingImpulseThreshold;\n"
"			}\n"
"//						solverConstraint->m_originalContactPoint = constraint;\n"
"							\n"
"			Matrix3x3 invInertiaWorldA= inertias[constraint->m_rbA].m_invInertiaWorld;\n"
"			{\n"
"				//float4 angularFactorA(1,1,1);\n"
"				float4 ftorqueAxis1 = solverConstraint->m_relpos1CrossNormal;\n"
"				solverConstraint->m_angularComponentA = mtMul1(invInertiaWorldA,ftorqueAxis1);//*angularFactorA;\n"
"			}\n"
"						\n"
"			Matrix3x3 invInertiaWorldB= inertias[constraint->m_rbB].m_invInertiaWorld;\n"
"			{\n"
"				float4 ftorqueAxis2 = solverConstraint->m_relpos2CrossNormal;\n"
"				solverConstraint->m_angularComponentB = mtMul1(invInertiaWorldB,ftorqueAxis2);//*constraint->m_rbB.getAngularFactor();\n"
"			}\n"
"			{\n"
"				//it is ok to use solverConstraint->m_contactNormal instead of -solverConstraint->m_contactNormal\n"
"				//because it gets multiplied iMJlB\n"
"				float4 iMJlA = solverConstraint->m_contactNormal*rbA->m_invMass;\n"
"				float4 iMJaA = mtMul3(solverConstraint->m_relpos1CrossNormal,invInertiaWorldA);\n"
"				float4 iMJlB = solverConstraint->m_contactNormal*rbB->m_invMass;//sign of normal?\n"
"				float4 iMJaB = mtMul3(solverConstraint->m_relpos2CrossNormal,invInertiaWorldB);\n"
"				float sum = dot3F4(iMJlA,solverConstraint->m_contactNormal);\n"
"				sum += dot3F4(iMJaA,solverConstraint->m_relpos1CrossNormal);\n"
"				sum += dot3F4(iMJlB,solverConstraint->m_contactNormal);\n"
"				sum += dot3F4(iMJaB,solverConstraint->m_relpos2CrossNormal);\n"
"				float fsum = fabs(sum);\n"
"				if (fsum>FLT_EPSILON)\n"
"				{\n"
"					solverConstraint->m_jacDiagABInv = 1.f/sum;\n"
"				} else\n"
"				{\n"
"					solverConstraint->m_jacDiagABInv = 0.f;\n"
"				}\n"
"			}\n"
"			///fix rhs\n"
"			///todo: add force/torque accelerators\n"
"			{\n"
"				float rel_vel;\n"
"				float vel1Dotn = dot3F4(solverConstraint->m_contactNormal,rbA->m_linVel) + dot3F4(solverConstraint->m_relpos1CrossNormal,rbA->m_angVel);\n"
"				float vel2Dotn = -dot3F4(solverConstraint->m_contactNormal,rbB->m_linVel) + dot3F4(solverConstraint->m_relpos2CrossNormal,rbB->m_angVel);\n"
"				rel_vel = vel1Dotn+vel2Dotn;\n"
"				float restitution = 0.f;\n"
"				float positionalError = solverConstraint->m_rhs;//already filled in by getConstraintInfo2\n"
"				float	velocityError = restitution - rel_vel * info2.m_damping;\n"
"				float	penetrationImpulse = positionalError*solverConstraint->m_jacDiagABInv;\n"
"				float	velocityImpulse = velocityError *solverConstraint->m_jacDiagABInv;\n"
"				solverConstraint->m_rhs = penetrationImpulse+velocityImpulse;\n"
"				solverConstraint->m_appliedImpulse = 0.f;\n"
"			}\n"
"		}\n"
"	}\n"
"}\n"
;
