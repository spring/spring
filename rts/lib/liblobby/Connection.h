/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONNECTION_H
#define CONNECTION_H

#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <string>

#include <iostream>

class RawTextMessage
{
public:
	RawTextMessage(const std::string& _message) : message(_message), pos(0)
	{};
	
	std::string GetWord()
	{
		size_t oldpos = pos;
		pos = message.find_first_of(std::string("\t \n"), oldpos);
		if (pos != std::string::npos)
		{
			return message.substr(oldpos, pos++ - oldpos);
		}
		else if (oldpos != std::string::npos)
		{
			return message.substr(oldpos);
		}
		else
		{
			return "";
		}
	};
	
	std::string GetSentence()
	{
		size_t oldpos = pos;
		pos = message.find_first_of(std::string("\t\n"), oldpos);
		if (pos != std::string::npos)
		{
			return message.substr(oldpos, pos++ - oldpos);
		}
		else if (oldpos != std::string::npos)
		{
			return message.substr(oldpos);
		}
		else
		{
			return "";
		}
	};
	
	int GetInt()
	{
		std::istringstream buf(GetWord());
		int temp;
		buf >> temp;
		return temp;
	};
	
	long unsigned GetTime()
	{
		std::istringstream buf(GetWord());
		long unsigned temp;
		buf >> temp;
		return temp;
	};

private:
	std::string message;
	size_t pos;
};

struct ClientStatus
{
	bool ingame;
	bool away;
	int rank;
	bool moderator;
	bool bot;
};

class Connection
{
public:
	Connection();
	~Connection();

	void Connect(const std::string& server, int port);
	virtual void DoneConnecting(bool succes, const std::string& err) {};
	virtual void ServerGreeting(const std::string& serverVer, const std::string& springVer, int udpport, int mode) {};

	void Register(const std::string& name, const std::string& password);
	virtual void RegisterDenied(const std::string& reason) {};
	virtual void RegisterAccept() {};

	void Login(const std::string& name, const std::string& password);
	virtual void Denied(const std::string& reason) {};
	virtual void LoginEnd() {};
	virtual void Aggreement(const std::string text) {};
	void ConfirmAggreement();

	void Rename(const std::string& newName);
	void ChangePass(const std::string& oldpass, const std::string& newpass);

	virtual void Motd(const std::string& text) {};
	virtual void ServerMessage(const std::string& text) {};
	virtual void ServerMessageBox(const std::string& text, const std::string& url) {};

	virtual void AddUser(const std::string& name, const std::string& country, int cpu) {};
	virtual void RemoveUser(const std::string& name) {};
	virtual void ClientStatusUpdate(const std::string& name, ClientStatus status) {};

	void Channels();
	virtual void ChannelInfo(const std::string& channel, unsigned users) {};
	virtual void ChannelInfoEnd() {};

	void RequestMutelist(const std::string& channel);
	virtual void Mutelist(const std::string& channel, std::list<std::string> list) {};

	void JoinChannel(const std::string& channame, const std::string& password = "");
	virtual void Joined(const std::string& channame) {};
	virtual void JoinFailed(const std::string& channame, const std::string& reason) {};
	void LeaveChannel(const std::string& channame);

	void ChangeTopic(const std::string& channame, const std::string& topic);
	virtual void ChannelTopic(const std::string& channame, const std::string& author, long unsigned time, const std::string& topic) {};

	void Say(const std::string& channel, const std::string& text);
	void SayEx(const std::string& channel, const std::string& text);
	void SayPrivate(const std::string& user, const std::string& text);
	virtual void Said(const std::string& channel, const std::string& user, const std::string& text) {};
	virtual void SaidEx(const std::string& channel, const std::string& user, const std::string& text) {};
	virtual void SaidPrivate(const std::string& user, const std::string& text) {};

	virtual void Disconnected() {};
	virtual void NetworkError(const std::string& msg) {};

	void Ping();
	void SendData(const std::string& msg);

	void Poll();
	void Run();

private:
	void DataReceived(const std::string& command, const std::string& msg);

	void ConnectCallback(const boost::system::error_code& error);
	void ReceiveCallback(const boost::system::error_code& error, size_t bytes);
	void SendCallback(const boost::system::error_code& error);
	
	boost::asio::io_service netservice;
	std::string aggreementbuf;
	std::string inMutelistChannel;
	std::list<std::string> mutelistBuf;
	boost::asio::ip::tcp::socket sock;
	boost::asio::streambuf incomeBuffer;
	boost::asio::deadline_timer timer;
};

#endif
