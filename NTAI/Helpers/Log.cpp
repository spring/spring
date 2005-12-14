//#include "Log.h"
#include "../helper.h"
#include <time.h>
#ifdef _MSC_VER
#include <windows.h>
#endif

FILE *logFile = 0;

Log::Log(Global* GL){
	G = GL;
	verbose = false;
}

Log::~Log(){}

string Log::GameTime(){
	char min[2];
	char sec[2];
	char hour[2];
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

void Log::Open(bool plain){
	if(plain == true){
		plaintext = true;
		logFile = fopen ("AILog.txt", "a+");
		fseek(logFile,0,SEEK_END);
		string mu ="\n";
		mu+= "NTAI V0.28.10 Log File \nProgrammed and maintained by Alantai/Redstar & co \nReleased under the GPL 2.0 Liscence \n\n";
		fputs(mu.c_str(), logFile);
		time_t tval;
		char buf[128];
		tval = time(NULL);
		tm* now = localtime(&tval);
		strftime(buf,sizeof(buf),"Game started:  %B %d %Y %I:%M %p.",now);
		print(buf);
	}else{
		plaintext = false;
		logFile = fopen ("AILog.htm", "a+");
		fseek(logFile,0,SEEK_END);
		string mu ="</table><style type='text/css'>\n<!--\nbody,td,th {\n	font-family: sans-serif;\n	color: #111111;\nfont-size: 12px;\n\n}\nbody {\n	background-color: #FFFFFF;\n\n}\n.c {color: #FF2222}\n.e {color: #FFCC11}\n-->\n</style>\n";
		mu+= "<b><br><br>NTAI V0.28 b5 Log File <br>\n<span class='c'>Programmed and maintained by Alantai/Redstar & co <br>\nReleased under the GPL 2.0 Liscence </p></span></b><br> \n<table width='98%'border='0' cellpadding='0' cellspacing='0' bordercolor='#999999'>\n";
		fputs(mu.c_str(), logFile);
		time_t tval;
		char buf[128];
		tval = time(NULL);
		tm* now = localtime(&tval);
		strftime(buf,sizeof(buf),"<b><p>Game started: <span class='c'> %B %d %Y %I:%M %p.</p></span></b>",now);
		print(buf);
	}
}

void Log::print(string message){
	if(verbose){
		G->cb->SendTextMsg(message.c_str(),1);
	}
	string gtime;
	if(plaintext == true){
		gtime +=GameTime().c_str();
		gtime += " ";
		gtime += message.c_str();
	}else{
		gtime = "\n<tr><th width='8%' scope='row'><b>";
		gtime +=GameTime().c_str();
		gtime +="</b></th>\n<td width='92%'>";
		gtime += message.c_str();
		gtime += "</td></tr>\n";
	}
	if (logFile){
		fputs (gtime.c_str(), logFile);
	}	
}

void Log::iprint(string message){
	string gtime = GameTime().c_str();
	gtime += message.c_str();
    G->cb->SendTextMsg(gtime.c_str(),1);
	print(message.c_str());
}

void Log::Close(){
	if(logFile){
		print("<b><span class='c'>Closing logfile...</b></span>");
		fputs("\n</table>",logFile);
		fclose(logFile);
		logFile = 0;
	}
}

void Log::Flush(){
	if(logFile){
        fflush(logFile);
	}
}

void Log::Message(string msg,int player){
	if(plaintext == true){
		print(msg);
		return;
	}
	string message = "<span class='c'>";
	message += msg.c_str();
	message += "</span>";
	print(message);
}

void Log::eprint(string message){
	if(plaintext == true){
		print(message);
		return;
	}
	G->cb->SendTextMsg(message.c_str(),1000);
	string msg = "<span class='e'>";
	msg += message.c_str();
	msg += "</span>";
	print(msg);
}

