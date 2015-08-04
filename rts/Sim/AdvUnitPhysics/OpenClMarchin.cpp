//This is the A.U.P. Core - here the new objects/forces are pushed into the physics
//and the recived Move/Turn Input is transfered back into engine-moves and turns



#ifdef _WIN32
#include <GL/glew.h>
#endif

#ifndef USE_MINICL
#define USE_SIMDAWARE_SOLVER
#endif

#if !defined (__APPLE__)
#define USE_GPU_SOLVER
#if defined (_WIN32)  &&  !defined(USE_MINICL)
	#define USE_GPU_COPY //only tested on Windows
#endif //_WIN32 && !USE_MINICL
#endif //!__APPLE__ 

//Vital Includes - check for desync
#include “btBulletDynamicsCommon.h"
#include "LinearMath/btHashMap.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "clstuff.h"
#include "vectormath/vmInclude.h" 


#include "BulletMultiThreaded/GpuSoftBodySolvers/OpenCL/btSoftBodySolver_OpenCL.h"
#include "BulletMultiThreaded/GpuSoftBodySolvers/OpenCL/btSoftBodySolver_OpenCLSIMDAware.h"
#include "BulletMultiThreaded/GpuSoftBodySolvers/OpenCL/btSoftBodySolverVertexBuffer_OpenGL.h"
#include "BulletMultiThreaded/GpuSoftBodySolvers/OpenCL/btSoftBodySolverOutputCLtoGL.h"

#include "gl_win.h"
#include "cloth.h"

#include "../OpenGL/GLDebugDrawer.h"

GLDebugDrawer debugDraw;






void initBullet(void)
{

#ifdef USE_GPU_SOLVER
#ifdef USE_SIMDAWARE_SOLVER
	g_openCLSIMDSolver = new btOpenCLSoftBodySolverSIMDAware( g_cqCommandQue, g_cxMainContext);
	g_solver = g_openCLSIMDSolver;
#ifdef USE_GPU_COPY
	g_softBodyOutput = new btSoftBodySolverOutputCLtoGL(g_cqCommandQue, g_cxMainContext);
#else // #ifdef USE_GPU_COPY
	g_softBodyOutput = new btSoftBodySolverOutputCLtoCPU;
#endif // #ifdef USE_GPU_COPY
#else
	g_openCLSolver = new btOpenCLSoftBodySolver( g_cqCommandQue, g_cxMainContext );
	g_solver = g_openCLSolver;
#ifdef USE_GPU_COPY
	g_softBodyOutput = new btSoftBodySolverOutputCLtoGL(g_cqCommandQue, g_cxMainContext);
#else // #ifdef USE_GPU_COPY
	g_softBodyOutput = new btSoftBodySolverOutputCLtoCPU;
#endif // #ifdef USE_GPU_COPY
#endif
#else
	g_openCLSolver = new btOpenCLSoftBodySolver( g_cqCommandQue, g_cxMainContext );
	g_solver = g_openCLSolver;
#endif

	//m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_collisionConfiguration = new btSoftBodyRigidBodyCollisionConfiguration();
	

	m_dispatcher = new	btCollisionDispatcher(m_collisionConfiguration);
	m_broadphase = new btDbvtBroadphase();
	btSequentialImpulseConstraintSolver* sol = new btSequentialImpulseConstraintSolver;
	m_solver = sol;

	m_dynamicsWorld = new btSoftRigidDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfiguration, g_solver);	

	m_dynamicsWorld->setGravity(btVector3(0,-10,0));	
	btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(50.),btScalar(50.),btScalar(50.)));	
	m_collisionShapes.push_back(groundShape);
	btTransform groundTransform;
	groundTransform.setIdentity();
	groundTransform.setOrigin(btVector3(0,-50,0));

	




	m_dynamicsWorld->getWorldInfo().air_density			=	(btScalar)1.2;
	m_dynamicsWorld->getWorldInfo().water_density		=	0;
	m_dynamicsWorld->getWorldInfo().water_offset		=	0;
	m_dynamicsWorld->getWorldInfo().water_normal		=	btVector3(0,0,0);
	m_dynamicsWorld->getWorldInfo().m_gravity.setValue(0,-10,0);



#if 0
	{
		btScalar mass(0.);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			groundShape->calculateLocalInertia(mass,localInertia);

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,groundShape,localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		//add the body to the dynamics world
		m_dynamicsWorld->addRigidBody(body);
	}
 
#endif

	#if 1
	{		
		btScalar mass(0.);

		//btScalar mass(1.);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);
		
		btCollisionShape *capsuleShape = new btCapsuleShape(5, 10);
		capsuleShape->setMargin( 0.5 );
		
		


		btVector3 localInertia(0,0,0);
		if (isDynamic)
			capsuleShape->calculateLocalInertia(mass,localInertia);

		m_collisionShapes.push_back(capsuleShape);
		btTransform capsuleTransform;
		capsuleTransform.setIdentity();
#ifdef TABLETEST
		capsuleTransform.setOrigin(btVector3(0, 10, -11));
		const btScalar pi = 3.141592654;
		capsuleTransform.setRotation(btQuaternion(0, 0, pi/2));
#else
		capsuleTransform.setOrigin(btVector3(0, 0, 0));
		
	//	const btScalar pi = 3.141592654;
		//capsuleTransform.setRotation(btQuaternion(0, 0, pi/2));
		capsuleTransform.setRotation(btQuaternion(0, 0, 0));
#endif
		btDefaultMotionState* myMotionState = new btDefaultMotionState(capsuleTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,capsuleShape,localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		body->setFriction( 0.8f );

		m_dynamicsWorld->addRigidBody(body);
		//cap_1.collisionShape = body;
		capCollider = body;
	}
#endif


//#ifdef USE_GPU_SOLVER
	createFlag( *g_openCLSolver, clothWidth, clothHeight, m_flags );
//#else
	
//#endif

	// Create output buffer descriptions for ecah flag
	// These describe where the simulation should send output data to
	for( int flagIndex = 0; flagIndex < m_flags.size(); ++flagIndex )
	{		
//		m_flags[flagIndex]->setWindVelocity( Vectormath::Aos::Vector3( 0.f, 0.f, 15.f ) );
		
		// In this case we have a DX11 output buffer with a vertex at index 0, 8, 16 and so on as well as a normal at 3, 11, 19 etc.
		// Copies will be performed GPU-side directly into the output buffer

#ifdef USE_GPU_COPY
		GLuint targetVBO = cloths[flagIndex].getVBO();
		btOpenGLInteropVertexBufferDescriptor *vertexBufferDescriptor = new btOpenGLInteropVertexBufferDescriptor(g_cqCommandQue, g_cxMainContext, targetVBO, 0, 8, 3, 8);
#else
		btCPUVertexBufferDescriptor *vertexBufferDescriptor = new btCPUVertexBufferDescriptor(reinterpret_cast< float* >(cloths[flagIndex].cpu_buffer), 0, 8, 3, 8);
#endif
		cloths[flagIndex].m_vertexBufferDescriptor = vertexBufferDescriptor;
	}


	g_solver->optimize( m_dynamicsWorld->getSoftBodyArray() );
	
	if (!g_solver->checkInitialized())
	{
		printf("OpenCL kernel initialization ?failed\n");
		btAssert(0);
		exit(0);
	}

}




//Init::OpenCL SetUp the OpenCL Bullet Engine
void InitEngine()
{
	#ifdef _WIN32
	glewInit();
#endif

#ifdef USE_GPU_COPY
#ifdef _WIN32
    HGLRC glCtx = wglGetCurrentContext();
#else //!_WIN32
    GLXContext glCtx = glXGetCurrentContext();
#endif //!_WIN32
	HDC glDC = wglGetCurrentDC();
	
	initCL(glCtx, glDC);
#else

	initCL();

#endif
	initBullet();
	m_dynamicsWorld->stepSimulation(1./60.,0);
}

//clean up after oneself
void undo()
{

	if( g_openCLSolver  )
		delete g_openCLSolver;
	if( g_openCLSIMDSolver  )
		delete g_openCLSIMDSolver;
	if( g_softBodyOutput )
		delete g_softBodyOutput;
	
}

//Init::World Here we set up the Heightmap, and basicForces
void InitWorld()
{
	
	
}


//
void Main()
{
//Send all new PiecesSimulation
	
	
}

void updatePhysicsWorld()
{
	static int counter = 1;

	// Change wind velocity a bit based on a frame counter
	if( (counter % 400) == 0 )
	{
		_windAngle = (_windAngle + 0.05f);
		if( _windAngle > (2*3.141) )
			_windAngle = 0;

		for( int flagIndex = 0; flagIndex < m_flags.size(); ++flagIndex )
		{		
			btSoftBody *cloth = 0;

			cloth = m_flags[flagIndex];

			float localWind = _windAngle + 0.5*(((float(rand())/RAND_MAX))-0.1);
			float xCoordinate = cos(localWind)*_windStrength;
			float zCoordinate = sin(localWind)*_windStrength;

			cloth->setWindVelocity( btVector3(xCoordinate, 0, zCoordinate) );
		}
	}

	//btVector3 origin( capCollider->getWorldTransform().getOrigin() );
	//origin.setX( origin.getX() + 0.05 );
	//capCollider->getWorldTransform().setOrigin( origin );
	
	counter++;
}
