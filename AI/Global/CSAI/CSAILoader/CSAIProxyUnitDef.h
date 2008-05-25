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

#ifndef _UNITDEFFORCSHARP_H
#define _UNITDEFFORCSHARP_H

#using <mscorlib.dll>
#include <vcclr.h>
#include <string>
using namespace System;
using namespace System::Collections;
using namespace System::Runtime::InteropServices;    

#include "Sim/Units/UnitDef.h"

#include "CSAIProxyMoveData.h"

#include "AbicUnitDefWrapper.h"
#include "AbicMoveDataWrapper.h"

// This is a managed class that we can use to pass unitdefs to C#
// Since unitdefs are very large, and often passed in arrays, we dont copy by value
// but simply wrap a pointer to the original C++ unitdef,
// then we copy out the data on demand, via the get accessor functions
//
// This class derives from a C# class IUnitDef, so you'll need to update the IUnitDef class as well (in the C# code)
// for any additional accessors to work
//
// The function names are systematically get_<property name>  This is automatically defined by C# for any property
//
// This file is partially-generated using BuildTools\GenerateUnitDefClasses.exe
//
__gc class UnitDefForCSharp : public CSharpAI::IUnitDef
{    
public:
    AbicUnitDefWrapper *self;

    UnitDefForCSharp( const AbicUnitDefWrapper *actualunitdef )
    {
        this->self = ( AbicUnitDefWrapper * )actualunitdef;
    }
    
    // please use GetBuildOption( const UnitDef *self, int index ); // assumes map is really a vector where the int is a contiguous index starting from 1
    //CSharpAI::BuildOption *get_buildOptions()[]
    //{
      //  ArrayList *buildoptionarraylist = new ArrayList();
        //for(map<int, string>::const_iterator j = actualunitdef->buildOptions.begin(); j != actualunitdef->buildOptions.end(); j++)
        //{
          //  CSharpAI::BuildOption *buildoption = new CSharpAI::BuildOption( j->first, new System::String( j->second.c_str() ) );
            //buildoptionarraylist->Add( buildoption );
		//}
       // 
        //return dynamic_cast< CSharpAI::BuildOption*[] >( buildoptionarraylist->ToArray( __typeof( CSharpAI::BuildOption ) ) );
    //}
    
    CSharpAI::IMoveData *get_movedata()
    {
        if( self->get_movedata() != 0 )
        {
            return new MoveDataProxy( self->get_movedata() ); 
        }
        return 0;
    }
    
    #include "CSAIProxyIUnitDef_generated.h"
};

#endif
