#ifdef _MSC_VER
#	include "StdAfx.h"
#elif defined(_WIN32)
#	include <windows.h>
#endif

#include "UDPConnection.h"


#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>

#include "mmgr.h"

#include "Socket.h"
#include "ProtocolDef.h"
#include "Exception.h"
#include "ConfigHandler.h"
#include <boost/cstdint.hpp>

namespace netcode {
using namespace boost::asio;

const unsigned UDPMaxPacketSize = 4096;
const int MaxChunkSize = 254;

class Unpacker
{
public:
	Unpacker(const unsigned char* _data, unsigned _length) : data(_data), length(_length)
	{
		pos = 0;
	};
	
	template<typename T>
	void Unpack(T& t)
	{
		assert(length >= pos + sizeof(t));
		t = *reinterpret_cast<const T*>(data+pos);
		pos += sizeof(t);
	};
	void Unpack(std::vector<uint8_t>& t, unsigned _length)
	{
		std::copy(data +pos, data +pos+_length, std::back_inserter(t));
		pos+= _length;
	};

	unsigned Remaining() const
	{
		return length - std::min(pos, length);
	};
private:
	const unsigned char* data;
	unsigned length;
	unsigned pos;
};

class Packer
{
public:
	Packer(std::vector<uint8_t>& _data) : data(_data)
	{
		assert(data.size() == 0);
	};
	
	template<typename T>
	void Pack(T& t)
	{
		const size_t pos = data.size();
		data.resize(pos + sizeof(T));
		*reinterpret_cast<T*>(&data[pos]) = t;
	};
	void Pack(std::vector<uint8_t>& _data)
	{
		std::copy(_data.begin(), _data.end(), std::back_inserter(data));
	};

private:
	std::vector<uint8_t>& data;
};

Packet::Packet(const unsigned char* data, unsigned length)
{
	Unpacker buf(data, length);
	buf.Unpack(lastContinuous);
	buf.Unpack(nakType);
	
	if (nakType > 0)
	{
		naks.reserve(nakType);
		for (int i = 0; i != nakType; ++i)
		{
			if (buf.Remaining() >= sizeof(naks[i]))
				buf.Unpack(naks[i]);
			else
				break;
		}
	}

	while (buf.Remaining() > Chunk::headerSize)
	{
		ChunkPtr temp(new Chunk);
		buf.Unpack(temp->chunkNumber);
		buf.Unpack(temp->chunkSize);
		if (buf.Remaining() >= temp->chunkSize)
		{
			buf.Unpack(temp->data, temp->chunkSize);
			chunks.push_back(temp);
		}
		else
		{
			// defective, ignore
			break;
		}
	}
}

Packet::Packet(int _lastContinuous, int _nak) : lastContinuous(_lastContinuous), nakType(_nak)
{
}

void Packet::Serialize(std::vector<uint8_t>& data)
{
	data.reserve(GetSize());
	Packer buf(data);
	buf.Pack(lastContinuous);
	buf.Pack(nakType);
	buf.Pack(naks);
	for (std::list<ChunkPtr>::const_iterator it = chunks.begin(); it != chunks.end(); ++it)
	{
		buf.Pack((*it)->chunkNumber);
		buf.Pack((*it)->chunkSize);
		buf.Pack((*it)->data);
	}
}

UDPConnection::UDPConnection(boost::shared_ptr<boost::asio::ip::udp::socket> NetSocket, const boost::asio::ip::udp::endpoint& MyAddr) : mySocket(NetSocket)
{
	sharedSocket = true;
	addr = MyAddr;
	Init();
}

UDPConnection::UDPConnection(int sourceport, const std::string& address, const unsigned port)
{
	addr = ResolveAddr(address, port);
	if (addr.address().is_v6())
	{
		boost::shared_ptr<ip::udp::socket> temp(new ip::udp::socket(netcode::netservice, ip::udp::endpoint(ip::address_v6::any(), sourceport)));
		mySocket = temp;
	}
	else
	{
		boost::shared_ptr<ip::udp::socket> temp(new ip::udp::socket(netcode::netservice, ip::udp::endpoint(ip::address_v4::any(), sourceport)));
		mySocket = temp;
	}
	sharedSocket = false;
	Init();
}

UDPConnection::~UDPConnection()
{
	if (fragmentBuffer)
		delete fragmentBuffer;
	Flush(true);
}

void UDPConnection::SendData(boost::shared_ptr<const RawPacket> data)
{
	assert(data->length > 0);
	outgoingData.push_back(data);
}

bool UDPConnection::HasIncomingData() const
{
	return !msgQueue.empty();
}

boost::shared_ptr<const RawPacket> UDPConnection::Peek(unsigned ahead) const
{
	if (ahead < msgQueue.size())
		return msgQueue[ahead];
	else
	{
		boost::shared_ptr<const RawPacket> empty;
		return empty;
	}
}

boost::shared_ptr<const RawPacket> UDPConnection::GetData()
{
	if (!msgQueue.empty())
	{
		boost::shared_ptr<const RawPacket> msg = msgQueue.front();
		msgQueue.pop_front();
		return msg;
	}
	else
	{
		boost::shared_ptr<const RawPacket> empty;
		return empty;
	}
}

void UDPConnection::Update()
{
	outgoing.UpdateTime(spring_gettime());
	
	if (!sharedSocket)
	{
		// duplicated code with UDPListener
		netservice.poll();
		size_t bytes_avail = 0;
		while ((bytes_avail = mySocket->available()) > 0)
		{
			std::vector<uint8_t> buffer(bytes_avail);
			ip::udp::endpoint sender_endpoint;
			size_t bytesReceived;
			boost::asio::ip::udp::socket::message_flags flags = 0;
			boost::system::error_code err;
			bytesReceived = mySocket->receive_from(boost::asio::buffer(buffer), sender_endpoint, flags, err);
			if (CheckErrorCode(err))
				break;

			if (bytesReceived < Packet::headerSize)
				continue;
			Packet data(&buffer[0], bytesReceived);
			if (CheckAddress(sender_endpoint))
			{
				ProcessRawPacket(data);
			}
		}
	}

	Flush(false);
}

void UDPConnection::ProcessRawPacket(Packet& incoming)
{
	lastReceiveTime = spring_gettime();
	dataRecv += incoming.GetSize();
	recvOverhead += Packet::headerSize;
	++recvPackets;

	AckChunks(incoming.lastContinuous);

	if (!unackedChunks.empty())
	{
		if (incoming.nakType < 0)
		{
			for (int i = 0; i != -incoming.nakType; ++i)
			{
				if (i < unackedChunks.size())
					RequestResend(unackedChunks[i]);
			}
		}
		else if (incoming.nakType > 0)
		{
			for (int i = 0; i != incoming.naks.size(); ++i)
			{
				if (incoming.naks[i] < unackedChunks.size())
					RequestResend(unackedChunks[incoming.naks[i]]);
			}
		}
	}
	for (std::list<ChunkPtr>::const_iterator it = incoming.chunks.begin(); it != incoming.chunks.end(); ++it)
	{
		if (lastInOrder >= (*it)->chunkNumber || waitingPackets.find((*it)->chunkNumber) != waitingPackets.end())
		{
			++droppedChunks;
			continue;
		}
		waitingPackets.insert((*it)->chunkNumber, new RawPacket(&(*it)->data[0], (*it)->data.size()));
	}

	packetMap::iterator wpi;
	//process all in order packets that we have waiting
	while ((wpi = waitingPackets.find(lastInOrder+1)) != waitingPackets.end())
	{
		std::vector<boost::uint8_t> buf;
		if (fragmentBuffer)
		{
			buf.resize(fragmentBuffer->length);
			// combine with fragment buffer
			std::copy(fragmentBuffer->data, fragmentBuffer->data+fragmentBuffer->length, buf.begin());
			delete fragmentBuffer;
			fragmentBuffer = NULL;
		}

		lastInOrder++;
		std::copy(wpi->second->data, wpi->second->data+wpi->second->length, std::back_inserter(buf));
		waitingPackets.erase(wpi);

		for (unsigned pos = 0; pos < buf.size();)
		{
			char msgid = buf[pos];
			ProtocolDef* proto = ProtocolDef::instance();
			if (proto->IsAllowed(msgid))
			{
				unsigned msglength = 0;
				if (proto->HasFixedLength(msgid))
				{
					msglength = proto->GetLength(msgid);
				}
				else
				{
					int length_t = proto->GetLength(msgid);

					// got enough data in the buffer to read the length of the message?
					if (buf.size() > pos - length_t)
					{
						// yes => read the length (as byte or word)
						if (length_t == -1)
						{
							msglength = buf[pos+1];
						}
						else if (length_t == -2)
						{
							msglength = *(uint16_t*)(&buf[pos+1]);
						}
					}
					else
					{
						// no => store the fragment and break
						fragmentBuffer = new RawPacket(&buf[pos], buf.size() - pos);
						break;
					}
				}

				// if this isn't true we'll loop infinitely while filling up memory
				assert(msglength != 0);

				// got the complete message in the buffer?
				if (buf.size() >= pos + msglength)
				{
					// yes => add to msgQueue and keep going
					msgQueue.push_back(boost::shared_ptr<const RawPacket>(new RawPacket(&buf[pos], msglength)));
					pos += msglength;
				}
				else
				{
					// no => store the fragment and break
					fragmentBuffer = new RawPacket(&buf[pos], buf.size() - pos);
					break;
				}
			}
			else
			{
				// error
				pos++;
			}
		}
	}
}

void UDPConnection::Flush(const bool forced)
{
	const spring_time curTime = spring_gettime();

	unsigned outgoingLength = 0;
	for (packetList::const_iterator it = outgoingData.begin(); it != outgoingData.end(); ++it)
		outgoingLength += (*it)->length;

	// do not create more than 30 chunks per second
	const bool waitMore = (lastChunkCreated < curTime - spring_msecs(34)) ? false : true;
	if (forced || (!waitMore && lastChunkCreated + spring_msecs(200) < (curTime + outgoingLength*10)))
	{
		boost::uint8_t buffer[UDPMaxPacketSize];
		unsigned pos = 0;
		// Manually fragment packets to respect configured UDP_MTU.
		// This is an attempt to fix the bug where players drop out of the game if
		// someone in the game gives a large order.

		do
		{
			if (!outgoingData.empty())
			{
				packetList::iterator it = outgoingData.begin();
				unsigned numBytes = std::min((unsigned)MaxChunkSize-pos, (*it)->length);
				assert((*it)->length > 0);
				memcpy(buffer+pos, (*it)->data, numBytes);
				pos+= numBytes;
				if (numBytes == (*it)->length) // full packet copied
					outgoingData.pop_front();
				else // partially transfered
					(*it).reset(new RawPacket((*it)->data + numBytes, (*it)->length - numBytes));
			} // iterator "it" is now invalid
			if (pos > 0 && (pos == MaxChunkSize || outgoingData.empty()))
			{
				CreateChunk(buffer, pos, currentNum++);
				pos = 0;
			}
		} while (!outgoingData.empty());
	}
	SendIfNecessary(forced);
}

bool UDPConnection::CheckTimeout() const
{
	const spring_duration timeout = ((dataRecv == 0) ? spring_secs(45) : spring_secs(30));
	if((lastReceiveTime+timeout) < spring_gettime())
	{
		return true;
	}
	else
		return false;
}

std::string UDPConnection::Statistics() const
{
	std::string msg = "Statistics for UDP connection:\n";
	msg += str( boost::format("Received: %1% bytes in %2% packets (%3% bytes/package)\n") %dataRecv %recvPackets %((float)dataRecv / (float)recvPackets));
	msg += str( boost::format("Sent: %1% bytes in %2% packets (%3% bytes/package)\n") %dataSent %sentPackets %((float)dataSent / (float)sentPackets));
	msg += str( boost::format("Relative protocol overhead: %1% up, %2% down\n") %((float)sentOverhead / (float)dataSent) %((float)recvOverhead / (float)dataRecv) );
	msg += str( boost::format("%1% incoming chunks had been dropped, %2% outgoing chunks had to be resent\n") %droppedChunks %resentChunks);
	return msg;
}

bool UDPConnection::CheckAddress(const boost::asio::ip::udp::endpoint& from) const
{
	return (addr == from);
}

std::string UDPConnection::GetFullAddress() const
{
	return str( boost::format("[%s]:%u") %addr.address().to_string() %addr.port() );
}

void UDPConnection::SetMTU(unsigned mtu2)
{
	if (mtu2 > 300 && mtu2 < UDPMaxPacketSize)
	{
		mtu = mtu2;
	}
}

void UDPConnection::Init()
{
	spring_notime(lastNakTime);
	spring_notime(lastSendTime);
	spring_notime(lastUnackResent);
	lastReceiveTime = spring_gettime();
	lastInOrder=-1;
	waitingPackets.clear();
	currentNum=0;
	lastNak=-1;
	sentOverhead = 0;
	recvOverhead = 0;
	fragmentBuffer = 0;
	resentChunks = 0;
	sentPackets = recvPackets = 0;
	droppedChunks = 0;
	mtu = std::max(configHandler->Get("MaximumTransmissionUnit", 1400), 300);
}

void UDPConnection::CreateChunk(const unsigned char* data, const unsigned length, const int packetNum)
{
	assert(length > 0 && length < 255);
	ChunkPtr buf(new Chunk);
	buf->chunkNumber = packetNum;
	buf->chunkSize = length;
	std::copy(data, data+length, std::back_inserter(buf->data));
	newChunks.push_back(buf);
	lastChunkCreated = spring_gettime();
}

void UDPConnection::SendIfNecessary(bool flushed)
{
	const spring_time curTime = spring_gettime();
	
	int nak = 0;
	std::vector<int> dropped;
	
	{
		int packetNum = lastInOrder+1;
		for (packetMap::iterator it = waitingPackets.begin(); it != waitingPackets.end(); ++it)
		{
			const int diff = it->first - packetNum;
			if (diff > 0)
			{
				for (int i = 0; i < diff; ++i)
				{
					dropped.push_back(packetNum);
					packetNum++;
				}
			}
			packetNum++;
		}
		unsigned numContinuous = 0;
		for (unsigned i = 0; i != dropped.size(); ++i)
		{
			if (dropped[i] == lastInOrder+i+1)
			{
				numContinuous++;
			}
			else
				break;
		}

		if (numContinuous < 8 && !(lastNakTime + spring_msecs(200) < spring_gettime()))
		{
			nak = std::min(dropped.size(), (size_t)127);
			lastNakTime = curTime; // needs 1 byte per requested packet, so don't spam to often
		}
		else
		{
			nak = -(int)std::min((unsigned)127, numContinuous);
		}
	}

	lastUnackResent = std::max(lastUnackResent, lastChunkCreated);
	if (!unackedChunks.empty() && lastUnackResent + spring_msecs(400) < curTime)
	{
		if (newChunks.empty())
		{
			// resent last packet if we didn't get an ack after 0,4 seconds
			// and don't plan sending out a new chunk either
			RequestResend(*(unackedChunks.end()-1));
		}
		lastUnackResent = curTime;
	}

	if (flushed || !newChunks.empty() || !resendRequested.empty() || nak > 0 || lastSendTime + spring_msecs(200) < curTime)
	{
		bool todo = true;
		while (todo && outgoing.GetAverage() < 64*1024)
		{
			Packet buf(lastInOrder, nak);
			if (nak > 0)
			{
				buf.naks.resize(nak);
				for (unsigned i = 0; i != buf.naks.size(); ++i)
				{
					buf.naks[i] = dropped[i];
				}
				nak = 0; // 1 request is enought
			}
			
			while (true)
			{
				if (!newChunks.empty() && buf.GetSize() + newChunks[0]->GetSize() <= mtu)
				{
					buf.chunks.push_back(newChunks[0]);
					unackedChunks.push_back(newChunks[0]);
					newChunks.pop_front();
				}
				else if (!resendRequested.empty() && buf.GetSize() + resendRequested[0]->GetSize() <= mtu)
				{
					buf.chunks.push_back(resendRequested[0]);
					resendRequested.pop_front();
					++resentChunks;
				}
				else
					break;
			}
			
			SendPacket(buf);
			if (resendRequested.empty() && newChunks.empty())
				todo = false;
		}
	}
}

void UDPConnection::SendPacket(Packet& pkt)
{
	std::vector<uint8_t> data;
	pkt.Serialize(data);

	outgoing.DataSent(data.size());
	lastSendTime = spring_gettime();
	boost::asio::ip::udp::socket::message_flags flags = 0;
	boost::system::error_code err;
	mySocket->send_to(buffer(data), addr, flags, err);
	if (CheckErrorCode(err))
		return;

	dataSent += data.size();
	++sentPackets;
}

void UDPConnection::AckChunks(int lastAck)
{
	while (!unackedChunks.empty() && lastAck >= (*unackedChunks.begin())->chunkNumber)
	{
		unackedChunks.pop_front();
	}

	bool done;
	do
	{
		// resend requested and later acked, happens every now and then
		done = true;
		for (std::deque<ChunkPtr>::iterator it = resendRequested.begin(); it != resendRequested.end(); ++it)
		{
			if ((*it)->chunkNumber <= lastAck)
			{
				resendRequested.erase(it);
				done = false;
				break;
			}
		}
	} while (!done);
}

void UDPConnection::RequestResend(ChunkPtr ptr)
{
	for (std::deque<ChunkPtr>::const_iterator it = resendRequested.begin(); it != resendRequested.end(); ++it)
	{
		// filter out duplicates
		if (*it == ptr)
			return;
	}
	resendRequested.push_back(ptr);
}

UDPConnection::BandwidthUsage::BandwidthUsage() : lastTime(0), trafficSinceLastTime(1), average(0.0)
{
}

void UDPConnection::BandwidthUsage::UpdateTime(unsigned newTime)
{
	if (newTime > lastTime+100)
	{
		average = (average*9 + float(trafficSinceLastTime)/float(newTime-lastTime)*1000.0f)/10.0f;
		trafficSinceLastTime = 0;
		lastTime = newTime;
	}
}

void UDPConnection::BandwidthUsage::DataSent(unsigned amount)
{
	trafficSinceLastTime += amount;
}

float UDPConnection::BandwidthUsage::GetAverage() const
{
	return (average+trafficSinceLastTime); // not exactly accurate, but does job
}

} // namespace netcode
