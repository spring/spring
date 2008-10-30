#include "UnpackPacket.h"

namespace netcode
{

UnpackPacket::UnpackPacket(boost::shared_ptr<const RawPacket> packet, unsigned skipBytes) : pckt(packet), pos(skipBytes)
{
}

}
