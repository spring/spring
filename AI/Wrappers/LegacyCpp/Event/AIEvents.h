/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_EVENTS_H
#define _AI_EVENTS_H

#include "../AIAI.h"

#include "ExternalAI/Interface/AISEvents.h"
#include "ExternalAI/Interface/AISCommands.h"
#include "ExternalAI/Interface/SSkirmishAICallback.h"

#include "AIEvent.h"
#include "AINullEvent.h"

#include "AIInitEvent.h"
#include "AIReleaseEvent.h"
#include "AIUpdateEvent.h"

#include "AIChatMessageEvent.h"
#include "AILuaMessageEvent.h"

#include "AIUnitCreatedEvent.h"
#include "AIUnitFinishedEvent.h"
#include "AIUnitIdleEvent.h"
#include "AIUnitMoveFailedEvent.h"
#include "AIUnitDamagedEvent.h"
#include "AIUnitDestroyedEvent.h"
#include "AIUnitGivenEvent.h"
#include "AIUnitCapturedEvent.h"

#include "AIEnemyCreatedEvent.h"
#include "AIEnemyFinishedEvent.h"
#include "AIEnemyEnterLOSEvent.h"
#include "AIEnemyLeaveLOSEvent.h"
#include "AIEnemyEnterRadarEvent.h"
#include "AIEnemyLeaveRadarEvent.h"
#include "AIEnemyDamagedEvent.h"
#include "AIEnemyDestroyedEvent.h"

#include "AIWeaponFiredEvent.h"
#include "AIPlayerCommandEvent.h"
#include "AISeismicPingEvent.h"

#endif // _AI_EVENTS_H
