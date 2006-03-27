#include "../helper.h"

int Lmagic;

Log::Log(){
	verbose = false;
	First = false;
}

Log::~Log(){
G = 0;
}

string Log::GameTime(){
	char min[4];
	char sec[4];
	char hour[4];
	int Time = G->cb->GetCurrentFrame();
	Time = Time/30;
	int Seconds = Time%60;
	int Minutes = (Time/60)%60;
	int hours = 0;
	if(Time>3600) hours = (Time-(Time%3600))/3600;
	if(Seconds+Minutes+hours == 0){
		return "[00:00]";
	}
	sprintf(hour,"%i",hours);
	sprintf(min,"%i",Minutes);
	sprintf(sec,"%i",Seconds);
	string stime="[";
	if(hours>0){
		if(hours<10) stime += "0";
		stime += hour;
		stime += ":";
	}
	if(Minutes<10){
		stime += "0";
	}
	stime += min;
	stime += ":";
	if(Seconds <10){
		stime += "0";
	}
	stime += sec;
	stime += "]";
	return stime;
}
bool Log::FirstInstance(){
	return First;
}
void Log::Open(bool plain){
	if( Lmagic != 95768){
		Lmagic = 95768;
		this->First = true;
	}
	if(plain == true){ // Textfile Logging
		plaintext = true;
		char c[400];
		time_t now1;
		time(&now1);
		struct tm *now2;
		string filename = "aidll/globalai/NTAI";
		filename += slash;
		filename += "Logs";
		filename += slash;
		now2 = localtime(&now1);
		//             DDD MMM DD HH:MM:SS YYYY_X - NTAI.log
		sprintf(c, "%2.2d-%2.2d-%4.4d %2.2d%2.2d [%d]XE7.5.log",
				now2->tm_mon+1, now2->tm_mday, now2->tm_year + 1900, now2->tm_hour,
				now2->tm_min, G->team);
		filename += c;
		logFile.open(filename.c_str());
		if(logFile.is_open() == false){
			string s = "mkdir NTAI";
			s+= slash;
			s+= "Logs";
			system(s.c_str());
			logFile.close();
			logFile.open(filename.c_str());
		}
		header(" :: NTAI XE7.5 Log File \n :: Programmed and maintained by AF \n :: Copyright (C) 2006 AF \n :: Released under the GPL 2.0 Liscence \n");
		logFile << " :: Game started: " << now2->tm_mday << "." << now2->tm_mon << "." << 1900 + now2->tm_year << "  " << now2->tm_hour << ":" << now2->tm_min << ":" << now2->tm_sec << endl << endl;
		CSunParser cp(G);
		cp.LoadFile("modinfo.tdf");
		logFile << " :: " << cp.SGetValueMSG("MOD\\Name") << endl;
		logFile << " :: " << cp.SGetValueMSG("MOD\\Description") << endl;
		if(First == true) logFile << " :: First instance of NTAI" << endl;
		CSunParser cq(G);
		cq.LoadFile("script.txt");
		vector<string> names;
		for(int n=0; n<MAX_TEAMS; n++){
    		char c[8];
    		sprintf(c, "%i", n);
			string s = cq.SGetValueDef("", string("GAME\\PLAYER") + string(c) + "\\name");
			if(s != string("")){
				names.push_back(s + " : player :: " + c);
				PlayerNames[n] = s;
			} else{
				PlayerNames[n] = "spring_engine";
			}
		}
		if(names.empty() == false){
			for(vector<string>::iterator i = names.begin(); i != names.end(); ++i){
				logFile << " :: " << *i << endl;
			}
		}
		logFile << _T(" :: AI DLL's in game") << endl;
		vector<string> AInames;
		for(int n=0; n<MAX_TEAMS; n++){
    		char c[8];
			sprintf(c, "%i", n);
			string s = cq.SGetValueDef("", string(_T("GAME\\TEAM")) + string(c) + "\\AIDLL");
			if(s != string("")) AInames.push_back(s + " : AI :: " + c);
		}
		if(names.empty() == false){
			for(vector<string>::iterator i = AInames.begin(); i != AInames.end(); ++i){
				logFile << " :: " << *i << endl;
			}
		}
		logFile << endl;
	}else{ // HTML logging
		char c[400];
		time_t now1;
		time(&now1);
		struct tm *now2;
		now2 = localtime(&now1);
		string filename = _T("aidll/globalai/NTAI");
		filename += slash;
		filename += _T("Logs");
		filename += slash;
		//                                      DDD MMM DD HH:MM:SS YYYY_X - NTAI.htm
		sprintf(c, "%2.2d-%2.2d-%4.4d %2.2d%2.2d [%d] - NTAIXE7.5.htm",
				now2->tm_mon+1, now2->tm_mday, now2->tm_year + 1900, now2->tm_hour,
				now2->tm_min, G->team);
		filename += c;
		logFile.open(filename.c_str());
		plaintext = false;
		string mu ="</table><style type='text/css'>\n<!--\nbody,td,th {\n	font-family: sans-serif;\n	color: #111111;\nfont-size: 12px;\n\n}\nbody {\n	background-color: #FFFFFF;\n\n}\n.c {color: #FF2222}\n.e {color: #FFCC11}\n-->\n</style>\n";
		mu+= "<b><br><br>NTAI XE7c  Log File <br>\n<span class='c'>Programmed and maintained by AF Copyright (C) 2006 AF<br>\nReleased under the GPL 2.0 Liscence </p></span></b><br> \n<table width='98%'border='0' cellpadding='0' cellspacing='0' bordercolor='#999999'>\n";
		header(mu);
		time_t tval;
		char buf[128];
		tval = time(NULL);
		tm* now = localtime(&tval);
		strftime(buf,sizeof(buf),"<b><p>Game started: <span class='c'> %B %d %Y %I:%M %p.</p></span></b>",now);
		print(buf);
		CSunParser cp(G);
		cp.LoadFile("modinfo.tdf");
		logFile << " :: " << cp.SGetValueMSG("MOD\\Name") << endl;
		if(First == true) logFile << " :: First instance of NTAI" << endl;
	}
}

void Log::print(string message){
	if(message.empty() == true) return;
	string gtime;
	string getime = GameTime();
	if(plaintext == true){
		gtime = getime + " " + message + "\n";
	}else{
		gtime = "\n<tr><th width='8%' scope='row'><b>" + getime + "</b></th>\n<td width='92%'>" + message + "</td></tr>\n";
	}
	header(gtime);
}

void Log::header(string message){
	if(message.empty() == true) return;
	if(verbose == true){
		G->cb->SendTextMsg(message.c_str(),1);
	}
	string gtime;
	if(plaintext == true){
		gtime = message;
	}else{
		gtime = "\n<tr><th width='100%' scope='row'><b>" + message + "</th></tr>\n";
	}
	if (logFile.is_open() == true){
		logFile << gtime.c_str();
	}
}

void Log::iprint(string message){
	string gtime = GameTime() + message;
    G->cb->SendTextMsg(gtime.c_str(),1);
	print(message);
}

void Log::Close(){
	if(logFile.is_open() == true){
		if(plaintext){
			header(" :: Closing LogFile...\n");
		}else{
            print("<b><span class='c'>Closing logfile...</b></span>");
			header("\n</table>");
		}
		logFile.close();
	}
}

void Log::Flush(){
	if(logFile.is_open() == true){
		logFile.flush();
	}
}

void Log::Message(string msg,int player){
	if(plaintext == true){
		string m = "[" + PlayerNames[player] + "] " + GameTime() + " :: " + msg + "\n";
		header(m);
		return;
	}else{
		print("<span class='c'>" + msg + "</span>");
	}
}

void Log::eprint(string message){
	if(plaintext == true){
		G->cb->SendTextMsg(message.c_str(),3);
		print(message);
		return;
	} else{
		G->cb->SendTextMsg(message.c_str(),1000);
		string msg = "<span class='e'>" + message + "</span>";
		print(msg);
	}
}

Log& Log::operator<< (const char* c){
	this->header(c);
	return *this;
}

Log& Log::operator<< (string s){
	this->header(s);
	return *this;
}

Log& Log::operator<< (int i){
	char c[10];
	sprintf(c,"%i",i);
	string s = c;
	header(s);
	return *this;
}

Log& Log::operator<< (float f){
	char c [15];
	sprintf(c,"%f",f);
	header(c);
	return *this;
}
