#ifndef PATHPERFORMANCE_H
#define PATHPERFORMANCE_H
// PathPerformance.h: interface for the CPathPerformance class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PATHPERFORMANCE_H__230DE926_8743_11D5_AA64_CEDE6FBAB037__INCLUDED_)
#define AFX_PATHPERFORMANCE_H__230DE926_8743_11D5_AA64_CEDE6FBAB037__INCLUDED_

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

#include "Script.h"

class CPathPerformace : public CScript  
{
public:
	virtual void SetCamera();
	virtual void Update();
	CPathPerformace();
	virtual ~CPathPerformace();

	CGroundVehicle* vecs[400]; 

};
#endif // !defined(AFX_PATHPERFORMANCE_H__230DE926_8743_11D5_AA64_CEDE6FBAB037__INCLUDED_)


#endif /* PATHPERFORMANCE_H */
