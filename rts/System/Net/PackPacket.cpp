#include "PackPacket.h"

namespace netcode
{

PackPacket::PackPacket(const unsigned length) : RawPacket(length), pos(0)
{
}

}

