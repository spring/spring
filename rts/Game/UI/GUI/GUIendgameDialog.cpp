#include "StdAfx.h"
#include "GUIendgameDialog.h"
#include "GUItable.h"
#include "GUIbutton.h"
#include "Game/Team.h"
#include "Game/Player.h"
#include "LogOutput.h"
#include "GUImatrix.h"
#include "GUIendgameDialog.h"
#include "Game/Game.h"

extern bool globalQuit;

GUIendgameDialog::GUIendgameDialog(): GUIdialogController()
{
	stats=NULL;
	graph=NULL;
	showDif=false;
	lastSelection[0]=0;
	lastSelection[1]=0;
}

GUIendgameDialog::~GUIendgameDialog()
{
	delete controls["endgame"];
}

void GUIendgameDialog::ButtonPressed(GUIbutton* b)
{
	if(b->Identifier()=="close")
	{
		DialogEvent("togglehidden");

		globalQuit=true;
	}
	else if(b->Identifier()=="sum")
	{
		showDif=false;
		graph->SetDatasets(statistics[tableEntries[lastSelection[0]]],showDif,0,tableEntries[lastSelection[0]]);
		graph->SetDatasets(statistics[tableEntries[lastSelection[1]]],showDif,1,tableEntries[lastSelection[1]]);
		ShowGraph();
	}
	else if(b->Identifier()=="dif")
	{
		showDif=true;
		graph->SetDatasets(statistics[tableEntries[lastSelection[0]]],showDif,0,tableEntries[lastSelection[0]]);
		graph->SetDatasets(statistics[tableEntries[lastSelection[1]]],showDif,1,tableEntries[lastSelection[1]]);
		ShowGraph();
	}
	else if(b->Identifier()=="player")
	{
		ShowPlayerStats();
	}
	else if(b->Identifier()=="togglehidden")
	{
		DialogEvent("togglehidden");
	}
}



void GUIendgameDialog::TableSelection(GUItable* table, int i,int button)
{
	button/=2;		//untransform stupid  buttonMap
	lastSelection[button]=i;
	graph->SetDatasets(statistics[tableEntries[i]],showDif,button,tableEntries[i]);
}

void GUIendgameDialog::UpdateStatistics()
{
	statistics.clear();
	
	GUIgraph::Dataset data;
	
	// build statistics
	
	for(int team=0; team<gs->activeTeams; team++)
	{
		// if you're not spectating and the current team is not allied to you, then don't show the
		// teams information
		if ( !gu->spectating && !gs->Ally(gu->myAllyTeam, gs->AllyTeam(team)) )
			continue;
		char buf[50];
		sprintf(buf, "Team %i", team+1);

		// for every team, take the corresponding history
		std::list<CTeam::Statistics>& s=gs->Team(team)->statHistory;

		std::list<CTeam::Statistics>::iterator i=s.begin();
		std::list<CTeam::Statistics>::iterator e=s.end();

		// for every value in the teams history
		for(; i!=e; i++)
		{
			// initialize the per-team statistics array
			#define InitStat(a) a.resize(gs->activeTeams); 
			
			// set values for the teams statistics
			#define FillStat(a) a[team].caption=buf; \
								a[team].r=gs->Team(team)->color[0]/255.0f; \
								a[team].g=gs->Team(team)->color[1]/255.0f; \
								a[team].b=gs->Team(team)->color[2]/255.0f; 

			// add a value, initializing the array if it isn't already,
			// and setting suitable values for team name and color
			// first parameter is the value name, second is the value itself
			#define AddValue(a, b) if(statistics[a].empty()) { InitStat(statistics[a]); } FillStat(statistics[a]); statistics[a][team].points.push_back(b);
		
			// add the value to the corresponding statistics
			AddValue("Metal used", i->metalUsed);
			AddValue("Energy used", i->energyUsed);
			AddValue("Metal produced", i->metalProduced);
			AddValue("Energy produced", i->energyProduced);
			
			AddValue("Metal excess", i->metalExcess);
			AddValue("Energy excess", i->energyExcess);
			
			AddValue("Metal received", i->metalReceived);
			AddValue("Energy received", i->energyReceived);
			
			AddValue("Metal sent", i->metalSent);
			AddValue("Energy sent", i->energySent);
			
			AddValue("Units produced", i->unitsProduced);
			AddValue("Units died", i->unitsDied);
			
			AddValue("Units received", i->unitsReceived);
			AddValue("Units sent", i->unitsSent);
			AddValue("Units captured", i->unitsCaptured);
			AddValue("Units stolen", i->unitsOutCaptured);
			AddValue("Units killed", i->unitsKilled);

			AddValue("Damage Dealt", i->damageDealt);
			AddValue("Damage Received", i->damageReceived);
			
			#undef InitStat
			#undef FillStat
			#undef AddValue
		}
	}
	tableEntries.clear();
	
	map<string, DatasetList>::iterator i=statistics.begin();
	map<string, DatasetList>::iterator e=statistics.end();
	
	// then, for every type of statistic added
	for(;i!=e; i++)
	{
		// add the type of statistic to the list of types	
		tableEntries.push_back(i->first);
	}

	// finally display the 
	stats->ChangeList(&tableEntries);

	if(stats->GetSelected()<tableEntries.size()){
		graph->SetDatasets(statistics[tableEntries[lastSelection[0]]],showDif,0,tableEntries[lastSelection[0]]);
		graph->SetDatasets(statistics[tableEntries[lastSelection[1]]],showDif,1,tableEntries[lastSelection[1]]);
	}

}

void GUIendgameDialog::DialogEvent(const std::string& event)
{
	guicontroller->AddText(event);
	if(event=="togglehidden")
	{
		if(controls["endgame"])
		{
			controls["endgame"]->ToggleHidden();
			controls["endgame"]->Raise();
		}

		graph=dynamic_cast<GUIgraph*>(controls["graph"]);
		stats=dynamic_cast<GUItable*>(controls["stats"]);
		
		UpdateStatistics();
	}
}

GUIframe* GUIendgameDialog::CreateControl(const std::string& type, int x, int y, int w, int h, TdfParser & parser)
{
	GUIframe *frame=NULL;
	if(type=="graph")
	{
		frame=new GUIgraph(x, y, w, h);		
	} else if(type=="matrix"){
		frame=new GUImatrix(x, y, w, h);
	}
	
	return frame;
}

void GUIendgameDialog::PrivateDraw(void)
{
}

void GUIendgameDialog::ShowPlayerStats(void)
{
	if(!controls["graph"]->isHidden())
		controls["graph"]->ToggleHidden();
	if(!controls["stats"]->isHidden())
		controls["stats"]->ToggleHidden();
	if(controls["playerstats"]->isHidden())
		controls["playerstats"]->ToggleHidden();

	vector<vector<string> > values;
	vector<string> tooltips;

	values.push_back(vector<string>());
	values[0].push_back("Name");
	tooltips.push_back("Player Name");
	values[0].push_back("MC/m");
	tooltips.push_back("Mouse clicks per minute");
	values[0].push_back("MP/m");
	tooltips.push_back("Mouse movement in pixels per minute");
	values[0].push_back("KP/m");
	tooltips.push_back("Keyboard presses per minute");
	values[0].push_back("Cmds/m");
	tooltips.push_back("Unit commands per minute");
	values[0].push_back("ACS");
	tooltips.push_back("Average command size (units affected per command)");

	int lines=1;
	char c[500];
	for(int a=0;a<gs->activePlayers;++a){
		if(gs->players[a]->active){
			values.push_back(vector<string>());
			values[lines].push_back(gs->players[a]->playerName);
			sprintf(c,"%i",(int)(gs->players[a]->currentStats->mouseClicks*60/game->totalGameTime));
			values[lines].push_back(c);
			sprintf(c,"%i",(int)(gs->players[a]->currentStats->mousePixels*60/game->totalGameTime));
			values[lines].push_back(c);
			sprintf(c,"%i",(int)(gs->players[a]->currentStats->keyPresses*60/game->totalGameTime));
			values[lines].push_back(c);
			sprintf(c,"%i",(int)(gs->players[a]->currentStats->numCommands*60/game->totalGameTime));
			values[lines].push_back(c);
			sprintf(c,"%i",(int)
				( gs->players[a]->currentStats->numCommands != 0 ) ? 
				( gs->players[a]->currentStats->unitCommands/gs->players[a]->currentStats->numCommands) :
				( 0 ));
			values[lines].push_back(c);
			lines++;
		}
	}

	GUImatrix* playerstats=dynamic_cast<GUImatrix*>(controls["playerstats"]);
	playerstats->SetValues(values,tooltips);
}

void GUIendgameDialog::ShowGraph(void)
{
	if(controls["graph"]->isHidden())
		controls["graph"]->ToggleHidden();
	if(controls["stats"]->isHidden())
		controls["stats"]->ToggleHidden();
	if(!controls["playerstats"]->isHidden())
		controls["playerstats"]->ToggleHidden();
}
