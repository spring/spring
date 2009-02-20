#include "UnpackPacket.h"

namespace netcode
{

UnpackPacket::UnpackPacket(boost::shared_ptr<const RawPacket> packet, size_t skipBytes) : pckt(packet), pos(skipBytes)
{
}

}
