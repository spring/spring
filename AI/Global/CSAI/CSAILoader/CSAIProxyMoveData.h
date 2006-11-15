// Copyright Hugh Perkins 2006
// hughperkins@gmail.com http://manageddreams.com
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURVector3E. See the GNU General Public License for
//  more details.
//
// You should have received a copy of the GNU General Public License along
// with this program in the file licence.txt; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-
// 1307 USA
// You can find the licence also on the web at:
// http://www.opensource.org/licenses/gpl-license.php
//
// ======================================================================================

#ifndef _MOVEDATAPROXY_H
#define _MOVEDATAPROXY_H

#using <mscorlib.dll>
#include <vcclr.h>
using namespace System;
using namespace System::Collections;
using namespace System::Runtime::InteropServices;    

#include "Sim/MoveTypes/MoveInfo.h"

#include "AbicMoveDataWrapper.h"

__gc class MoveDataProxy : public CSharpAI::IMoveData
{    
public:
    AbicMoveDataWrapper *self;

    MoveDataProxy( const AbicMoveDataWrapper *actualmovedata )
    {
        this->self = ( AbicMoveDataWrapper *) actualmovedata;
    }
    
    CSharpAI::MoveType get_moveType(){ return (CSharpAI::MoveType)self->get_moveType(); }
   
    #include "CSAIProxyIMoveData_generated.h"
/*
	int get_size(){ return actualmovedata->get_size(); }
	double get_depth(){ return actualmovedata->get_depth(); }
	double get_maxSlope(){ return actualmovedata->get_maxSlope(); }
	double get_slopeMod(){ return actualmovedata->get_slopeMod(); }
	double get_depthMod(){ return actualmovedata->get_depthMod(); }

	int get_pathType(){ return actualmovedata->get_pathType(); }
	//CMoveMath* moveMath;
	double get_crushStrength(){ return actualmovedata->get_crushStrength(); }
	int get_moveFamily(){ return actualmovedata->get_moveFamily(); }
    */
};

#endif // _MOVEDATAPROXY_H
