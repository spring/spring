#include "UnpackPacket.h"

namespace netcode
{

UnpackPacket::UnpackPacket(boost::shared_ptr<const RawPacket> packet) : pckt(packet), pos(0)
{
}

}
