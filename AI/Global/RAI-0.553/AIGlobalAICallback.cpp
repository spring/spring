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

#include "AIGlobalAICallback.h"
#include "AIAICallback.h"
#include "AIAICheats.h"

CAIGlobalAICallback::CAIGlobalAICallback()
    : IGlobalAICallback(), sAICallback(NULL), team(-1),
        wrappedAICallback(NULL), wrappedAICheats(NULL) {
    
}

CAIGlobalAICallback::CAIGlobalAICallback(SAICallback* sAICallback, int team)
    : IGlobalAICallback(), sAICallback(sAICallback), team(team),
        wrappedAICallback(NULL), wrappedAICheats(NULL) {
    
}

CAIGlobalAICallback::~CAIGlobalAICallback() {
    
}


IAICallback* CAIGlobalAICallback::GetAICallback() {
    
    if (wrappedAICallback == NULL) {
        wrappedAICallback = new CAIAICallback(team, sAICallback/*, globalAICallback->GetAICallback()*/);
    }
    return wrappedAICallback;
}

IAICheats* CAIGlobalAICallback::GetCheatInterface() {
    
    if (wrappedAICheats == NULL) {
        this->GetAICallback(); // to initialize
        wrappedAICheats = new CAIAICheats(team, sAICallback/*, globalAICallback->GetCheatInterface()*/, wrappedAICallback);
    }
    return wrappedAICheats;
}
