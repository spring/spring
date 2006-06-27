#include "EData.h"
EData::~EData() {
}
void EData::Init(CEvent e) {
}
CEvent foo;
CEvent EData::GetEvent() {
    return foo;
}
EData* CEvent::GetData() {
    return 0;
}

