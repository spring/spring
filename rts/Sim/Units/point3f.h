#ifndef POINT3F_H
#define POINT3F_H


#include <Eigen/Dense>

using namespace Eigen;


#ifdef _WIN32
	//#define EIGEN_DONT_ALIGN_STATICALLY
	#define EIGEN_DONT_VECTORIZE
	#define EIGEN_DISABLE_UNALIGNED_ARRAY_ASSERT
#endif


typedef Vector3f Point3f;

#endif