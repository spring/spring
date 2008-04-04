#include "UnpackPacket.h"

namespace netcode
{

UnpackPacket::UnpackPacket(const RawPacket& packet) : pckt(packet), pos(0)
{
}

}
