/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef _MSC_VER
#include "lib/streflop/streflop_cond.h"
#endif

#include "Connection.h"

#include "RawTextMessage.h"

#include <bitset>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "lib/md5/md5.h"
#include "lib/md5/base64.h"

#if BOOST_VERSION < 103600
using namespace boost::system::posix_error;
#else
using namespace boost::system::errc;
#endif
using namespace boost::asio;


Connection::Connection() : sock(netservice), timer(netservice)
{
}

Connection::~Connection()
{
	sock.close();
	netservice.poll();
	netservice.reset();
}

void Connection::Connect(const std::string& server, int port)
{
	using namespace boost::asio;
	boost::system::error_code err;
	ip::address tempAddr = ip::address::from_string(server, err);
	if (err)
	{
		// error, maybe a hostname?
		ip::tcp::resolver resolver(netservice);
		std::ostringstream portbuf;
		portbuf << port;
		ip::tcp::resolver::query query(server, portbuf.str());
		ip::tcp::resolver::iterator iter = resolver.resolve(query, err);
		if (err)
		{
			DoneConnecting(false, err.message());
			return;
		}
		tempAddr = iter->endpoint().address();
	}

	ip::tcp::endpoint serverep(tempAddr, port);
	sock.async_connect(serverep, boost::bind(&Connection::ConnectCallback, this, placeholders::error));
}

std::string MD5Base64(const std::string& plain)
{
	md5_state_t state;
	md5_init(&state);
	md5_append(&state, (md5_byte_t*)plain.c_str(), plain.size());
	unsigned char digest[16];
	md5_finish(&state, digest);
	return base64_encode(digest, 16);
}

void Connection::Register(const std::string& name, const std::string& password)
{
	std::ostringstream out;
	out << "REGISTER " << name << " " << MD5Base64(password) << "\n";
	SendData(out.str());
}

void Connection::Login(const std::string& name, const std::string& password)
{
	std::ostringstream out;
	out << "LOGIN " << name << " " << MD5Base64(password) << " 0 localhost libLobby v0.1\n";
	SendData(out.str());
	myUserName = name;
}

void Connection::ConfirmAggreement()
{
	SendData("CONFIRMAGREEMENT\n");
}


void Connection::Rename(const std::string& newName)
{
	std::ostringstream out;
	out << "RENAMEACCOUNT " << newName << "\n";
	SendData(out.str());
}

void Connection::ChangePass(const std::string& oldpass, const std::string& newpass)
{
	std::ostringstream out;
	out << "CHANGEPASSWORD " << MD5Base64(oldpass) << " " << MD5Base64(newpass) << "\n";
	SendData(out.str());
}

void Connection::StatusUpdate(bool ingame, bool away)
{
	std::bitset<8> statusbf;
	statusbf.set(0, ingame);
	statusbf.set(1, away);
	int trank = myStatus.rank;
	if (trank >= 4)
	{
		statusbf.set(4);
		trank -= 4;
	}
	if (trank >= 2)
	{
		statusbf.set(3);
		trank -= 2;
	}
	if (trank == 1)
	{
		statusbf.set(2);
	}
	statusbf.set(5, myStatus.moderator);
	statusbf.set(6, myStatus.bot);
	
	std::ostringstream out;
	out << "MYSTATUS " << static_cast<unsigned>(statusbf.to_ulong()) << "\n";
	SendData(out.str());
}

void Connection::Channels()
{
	SendData("CHANNELS\n");
}

void Connection::RequestMutelist(const std::string& channel)
{
	std::ostringstream out;
	out << "MUTELIST " << channel << std::endl;
	SendData(out.str());
}

void Connection::JoinChannel(const std::string& channame, const std::string& password)
{
	std::ostringstream out;
	out << "JOIN " << channame;
	if (!password.empty())
		out << " " << password;
	out << std::endl;
	SendData(out.str());
}

void Connection::LeaveChannel(const std::string& channame)
{
	std::ostringstream out;
	out << "LEAVE " << channame << std::endl;
	SendData(out.str());
}

void Connection::ForceLeaveChannel(const std::string& channame, const std::string& user, const std::string& reason)
{
	std::ostringstream out;
	out << "FORCELEAVECHANNEL " << channame << " " << user;
	if (!reason.empty())
		out  << " " << reason;
	out << std::endl;
	SendData(out.str());
}

void Connection::ChangeTopic(const std::string& channame, const std::string& topic)
{
	std::ostringstream out;
	out << "CHANNELTOPIC " << channame <<  " " << topic << std::endl;
	SendData(out.str());
}

void Connection::Say(const std::string& channel, const std::string& text)
{
	std::ostringstream out;
	out << "SAY " << channel << " " << text << std::endl;
	SendData(out.str());
}

void Connection::SayEx(const std::string& channel, const std::string& text)
{
	std::ostringstream out;
	out << "SAYEX " << channel << " " << text << std::endl;
	SendData(out.str());
}

void Connection::SayPrivate(const std::string& user, const std::string& text)
{
	std::ostringstream out;
	out << "SAYPRIVATE " << user << " " << text << std::endl;
	SendData(out.str());
}

void Connection::Ping()
{
	if (sock.is_open())
	{
		SendData("PING\n");
	
		timer.expires_from_now(boost::posix_time::seconds(10));
		timer.async_wait(boost::bind(&Connection::Ping, this));
	}
}

void Connection::SendData(const std::string& msg)
{
	if (sock.is_open())
	{
		sock.send(boost::asio::buffer(msg));
	}
	else
	{
		NetworkError("Already disconnected");
	}
}

void Connection::Poll()
{
	netservice.poll();
}

void Connection::Run()
{
	netservice.run();
	netservice.reset();
}

void Connection::DataReceived(const std::string& command, const std::string& msg)
{
	if (command == "TASServer")
	{
		if (!msg.empty())
		{
			RawTextMessage buf(msg);
			std::string serverVer = buf.GetWord();
			std::string clientVer = buf.GetWord();
			int udpport = buf.GetInt();
			int mode = buf.GetInt();
			ServerGreeting(serverVer, clientVer, udpport, mode);
		}
		Ping();
	}
	else if (command == "DENIED")
	{
		Denied(msg);
	}
	else if (command == "LOGININFOEND")
	{
		LoginEnd();
	}
	else if (command == "REGISTRATIONDENIED")
	{
		RegisterDenied(msg);
	}
	else if (command == "REGISTRATIONACCEPTED")
	{
		RegisterAccept();
	}
	else if (command == "ADDUSER")
	{
		RawTextMessage buf(msg);
		std::string name = buf.GetWord();
		std::string country = buf.GetWord();
		int cpu = buf.GetInt();
		AddUser(name, country, cpu);
	}
	else if (command == "REMOVEUSER")
	{
		RawTextMessage buf(msg);
		std::string name = buf.GetWord();
		RemoveUser(name);
	}
	else if (command == "CLIENTSTATUS")
	{
		RawTextMessage buf(msg);
		std::string name = buf.GetWord();
		ClientStatus status;
		std::bitset<8> statusbf(buf.GetInt());
		status.ingame = statusbf[0];
		status.away = statusbf[1];
		status.rank = (statusbf[2]? 1 : 0) + (statusbf[3]? 2 : 0) + (statusbf[4]? 4 : 0);
		status.moderator = statusbf[5];
		status.bot = statusbf[6];
		if (name == myUserName)
			myStatus = status;
		ClientStatusUpdate(name, status);
	}
	else if (command == "JOIN")
	{
		RawTextMessage buf(msg);
		std::string channame = buf.GetWord();
		Joined(channame);
	}
	else if (command == "JOINFAILED")
	{
		RawTextMessage buf(msg);
		std::string channame = buf.GetWord();
		std::string reason = buf.GetWord();
		JoinFailed(channame, reason);
	}
	else if (command == "CLIENTS")
	{
		RawTextMessage buf(msg);
		std::string channame = buf.GetWord();
		std::string client;
		while (!(client = buf.GetWord()).empty())
		{
			ChannelMember(channame, client, false);
		}
	}
	else if (command == "JOINED")
	{
		RawTextMessage buf(msg);
		std::string channame = buf.GetWord();
		std::string client = buf.GetWord();
		ChannelMember(channame, client, true);
	}
	else if (command == "LEFT")
	{
		RawTextMessage buf(msg);
		std::string channame = buf.GetWord();
		std::string client = buf.GetWord();
		std::string reason = buf.GetSentence();
		ChannelMemberLeft(channame, client, reason);
	}
	else if (command == "FORCELEAVECHANNEL")
	{
		RawTextMessage buf(msg);
		std::string channame = buf.GetWord();
		std::string client = buf.GetWord();
		std::string reason = buf.GetSentence();
		ForceLeftChannel(channame, client, reason);
	}
	else if (command == "CHANNELTOPIC")
	{
		RawTextMessage buf(msg);
		std::string channame = buf.GetWord();
		std::string author = buf.GetWord();
		long unsigned time = buf.GetTime();
		std::string topic = buf.GetSentence();
		ChannelTopic(channame, author, time, topic);
	}
	else if (command == "CHANNELMESSAGE")
	{
		RawTextMessage buf(msg);
		std::string channame = buf.GetWord();
		std::string message = buf.GetSentence();
		ChannelMessage(channame, message);
	}
	else if (command == "AGREEMENT")
	{
		aggreementbuf += msg + "\n";
	}
	else if (command == "AGREEMENTEND")
	{
		Aggreement(Connection::RTFToPlain(aggreementbuf));
	}
	else if (command == "MOTD")
	{
		Motd(msg);
	}
	else if (command == "SERVERMSG")
	{
		ServerMessage(msg);
	}
	else if (command == "CHANNEL")
	{
		RawTextMessage buf(msg);
		std::string channame = buf.GetWord();
		unsigned users = buf.GetInt();
		ChannelInfo(channame, users);
	}
	else if (command == "ENDOFCHANNELS")
	{
		ChannelInfoEnd();
	}
	else if (command == "MUTELISTBEGIN")
	{
		inMutelistChannel = msg;
		mutelistBuf.clear();
	}
	else if (command == "MUTELIST")
	{
		mutelistBuf.push_back(msg);
	}
	else if (command == "MUTELISTEND")
	{
		Mutelist(inMutelistChannel, mutelistBuf);
		inMutelistChannel.clear();
		mutelistBuf.clear();
	}
	else if (command == "SERVERMSGBOX")
	{
		RawTextMessage buf(msg);
		std::string message = buf.GetSentence();
		std::string url = buf.GetSentence();
		ServerMessageBox(message, url);
	}
	else if (command == "SAID")
	{
		
		RawTextMessage buf(msg);
		std::string channame = buf.GetWord();
		std::string user = buf.GetWord();
		std::string text = buf.GetSentence();
		Said(channame, user, text);
	}
	else if (command == "SAIDEX")
	{
		
		RawTextMessage buf(msg);
		std::string channame = buf.GetWord();
		std::string user = buf.GetWord();
		std::string text = buf.GetSentence();
		SaidEx(channame, user, text);
	}
	else if (command == "SAIDPRIVATE")
	{
		
		RawTextMessage buf(msg);
		std::string user = buf.GetWord();
		std::string text = buf.GetSentence();
		SaidPrivate(user, text);
	}
	else if (command == "BATTLEOPENED")
	{
		
		RawTextMessage buf(msg);
		int id = buf.GetInt();
		bool replay = !buf.GetInt();
		bool nat = buf.GetInt();
		std::string founder = buf.GetWord();
		std::string ip = buf.GetWord();
		int port = buf.GetInt();
		int maxplayers = buf.GetInt();
		bool password = buf.GetInt();
		int rank = buf.GetInt();
		int maphash = buf.GetInt();
		std::string map = buf.GetSentence();
		std::string title = buf.GetSentence();
		std::string mod = buf.GetSentence();
		BattleOpened(id, replay, nat, founder, ip, port, maxplayers, password, rank, maphash, title, map, mod);
	}
	else if (command == "UPDATEBATTLEINFO")
	{
		RawTextMessage buf(msg);
		int id = buf.GetInt();
		int spectators = buf.GetInt();
		bool locked = buf.GetInt();
		int maphash = buf.GetInt();
		std::string map = buf.GetSentence();
		BattleUpdated(id, spectators, locked, maphash, map);
	}
	else if (command == "BATTLECLOSED")
	{
		
		RawTextMessage buf(msg);
		int id = buf.GetInt();
		BattleClosed(id);
	}
	else
	{
		 // std::cout << "Unhandled command: " << command << " " << msg << std::endl;
	}
}

void Connection::ConnectCallback(const boost::system::error_code& error)
{
	if (!error)
	{
		DoneConnecting(true, "");
		boost::asio::async_read_until(sock, incomeBuffer, "\n", boost::bind(&Connection::ReceiveCallback, this, placeholders::error, placeholders::bytes_transferred));
	}
	else
	{
		DoneConnecting(false, error.message());
	}
}

void Connection::ReceiveCallback(const boost::system::error_code& error, size_t bytes)
{
	if (!error)
	{
		std::string msg;
		std::string command;
		std::istream buf(&incomeBuffer);
		buf >> command;
		std::getline(buf, msg);
		if (!msg.empty())
			msg = msg.substr(1);
		DataReceived(command, msg);
	}
	else
	{
		if (error.value() == connection_reset || error.value() == boost::asio::error::eof)
		{
			sock.close();
			Disconnected();
		}
		else
		{
			NetworkError(error.message());
		}
	}
	if (sock.is_open())
	{
		boost::asio::async_read_until(sock, incomeBuffer, "\n", boost::bind(&Connection::ReceiveCallback, this, placeholders::error, placeholders::bytes_transferred));
	}
}

void Connection::SendCallback(const boost::system::error_code& error)
{
	if (!error)
	{
	}
	else
	{
	}
}

std::string Connection::RTFToPlain(const std::string& rich)
{
	static const std::string screwTag = "\\pard";
	static const std::string lineend = "\\par";
	std::string out = rich.substr(rich.find_first_of('{')+1, (rich.find_last_of('}') - rich.find_first_of('{')) -2);

	out = out.substr(out.find_last_of('}')+1);
	size_t pos = 0;

	while ((pos = out.find(screwTag)) != std::string::npos)
	{
		out.erase(pos, screwTag.size());
	}
	while ((pos = out.find(lineend)) != std::string::npos)
	{
		out.replace(pos, lineend.size(), "\n");
	}
	while ((pos = out.find('\\')) != std::string::npos)
	{
		size_t pos2 = out.find_first_of("\\ \t\n}", pos+1);
		if (pos2 != std::string::npos)
		{
			out.erase(pos, pos2-pos);
		}
		else
		{
			break;
		}
	}

	return out;
}
