#include "../Core/helper.h"

int Lmagic;

Log::Log(){
	verbose = false;
	First = false;
}

Log::~Log(){
G = 0;
}

void Log::Set(Global* GL){
	G = GL;
	if(GL ==0){
		int jk = 3;
	}
	if(G ==0){
		int jk = 3;
	}
}
string Log::FrameTime(){
	char c[20];
	int Time = G->cb->GetCurrentFrame();
	sprintf(c,"%i",Time);
	string R = " < Frame: ";
	R += c;
	R += " >";
	return R;
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
	char buffer[1000];
	if( Lmagic != 95768){
		Lmagic = 95768;
		First = true;
	}
	if(plain == true){ // Textfile Logging
		plaintext = true;
		char c[400];
		time_t now1;
		time(&now1);
		struct tm *now2;
		string filename = G->info->datapath + slash + "Logs" + slash;
		now2 = localtime(&now1);
		//             DDD MMM DD HH:MM:SS YYYY_X - NTAI.log
		sprintf(c, "%2.2d-%2.2d-%4.4d %2.2d%2.2d [%d]XE9RC22.log",
				now2->tm_mon+1, now2->tm_mday, now2->tm_year + 1900, now2->tm_hour,
				now2->tm_min, G->Cached->team);
		filename += c;
		strcpy(buffer, filename.c_str());
		G->cb->GetValue(AIVAL_LOCATE_FILE_W, buffer);
		logFile.open(buffer);
		if(logFile.is_open() == false){
			logFile.close();
			logFile.open(buffer);
			if(logFile.is_open() == false){
				iprint(string("Error!!! ") + filename + string(" refused to open!"));
				verbose = true;
				return;
			}
		}
		header(" :: NTAI XE9RC22 Log File \n :: Programmed and maintained by AF \n :: Copyright (C) 2006 AF \n :: Released under the GPL 2.0 Liscence \n");
		logFile << " :: Game started: " << now2->tm_mday << "." << now2->tm_mon << "." << 1900 + now2->tm_year << "  " << now2->tm_hour << ":" << now2->tm_min << ":" << now2->tm_sec << endl << endl <<  flush;
		TdfParser cp(G);
		cp.LoadFile("modinfo.tdf");
		logFile << " :: " << cp.SGetValueMSG("MOD\\Name") << endl <<  flush;
		logFile << " :: " << cp.SGetValueMSG("MOD\\Description") << endl <<  flush;
		if(First == true) logFile << " :: First instance of NTAI" << endl;
		TdfParser cq(G);
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
		logFile << " :: AI DLL's in game" << endl <<  flush;
		vector<string> AInames;
		for(int n=0; n<MAX_TEAMS; n++){
    		char c[8];
			sprintf(c, "%i", n);
			string s = cq.SGetValueDef("", string("GAME\\TEAM") + string(c) + "\\AIDLL");
			if(s != string("")) AInames.push_back(s + " : AI :: " + c);
		}
		if(names.empty() == false){
			for(vector<string>::iterator i = AInames.begin(); i != AInames.end(); ++i){
				logFile << " :: " << *i << endl <<  flush;
			}
		}
		logFile << endl <<  flush;
	}else{ // HTML logging
		char c[400];
		time_t now1;
		time(&now1);
		struct tm *now2;
		now2 = localtime(&now1);
		string filename = G->info->tdfpath;
		filename += slash;
		filename += "Logs";
		filename += slash;
		//                                      DDD MMM DD HH:MM:SS YYYY_X - NTAI.htm
		sprintf(c, "%2.2d-%2.2d-%4.4d %2.2d%2.2d [%d] - NTAIXE9.htm",
				now2->tm_mon+1, now2->tm_mday, now2->tm_year + 1900, now2->tm_hour,
				now2->tm_min, G->Cached->team);
		filename += c;
		strcpy(buffer, filename.c_str());
		G->cb->GetValue(AIVAL_LOCATE_FILE_W, buffer);
		logFile.open(buffer);
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
		TdfParser cp(G);
		cp.LoadFile("modinfo.tdf");
		logFile << " :: " << cp.SGetValueMSG("MOD\\Name") << endl <<  flush;
		if(First == true) logFile << " :: First instance of NTAI" << endl <<  flush;
	}
}

void Log::print(string message){
	if(message.empty() == true) return;
	string gtime;
	if(plaintext == true){
		gtime = GameTime() + FrameTime() + message + "\n";
	}else{
		gtime = "\n<tr><th width='8%' scope='row'><b>" + GameTime() + FrameTime() +"</b></th>\n<td width='92%'>" + message + "</td></tr>\n";
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
		gtime = string("\n<tr><th width='100%' scope='row'><b>") + message + string("</th></tr>\n");
	}
	if (logFile.is_open() == true){
		logFile << gtime.c_str() << flush;
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
		string m = "[" + PlayerNames[player] + "] " + GameTime() + FrameTime() +  " :: " + msg + "\n";
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
