#include "GUIsharingDialog.h"
#include "GUItable.h"
#include "GUIbutton.h"
#include "GUIslider.h"
#include "GUIstateButton.h"

#include "team.h"
#include "player.h"
#include "selectedunits.h"
#include "net.h"


GUIsharingDialog::GUIsharingDialog(): GUIdialogController()
{

}

GUIsharingDialog::~GUIsharingDialog()
{
	delete controls["sharing"];
}

void GUIsharingDialog::ButtonPressed(GUIbutton* b)
{
	if(b->Identifier()=="execute")
	{
		DialogEvent("togglehidden");
		
		if(giveUnits->State()){
			Command c;
			c.id=CMD_STOP;
			selectedUnits.GiveCommand(c,false);		//make sure the units are stopped and that the selection is transmitted
		}
		netbuf[0]=NETMSG_SHARE;
		netbuf[1]=gu->myPlayerNum;
		netbuf[2]=table->GetSelected();
		netbuf[3]=giveUnits->State();
		*(float*)&netbuf[4]=min(gs->teams[gu->myTeam]->metal, giveMetal->Position());

		*(float*)&netbuf[8]=min(gs->teams[gu->myTeam]->energy, giveEnergy->Position());
		net->SendData(netbuf,12);

		if(giveUnits->State())
			selectedUnits.ClearSelected();
	}
	else if(b->Identifier()=="togglehidden")
	{
		DialogEvent("togglehidden");
	}
}


void GUIsharingDialog::TableSelection(GUItable* table, int i,int button)
{

}

void GUIsharingDialog::DialogEvent(const std::string& event)
{
	if(event=="togglehidden")
	{
		if(controls["sharing"])
		{
			controls["sharing"]->ToggleHidden();
			controls["sharing"]->Raise();
		}
		
		giveEnergy=dynamic_cast<GUIslider*>(controls["give_energy"]);
		giveMetal=dynamic_cast<GUIslider*>(controls["give_metal"]);
		shareEnergy=dynamic_cast<GUIslider*>(controls["share_energy"]);
		shareMetal=dynamic_cast<GUIslider*>(controls["share_metal"]);

		giveUnits=dynamic_cast<GUIstateButton*>(controls["give_units"]);
		shareVision=dynamic_cast<GUIstateButton*>(controls["share_vision"]);
		
		table=dynamic_cast<GUItable*>(controls["list"]);


		if(giveEnergy)
			giveEnergy->SetMaximum(gs->teams[gu->myTeam]->energy);
		if(giveMetal)
			giveMetal->SetMaximum(gs->teams[gu->myTeam]->metal);

		if(shareEnergy)
		{
			shareEnergy->SetMaximum(gs->teams[gu->myTeam]->energyStorage);
			shareEnergy->SetPosition(gs->teams[gu->myTeam]->energyShare);
		}
		if(shareMetal)
		{
			shareMetal->SetMaximum(gs->teams[gu->myTeam]->metalStorage);
			shareMetal->SetPosition(gs->teams[gu->myTeam]->metalShare);
		}

		UpdatePlayerList();
	}
}

void GUIsharingDialog::SliderChanged(GUIslider* slider, int i)
{
	
}

void GUIsharingDialog::UpdatePlayerList()
{
	if(!table)
		return;
		
	vector<string> teams;

	for(int team=0;team<gs->activeTeams;team++)
	{
		string teamName;
		
		char buf[100];
		sprintf(buf, "Team %i", team+1);
		
		teamName=buf;

		if(gs->allies[gu->myAllyTeam][gs->team2allyteam[team]])
			teamName+="(Ally)";
		if(gs->teams[team]->isDead)
			teamName+="(Dead)";

		teamName+="\t"+gs->players[gs->teams[team]->leader]->playerName;

		// display energy and metal for allied teams
		// maybe just for those with shared mapping
		if(gs->allies[gu->myAllyTeam][gs->team2allyteam[team]])
		{
			sprintf(buf, "%i", gs->teams[team]->metal);
			teamName+=string("\t")+buf;
			sprintf(buf, "%i", gs->teams[team]->energy);
			teamName+=string("\t")+buf;
		}
		else
			teamName+="\tn/a\tn/a";


		//if(team!=gu->myTeam)
		//	teams.push_back(teamName);
	}
	
	table->ChangeList(&teams);
	
	vector<GUItable::HeaderInfo> header;
	header.push_back(GUItable::HeaderInfo("Name", 0.25));
	header.push_back(GUItable::HeaderInfo("Leader", 0.25));
	header.push_back(GUItable::HeaderInfo("Metal", 0.25));
	header.push_back(GUItable::HeaderInfo("Energy", 0.25));
	
	table->SetHeader(header);	
}


GUIframe* GUIsharingDialog::CreateControl(const std::string& type, int x, int y, int w, int h, CSunParser& parser)
{
	GUIframe *frame=NULL;
	if(type=="slider")
	{
		GUIslider *slider=new GUIslider(x, y, w, 100, makeFunctor((Functor2<GUIslider*, int>*)0, *this, &GUIsharingDialog::SliderChanged));
		
		frame=slider;
	}
	
	
	return frame;
}
