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

#ifndef _FEATUREDEFFORCSHARP_H
#define _FEATUREDEFFORCSHARP_H

#using <mscorlib.dll>
#include <vcclr.h>
using namespace System;
using namespace System::Collections;
using namespace System::Runtime::InteropServices;    

#include "Sim/Features/FeatureDef.h"

#include "AbicFeatureDefWrapper.h"

__gc class FeatureDefForCSharp : public CSharpAI::IFeatureDef
{    
public:
    AbicFeatureDefWrapper *self;

    FeatureDefForCSharp( const AbicFeatureDefWrapper *actualfeaturedef )
    {
        this->self = ( AbicFeatureDefWrapper *) actualfeaturedef;
    }

    #include "CSAIProxyIFeatureDef_generated.h"
    
    /*
    System::String *get_myName(){ return new System::String( actualfeaturedef->myName.c_str() ); }
    System::String *get_description(){ return new System::String( actualfeaturedef->description.c_str() ); }

    double get_metal(){ return actualfeaturedef->metal; }
    double get_energy(){ return actualfeaturedef->energy; }
    double get_maxHealth(){ return actualfeaturedef->maxHealth; }

    double get_radius(){ return actualfeaturedef->radius; }
    double get_mass(){ return actualfeaturedef->mass; }									//used to see if the object can be overrun

    bool get_upright(){ return actualfeaturedef->upright; }
    int get_drawType(){ return actualfeaturedef->drawType; }
    //S3DOModel* model;						//used by 3do obects
    //std::string modelname;			//used by 3do obects
    //int modelType;							//used by tree etc

    bool get_destructable(){ return actualfeaturedef->destructable; }
    bool get_blocking(){ return actualfeaturedef->blocking; }
    bool get_burnable(){ return actualfeaturedef->burnable; }
    bool get_floating(){ return actualfeaturedef->floating; }

    bool get_geoThermal(){ return actualfeaturedef->geoThermal; }

    System::String *get_deathFeature(){ return new System::String( actualfeaturedef->deathFeature.c_str() ); }		//name of feature that this turn into when killed (not reclaimed)

    int get_xsize(){ return actualfeaturedef->xsize; }									//each size is 8 units
    int get_ysize(){ return actualfeaturedef->ysize; }	
    
    // Following is auto-generated (so none):
    */
};

#endif // _FEATUREDEFFORCSHARP_H
