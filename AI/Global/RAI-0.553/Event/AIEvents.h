/*
    Copyright 2008  Nicolas Wu
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef AIEVENTS_H
#define AIEVENTS_H

#include "../AI.h"
#include "../AIGlobalAI.h"

#include "ExternalAI/AISEvents.h"

#include "AIEvent.h"
#include "AINullEvent.h"

#include "AIInitEvent.h"
#include "AIUpdateEvent.h"

#include "AIMessageEvent.h"

#include "AIUnitCreatedEvent.h"
#include "AIUnitFinishedEvent.h"
#include "AIUnitIdleEvent.h"
#include "AIUnitMoveFailedEvent.h"
#include "AIUnitDamagedEvent.h"
#include "AIUnitDestroyedEvent.h"
#include "AIUnitGivenEvent.h"
#include "AIUnitCapturedEvent.h"

#include "AIEnemyEnterLOSEvent.h"
#include "AIEnemyLeaveLOSEvent.h"
#include "AIEnemyEnterRadarEvent.h"
#include "AIEnemyLeaveRadarEvent.h"
#include "AIEnemyDamagedEvent.h"
#include "AIEnemyDestroyedEvent.h"

#include "AIWeaponFiredEvent.h"
#include "AIPlayerCommandEvent.h"
#include "AISeismicPingEvent.h"

#endif /*AIEVENTS_H*/
