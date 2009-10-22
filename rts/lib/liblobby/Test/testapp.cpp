#include "../Connection.h"

#include <stdlib.h>
#include <iostream>

using namespace std;

class TestConnection : public Connection 
{
public:
	virtual void DoneConnecting(bool success, const std::string& err)
	{
		if (success)
		{
			cout << "Connection established\n";
		}
		else
		{
			cout << "Could not connect to server: " << err;
			exit(1);
		}
	};

	virtual void DataReceived(const std::string& command, const std::string& msg)
	{
		cout << command << ": ";
		if (!msg.empty())
		{
			RawTextMessage buf(msg);
			std::string par = buf.GetWord();
			while (!par.empty())
			{
				std::cout << std::endl << "--> " << par;
				par = buf.GetWord();
			}
		}
		cout << endl;
	};
};

int main()
{
	TestConnection con;
	con.Connect("taspringmaster.clan-sy.com", 8200);
	while (true)
	{
		con.Run();
	}
	return 0;
};