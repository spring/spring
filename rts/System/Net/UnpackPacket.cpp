#include "UnpackPacket.h"

namespace netcode
{

UnpackPacket::UnpackPacket(const unsigned char* const data, const unsigned length) : RawPacket(data, length), pos(0)
{
}

}

