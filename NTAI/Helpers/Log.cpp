#include "Log.h"
#include <time.h>
#ifdef _MSC_VER
#include <windows.h>
#endif

FILE *logFile = 0;
IAICallback* cb;

Log::Log(IAICallback* aicb){
	cb = aicb;
}
Log::~Log(){}


string Log::GameTime(){
	char min[2];
	char sec[2];
	int Time = cb->GetCurrentFrame();
	Time = Time/30;
	int Seconds = Time%60;
	int Minutes = Time/60;
	if(Seconds+Minutes == 0){
		return "";
	}
	sprintf(min,"%i",Minutes);
	sprintf(sec,"%i",Seconds);
	string stime="[";
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
	return stime.c_str();
}

void Log::Open(){
	logFile = fopen ("AILog.htm", "a+");
	fseek(logFile,0,SEEK_END);
	string mu ="<style type='text/css'>\n<!--\nbody,td,th {\n	font-family: sans-serif;\n	color: #FFFFFF;\nfont-size: 12px;\n\n}\nbody {\n	background-color: #333333;\n\n}\n.chat {color: #FF0000}\n-->\n</style>\n";
	mu+= "<b><br><br>NTAI V0.3 Log File <br>\n<span class='chat'>Programmed and maintained by Redstar & co <br>\nReleased under the GPL 2.0 Liscence </p></span></b><br> \n";
	fputs(mu.c_str(), logFile);
	time_t tval;
	char buf[128];
	tval = time(NULL);
	tm* now = localtime(&tval);
	strftime(buf,sizeof(buf),"<b><p>Game started: <span class='chat'> %B %d %Y %I:%M %p.</p></span></b>",now);
	print(buf);
}

void Log::print(string message){
	string gtime = "\n	<table width='96%'  border='1' cellpadding='0' cellspacing='0' bordercolor='#999999'>\n  <tr>\n    <th width='8%' scope='row'><b>";
	gtime +=GameTime().c_str();
	gtime +="	</b></th>\n    <td width='92%'>";
	gtime += message.c_str();
	gtime += "</td>\n  </tr>\n</table>\n";
	static char buf[300];
	if (logFile)
	{
		fputs (gtime.c_str(), logFile);
	}
}

void Log::iprint(string message){
	string gtime = GameTime().c_str();
	gtime += message.c_str();
	cb->SendTextMsg(gtime.c_str(),1);
	print(message.c_str());
}

void Log::Close(){
	if (logFile) {
		print ("<b><span class='chat'>Closing logfile...</b></span>");
		fclose (logFile);
		logFile = 0;
	}
}

void Log::Flush(){
	if(logFile){
        fflush(logFile);
	}
}

void Log::Message(string msg,int player){
	string message = "<span class='chat'>";
	message += msg.c_str();
	message += "</span>";
	print(message);
}