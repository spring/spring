#include "StdAfx.h"
#include "GUIsharingDialog.h"
#include "GUItable.h"
#include "GUIbutton.h"
#include "GUIslider.h"
#include "GUIstateButton.h"

#include "Game/Team.h"
#include "Game/Player.h"
#include "Game/SelectedUnits.h"
#include "Net.h"


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
		net->SendData<unsigned char, unsigned char, unsigned char, float, float>(
				NETMSG_SHARE, gu->myPlayerNum, table->GetSelected(), giveUnits->State(),
				min(gs->Team(gu->myTeam)->metal, (double)giveMetal->Position()),
				min(gs->Team(gu->myTeam)->energy, (double)giveEnergy->Position()));

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
 			giveEnergy->SetMaximum((int)gs->Team(gu->myTeam)->energy);
		if(giveMetal)
 			giveMetal->SetMaximum((int)gs->Team(gu->myTeam)->metal);

		if(shareEnergy)
		{
 			shareEnergy->SetMaximum((int)gs->Team(gu->myTeam)->energyStorage);
 			shareEnergy->SetPosition((int)gs->Team(gu->myTeam)->energyShare);
		}
		if(shareMetal)
		{
 			shareMetal->SetMaximum((int)gs->Team(gu->myTeam)->metalStorage);
 			shareMetal->SetPosition((int)gs->Team(gu->myTeam)->metalShare);
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

		if(gs->Ally(gu->myAllyTeam, team))
			teamName+="(Ally)";
		if(gs->Team(team)->isDead)
			teamName+="(Dead)";

		teamName+="\t"+gs->players[gs->Team(team)->leader]->playerName;

		// display energy and metal for allied teams
		// maybe just for those with shared mapping
		if(gs->Ally(gu->myAllyTeam,gs->AllyTeam(team)))
		{
			sprintf(buf, "%i", gs->Team(team)->metal);
			teamName+=string("\t")+buf;
			sprintf(buf, "%i", gs->Team(team)->energy);
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


GUIframe* GUIsharingDialog::CreateControl(const std::string& type, int x, int y, int w, int h, TdfParser& parser)
{
	GUIframe *frame=NULL;
	if(type=="slider")
	{
		GUIslider *slider=new GUIslider(x, y, w, 100, makeFunctor((Functor2<GUIslider*, int>*)0, *this, &GUIsharingDialog::SliderChanged));
		
		frame=slider;
	}
	
	
	return frame;
}
