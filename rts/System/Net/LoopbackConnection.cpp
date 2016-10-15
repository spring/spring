/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LoopbackConnection.h"

namespace netcode {

CLoopbackConnection::CLoopbackConnection() {
}

CLoopbackConnection::~CLoopbackConnection() {
}

void CLoopbackConnection::SendData(std::shared_ptr<const RawPacket> data) {
	Data.push_back(data);
}

std::shared_ptr<const RawPacket> CLoopbackConnection::Peek(unsigned ahead) const {
	if (ahead < Data.size())
		return Data[ahead];
	std::shared_ptr<const RawPacket> empty;
	return empty;
}

void CLoopbackConnection::DeleteBufferPacketAt(unsigned index) {
	if (index < Data.size())
		Data.erase(Data.begin() + index);
}

std::shared_ptr<const RawPacket> CLoopbackConnection::GetData() {
	if (!Data.empty()) {
		std::shared_ptr<const RawPacket> next = Data.front();
		Data.pop_front();
		return next;
	}
	std::shared_ptr<const RawPacket> empty;
	return empty;
}

void CLoopbackConnection::Flush(const bool forced) {
}

bool CLoopbackConnection::CheckTimeout(int seconds, bool initial) const {
	return false;
}

bool CLoopbackConnection::CanReconnect() const {
	return false;
}

bool CLoopbackConnection::NeedsReconnect() {
	return false;
}

std::string CLoopbackConnection::Statistics() const {
	return "Statistics for loopback connection: N/A";
}

std::string CLoopbackConnection::GetFullAddress() const {
	return "Loopback";
}

bool CLoopbackConnection::HasIncomingData() const {
	return !Data.empty();
}

} // namespace netcode

