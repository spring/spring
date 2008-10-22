#include "../Core/include.h"

namespace ntai {
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
		return " < Frame: " + to_string(G->cb->GetCurrentFrame()) + " >";
	}

	string Log::GameTime(){
		int Time = G->cb->GetCurrentFrame();
		Time = Time/30;
		int Seconds = Time%60;
		int Minutes = (Time/60)%60;
		int hours = 0;
		if(Time>3600) hours = (Time-(Time%3600))/3600;
		if(Seconds+Minutes+hours == 0){
			return "[-]";
		}
		string stime="[";
		if(hours>0){
			if(hours<10) stime += "0";
			stime += to_string(hours);
			stime += ":";
		}
		if(Minutes<10){
			stime += "0";
		}
		stime += to_string(Minutes);
		stime += ":";
		if(Seconds <10){
			stime += "0";
		}
		stime += to_string(Seconds);
		stime += "]";

		return stime;
	}

	bool Log::FirstInstance(){
		return First;
	}

	string GetSysTime(){
		time_t now1;
		time(&now1);//struct
		tm *now2;
		now2 = localtime(&now1);
		string s = string("|")+to_string(now2->tm_hour)+ string(":")+ to_string(now2->tm_min)+string(":")+ to_string(now2->tm_sec)+string("|");
		return s;
	}

	void Log::Open(bool plain){
		char buffer[1000];
		if( Lmagic != 95768){
			Lmagic = 95768;
			First = true;
		}
		if(plain == true){ // Textfile Logging
			plaintext = true;
			//		char c[400];
			time_t now1;
			time(&now1);
			struct tm *now2;
			now2 = localtime(&now1);
			string filename = G->info->datapath + slash + "Logs" + slash;

			//             DDD MMM DD HH:MM:SS YYYY_X - NTAI.log
			filename += to_string(now2->tm_mon+1)+"-" +to_string(now2->tm_mday) + "-" +to_string(now2->tm_year + 1900) +"-" +to_string(now2->tm_hour) +"_" +to_string(now2->tm_min) +"["+to_string(G->Cached->team)+"]XE9.79.log";
			//sprintf(c, "%2.2d-%2.2d-%4.4d %2.2d%2.2d [%d]XE9.79.log",
			//		now2->tm_mon+1, now2->tm_mday, now2->tm_year + 1900, now2->tm_hour,
			//		now2->tm_min, G->Cached->team);
			//filename += c;
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
			header(" :: NTAI XE9.79 Log File \n :: Programmed and maintained by AF/T.Nowell \n :: Copyright (C) 2004-7 Tom Nowell/AF \n");
			logFile << " :: Game started: " << now2->tm_mday << "." << now2->tm_mon << "." << 1900 + now2->tm_year << "  " << now2->tm_hour << ":" << now2->tm_min << ":" << now2->tm_sec << endl << endl <<  flush;
			TdfParser cp(G);
			cp.LoadFile("modinfo.tdf");
			logFile << " :: " << cp.SGetValueMSG("MOD\\Name") << endl <<  flush;
			logFile << " :: " << cp.SGetValueMSG("MOD\\Description") << endl <<  flush;
			if(First == true) logFile << " :: First instance of NTAI" << endl;
			startupscript = new TdfParser(G);
			startupscript->LoadFile("script.txt");
			vector<string> names;
			for(int n=0; n<MAX_TEAMS; n++){
				string s = startupscript->SGetValueDef("", string("GAME\\PLAYER") + to_string(n) + "\\name");
				if(s != string("")){
					names.push_back(s + " : player :: " + to_string(n));
					PlayerNames[n] = s;
				} else{
					PlayerNames[n] = "spring_engine";
				}
				logFile << " :: " << PlayerNames[n] << endl;
			}
			logFile << " :: AI DLL's in game" << endl <<  flush;
			vector<string> AInames;
			for(int n=0; n<MAX_TEAMS; n++){
				string s = startupscript->SGetValueDef("", string("GAME\\TEAM") + to_string(n) + "\\AIDLL");
				if(s != string("")){
					AInames.push_back(s + " : AI :: " + to_string(n));
					logFile << " :: "<<s << " : AI :: " << to_string(n) << endl <<  flush;
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
			sprintf(c, "%2.2d-%2.2d-%4.4d %2.2d%2.2d [%d] - NTAIXE9.79.htm",
			now2->tm_mon+1, now2->tm_mday, now2->tm_year + 1900, now2->tm_hour,
			now2->tm_min, G->Cached->team);
			filename += c;
			strcpy(buffer, filename.c_str());
			G->cb->GetValue(AIVAL_LOCATE_FILE_W, buffer);
			logFile.open(buffer);
			plaintext = false;
			string mu ="</table><style type='text/css'>\n<!--\nbody,td,th {\n	font-family: sans-serif;\n	color: #111111;\nfont-size: 12px;\n\n}\nbody {\n	background-color: #FFFFFF;\n\n}\n.c {color: #FF2222}\n.e {color: #FFCC11}\n-->\n</style>\n";
			mu+= "<b><br><br>NTAI XE Log File <br>\n<span class='c'>Programmed and maintained by AF Copyright (C) 2006 AF<br>\nReleased under the GPL 2.0 Liscence </p></span></b><br> \n<table width='98%'border='0' cellpadding='0' cellspacing='0' bordercolor='#999999'>\n";
			header(mu);
			time_t tval;
			char buf[128];
			tval = time(NULL);
			tm* now = localtime(&tval);
			strftime(buf, sizeof(buf), "<b><p>Game started: <span class='c'> %B %d %Y %I:%M %p.</p></span></b>", now);
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
			gtime = GameTime() + GetSysTime() +FrameTime() + message + "\n";
		}else{
			gtime = "\n<tr><th width='8%' scope='row'><b>" + GameTime() + FrameTime() +"</b></th>\n<td width='92%'>" + message + "</td></tr>\n";
		}
		header(gtime);
	}

	void Log::header(string message){
		if(message.empty() == true) return;
		if(verbose == true){
			G->cb->SendTextMsg(message.c_str(), 1);
		}
		string gtime="";
		if(plaintext == true){
			gtime = message;
		}else{
			gtime = string("\n<tr><th width='100%' scope='row'><b>") + message + string("</th></tr>\n");
		}
		if (logFile.is_open() == true){
	#ifdef TNLOG
			cout << gtime << endl<< flush;
	#endif
	logFile << gtime << flush;
		}
	}

	void Log::iprint(string message){
		string gtime = GameTime() + message;
		G->cb->SendTextMsg(gtime.c_str(), 1);
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

	void Log::Message(string msg, int player){
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
			G->cb->SendTextMsg(message.c_str(), 3);
			print(message);
			return;
		} else{
			G->cb->SendTextMsg(message.c_str(), 1000);
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
		string s = to_string(i);
		header(s);
		return *this;
	}
	Log& Log::operator<< (uint i){
		string s = to_string(i);
		header(s);
		return *this;
	}

	Log& Log::operator<< (float f){
		header(to_string(f));
		return *this;
	}

	Log& Log::operator<< (float3 f){
		header(to_string(f.x)+","+to_string(f.y)+","+to_string(f.z));
		return *this;
	}

	bool Log::Verbose(){
		if(verbose == true){
			verbose = false;
		}else{
			verbose = true;
		}
		return verbose;
	}

	bool Log::IsVerbose(){
		return verbose;
	}
}
