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
//

// this class wraps the ABIC UnitDef functions

#ifndef _ABICUNITDEFWRAPPER_H
#define _ABICUNITDEFWRAPPER_H

#include "ExternalAI/GlobalAICInterface/AbicAICallback.h"

#include "AbicMoveDataWrapper.h"

class AbicUnitDefWrapper
{        
public:
    UnitDef *self;
    AbicUnitDefWrapper( const UnitDef *self )
    {
        this->self = ( UnitDef *)self;
    }
    
    AbicMoveDataWrapper *get_movedata()
    {
        const MoveData *movedata = UnitDef_get_movedata( self );
        if( movedata == 0 )
        {
            return 0;
        }
        return new AbicMoveDataWrapper( movedata );
    }
    
    #include "AbicIUnitDefWrapper_generated.h"
};

#endif
