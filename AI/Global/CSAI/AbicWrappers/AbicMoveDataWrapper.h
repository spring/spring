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

// this class wraps the ABIC MoveData functions

#ifndef _ABICMOVEDATAWRAPPER_H
#define _ABICMOVEDATAWRAPPER_H

#include "ExternalAI/GlobalAICInterface/AbicAICallback.h"

class AbicMoveDataWrapper
{        
public:
    MoveData *self;
    AbicMoveDataWrapper( const MoveData *self )
    {
        this->self = ( MoveData *)self;
    }

   int get_moveType()
   {
      return MoveData_get_movetype( self );
   }
    
    #include "AbicIMoveDataWrapper_generated.h"
};

#endif
