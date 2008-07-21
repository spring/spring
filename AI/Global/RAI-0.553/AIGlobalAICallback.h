/*
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

#ifndef _AIGLOBALAICALLBACK_H
#define	_AIGLOBALAICALLBACK_H

#include "ExternalAI/SAICallback.h"
#include "ExternalAI/IGlobalAICallback.h"

#include "AIAICallback.h"
#include "AIAICheats.h"

/**
 * The AI side wrapper over the C AI interface for IGlobalAICallback.
 */
class CAIGlobalAICallback : public IGlobalAICallback {
public:
    CAIGlobalAICallback();
    CAIGlobalAICallback(SAICallback* sAICallback, int team/*, IGlobalAICallback* globalAICallback*/);
    ~CAIGlobalAICallback();

    virtual IAICheats* GetCheatInterface();
    virtual IAICallback* GetAICallback();
    
private:
    SAICallback* sAICallback;
    int team;
//    IGlobalAICallback* globalAICallback; // deprecated
    CAIAICallback* wrappedAICallback;
    CAIAICheats* wrappedAICheats;
};

#endif	/* _AIGLOBALAICALLBACK_H */

