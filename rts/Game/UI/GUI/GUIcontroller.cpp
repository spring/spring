#include "StdAfx.h"
#include "GUIcontroller.h"
#include "TdfParser.h"

#include "GUIpane.h"
#include "GUIbutton.h"
#include "GUIswitchBar.h"
#include "GUIimage.h"
#include "GUIconsole.h"
#include "GUIlabel.h"
#include "GUIstateButton.h"
#include "GUIgame.h"
#include "GUItab.h"
#include "GUIbuildMenu.h"
#include "GUIcenterBuildMenu.h"
#include "GUIminimap.h"
#include "GUIfont.h"
#include "GUIinput.h"
#include "GUIresourceBar.h"
#include "GUIallyResourceBar.h"
#include "FileSystem/FileHandler.h"
#include "Game/SelectedUnits.h"
#include "Game/command.h"
#include "float3.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Units/UnitHandler.h"
#include "Game/GameHelper.h"
#include "Game/Camera.h"
#include "Game/UI/MouseHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Game/Game.h"
#include "Net.h"
#include "GUIsharingDialog.h"
#include "GUIendgameDialog.h"
#include "GUItable.h"
#include "GUIinfoSelection.h"
#include <stdarg.h>
#include "SDL_types.h"
#include "SDL_keysym.h"

/* 
Remaining TODOs:
- mouse look mode doesn't show centered cursor (probably GUIgame)
- area and multi unit commands don't work (GUIgame)
- console thingy doesn't work (fill in GUIcontroller::ConsoleInput)
- add clipping to GUIminimap

Everything else should work, save for small stuff like too fast/slow
scrolling (adjust in GUIgame::MouseMoveAction).
*/


using namespace std;

// which modifiers was the key pressed with?
map<string, string> keyDownMap;
GUIcontroller* guicontroller=NULL;
map<string, string> bindings;
//  what is the human-readable name of a specific command id?
map<int, string> commandNames;
// which command does this button belong to?
map<string, CommandDescription> commandDesc;
// which dialog controller has which token?
map<string, GUIdialogController*> dialogControllers;

GUIcontroller& GUIcontroller::operator<< (const char* c)
{
	for(unsigned int a=0;a<strlen(c);a++){
		if(c[a]!='\n'){
			tempstring+=c[a];
		} else {
			AddText(tempstring);
			tempstring="";
			break;
		}
	}
	return *this;
}
GUIcontroller& GUIcontroller::operator<< (int i)
{
	char t[50];
	sprintf(t,"%d ",i);
	tempstring+=t;
	return *this;
}
GUIcontroller& GUIcontroller::operator<< (float f)
{
	char t[50];
	sprintf(t,"%f ",f);
	tempstring+=t;
	return *this;
}


void GUIcontroller::SetLastMsgPos(float3 pos)
{
	lastMsgPos=pos;
}

void GUIcontroller::AddText(const char* fmt, ...)
{
	char text[500];
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing

	va_start(ap, fmt);									// Parses The String For Variables
	    vsprintf(text, fmt, ap);						// And Converts Symbols To Actual Numbers
	va_end(ap);	

	console->AddText(text);
}


void GUIcontroller::AddText(const std::string& text)
{
	AddText("%s",text.c_str());	
}

void GUIcontroller::LoadInterface(const std::string& name)
{
	TdfParser parser("guis/standard.tdf");

	if(mainFrame)
		delete mainFrame;
	console=NULL;
	controls.clear();
	
	mainFrame=new GUIframe(0, 0, gu->screenx, gu->screeny);

	gameControl = new GUIgame(this);
	mainFrame->AddChild(gameControl);

	CreateUIElement(parser, mainFrame, name);

	UpdateCommands();
}

GUIcontroller::GUIcontroller()
{
	guicontroller=this;
	if(!guifont)
		guifont=new GUIfont("Luxi.ttf", 10);

	console=NULL;

	dialogControllers["sharing"]=new GUIsharingDialog();
	dialogControllers["endgame"]=new GUIendgameDialog();

	LoadInterface("gamegui");

	TdfParser keyparser("guis/bindings.tdf");

	const map<string, string> temp = keyparser.GetAllValues("bindings");
	bindings.insert(temp.begin(), temp.end());


	// initialize command names
	commandNames[CMD_STOP]="stop";
	commandNames[CMD_MOVE]="move";
	commandNames[CMD_PATROL]="patrol";
	commandNames[CMD_ATTACK]="attack";
	commandNames[CMD_AREA_ATTACK]="area_attack";
	commandNames[CMD_GUARD]="guard";
	commandNames[CMD_REPAIR]="repair";
	commandNames[CMD_FIRE_STATE]="firestate";
	commandNames[CMD_MOVE_STATE]="movestate";
	commandNames[CMD_CLOAK]="visibility";
	commandNames[CMD_SELFD]="selfdestruct";
	commandNames[CMD_LOAD_UNITS]="loadunits";
	commandNames[CMD_UNLOAD_UNITS]="unloadunits";
	commandNames[CMD_UNLOAD_UNIT]="unloadunit";
	commandNames[CMD_ONOFF]="onoff";
	commandNames[CMD_RECLAIM]="reclaim";
	commandNames[CMD_RESTORE]="restore";
	commandNames[CMD_DGUN]="dgun";
	commandNames[CMD_STOCKPILE]="stockpile";
	commandNames[CMD_AISELECT]="aiselect";
	commandNames[CMD_GROUPSELECT]="groupselect";
	commandNames[CMD_GROUPADD]="groupadd";
	commandNames[CMD_GROUPCLEAR]="groupclear";
	commandNames[CMD_REPEAT]="repeat";
	commandNames[CMD_TRAJECTORY]="trajectory";
	commandNames[CMD_RESURRECT]="resurrect";
	commandNames[CMD_CAPTURE]="capture";	


}

GUIcontroller::~GUIcontroller()
{
	map<string, GUIdialogController*>::iterator i=dialogControllers.begin();
	map<string, GUIdialogController*>::iterator e=dialogControllers.end();
	for(;i!=e; i++)
		delete i->second;
}

void GUIcontroller::ButtonPressed(GUIbutton* button)
{
	Event(button->Identifier());
}

void GUIcontroller::StateChanged(GUIstateButton* button, int state)
{
	string command=button->Identifier();

	if(!command.compare(0, 10, "selection_"))
	{
		CommandDescription desc=commandDesc[command];

		Command c;
		c.id=desc.id;
		c.params.push_back(state);

		selectedUnits.GiveCommand(c);
	}
}

void GUIcontroller::BuildMenuClicked(UnitDef* ud)
{
	EventAction("build_"+ud->name);
}

void GUIcontroller::TableSelection(GUItable* table, int i,int button)
{
	string command=table->Identifier();

	if(!command.compare(0, 10, "selection_"))
	{
		CommandDescription desc=commandDesc[command];

		Command c;
		c.id=desc.id;
		c.params.push_back(i);

		selectedUnits.GiveCommand(c);
	}
}

void GUIcontroller::ConsoleInput(const std::string& userInput)
{
	if(userInput.empty())
		return;
	if(userInput==".spectator")
		gu->spectating=true;
	if(userInput.find(".team")==0)
	{
		std::string::size_type space = userInput.find(" ");
		if (space != std::string::npos) {
			gu->spectating=false;
			gu->myTeam=atoi(&userInput.c_str()[space]);
			gu->myAllyTeam=gs->AllyTeam(gu->myTeam);
			gs->players[gu->myPlayerNum]->team=gu->myTeam;
		}
	}
	//userInput=gs->players[gu->myPlayerNum]->playerName+": "+inputText;
	net->SendMessage_CHAT(gu->myPlayerNum, userInput);
//	if(console)
//		console->AddText(userInput);
}

#ifdef _WIN32
char buttonMap[]={1, 3, 2, 4, 5, 6, 7, 8};
#else
char buttonMap[]={0 /* pad */, 1, 2, 3, 4, 5, 6, 7, 8};
#endif

bool GUIcontroller::MouseDown(int x1, int y1, int button)
{
	if(button<0||button>5) return false;
	if(!mainFrame) return false;
	if(mouse->hide)
		return guiGameControl->MouseDown(x1, y1, buttonMap[button]);
	return mainFrame->MouseDown(x1, y1, buttonMap[button]);
}

bool GUIcontroller::MouseUp(int x1, int y1, int button)
{
	if(button<0||button>5) return false;
	if(!mainFrame) return false;
	if(mouse->hide)
		return guiGameControl->MouseUp(x1, y1, buttonMap[button]);
	return mainFrame->MouseUp(x1, y1, buttonMap[button]);
}

bool GUIcontroller::MouseMove(int x1, int y1, int xrel, int yrel, int button)
{
	if(button<0||button>5) 
		return false;
	if(!mainFrame) return false;

	if(mouse->hide)
	{
		guiGameControl->MouseMove(x1, y1, xrel, yrel, buttonMap[button]);
		return false;
	}

	bool res=mainFrame->MouseMove(x1, y1, xrel, yrel, buttonMap[button]);

	return res;
}


extern Uint8 *keys;
const string GUIcontroller::Modifiers()
{
	string keyname("");

	if(keys[SDLK_LSHIFT])
		keyname.append("shift_");
	if(keys[SDLK_LCTRL])
		keyname.append("ctrl_");
	if(keys[SDLK_LALT])
		keyname.append("alt_");
	if(keys[SDLK_LMETA])
		keyname.append("meta_");
	return keyname;
}


void GUIcontroller::KeyDown(int keysym)
{
	if(mainFrame->KeyDown(keysym))
		return;
	if(guicontroller->m_SelectionKeyHandler.KeyPressed((unsigned char)keysym & 0xFF) )
	{
		return;
	}

	const string& mods=Modifiers();
	const string& key=KeyName(keysym); //SDL_GetKeyName((SDLKey)keysym);
	keyDownMap[key]=mods;

	string keyname=mods + key + "_press";
	
	if(bindings.find(keyname)!=bindings.end())
	{
		guicontroller->Event(bindings[keyname]);
	}
}

void GUIcontroller::KeyUp(int keysym)
{
	const string key=KeyName(keysym); //SDL_GetKeyName((SDLKey)keysym);
	const string mods=keyDownMap[key];

	string keyname=mods + key + "_release";

	if(bindings.find(keyname)!=bindings.end())
		guicontroller->EventAction(bindings[keyname]);
}

void GUIcontroller::Character(char c)
{
	if(mainFrame)
		mainFrame->Character(c);
}

bool GUIcontroller::Event(const std::string& event)
{
	return guicontroller->EventAction(event);
}

bool GUIcontroller::EventAction(const std::string& event1)
{
	string event(event1);

	bool eventHandled=false;
	
	while(!event.empty())
	{
		// strip leading spaces
		while(!event.empty()&&event[0]==' ')
		{
			event=event.substr(1);
		}

		int spPos=event.find(" ");
		string command=event.substr(0, spPos);

		if(!command.compare(0, 10, "selection_")||!command.compare(0, 6, "build_"))
		{
			if(commandDesc.find(command)!=commandDesc.end())
			{
				CommandDescription &desc=commandDesc[command];
			
				gameControl->StartCommand(desc);
			
				eventHandled=true;
			}
		}
		else if(!command.compare(0, 7, "gadget_"))
		{
			command=command.substr(7);
			size_t i=command.find("_");
			string target=command.substr(0, i);
			command=command.substr(i+1, string::npos);

			GUIframe *control=controls[target];

			if(control)
				control->EventAction(command);
			eventHandled=true;
		}
		else if(!command.compare(0, 7, "dialog_"))
		{
			command=command.substr(7);
			size_t i=command.find("_");
			string target=command.substr(0, i);
			command=command.substr(i+1, string::npos);

			GUIdialogController *controller=dialogControllers[target];
			if(controller)
				controller->DialogEvent(command);
			eventHandled=true;
		}
		else if(command=="traditional_gui")
			LoadInterface("talike");
		else if(command=="standard_gui")
		{
			TdfParser keyparser("guis/bindings.tdf");

			const map<string, string> temp = keyparser.GetAllValues("bindings");
			bindings.insert(temp.begin(), temp.end());
			
			LoadInterface("gamegui");
		}
		else
		{
			if(gameControl)
				eventHandled|=gameControl->EventAction(command);
		}
		
		if(spPos!=string::npos)
		{
			event=event.substr(spPos);
		}
		else
			event="";
	}
	return eventHandled;
}

// assuming the selection has just changed, this method makes sure only those buttons
// which are "legal" for the current selection are shown.
void GUIcontroller::UpdateCommands()
{
	gameControl->StopCommand();

	CSelectedUnits::AvailableCommandsStruct available;
	available=selectedUnits.GetAvailableCommands();
	commandDesc.clear();


	// first, hide all buttons which are bound to the selection
	{
		map<string, GUIframe*>::iterator i=controls.begin();
		map<string, GUIframe*>::iterator e=controls.end();

		for(; i!=e; i++)
		{
			if(!i->first.compare(0, sizeof("selection_")-1, "selection_"))
			{
				GUIframe *control=i->second;
				if(control)
					if(!control->isHidden())
						control->ToggleHidden();
			}
		}
	}
	// then show those which are described in commands.
	{
		vector<CommandDescription>::iterator i=available.commands.begin();
		vector<CommandDescription>::iterator e=available.commands.end();

		vector<CommandDescription*> opts;

		for(; i!=e; i++)
		{
			if(i->id<0)
			{
				UnitDef *ud = unitDefHandler->GetUnitByName(i->name);
				if(ud)
				{
					commandDesc["build_"+ud->name]=*i;
					opts.push_back(&commandDesc["build_"+ud->name]);
				}
			}
			else
			{
				string id=string("selection_") + commandNames[i->id];
				commandDesc[id]=*i;
				map<string, GUIframe*>::iterator cur=controls.find(id);
				if(cur!=controls.end())
				{
					cur->second->ToggleHidden();

					GUIstateButton* state=dynamic_cast<GUIstateButton*>(cur->second);
					if(state)
					{
						if(i->type==CMDTYPE_ICON_MODE||i->type==CMDTYPE_COMBO_BOX)
						{
							int s=atoi(i->params[0].c_str());
							vector<string> states;

							int size=i->params.size();
							for(int j=1; j<size; j++) {
								states.push_back(i->params[j]);
								printf("%s\n", states.back().c_str());
							}
							printf("selected: %d %s\n", s, states[s].c_str());

							state->SetStates(states);
							state->SetState(s);
						}
					}
					GUItable* table=dynamic_cast<GUItable*>(cur->second);
					if(table)
					{
						if(i->type==CMDTYPE_ICON_MODE||i->type==CMDTYPE_COMBO_BOX)
						{
							vector<GUItable::HeaderInfo> header;
							header.push_back(GUItable::HeaderInfo(i->name, 1.0));
							
							int s=atoi(i->params[0].c_str());
							vector<string> states;

							int size=i->params.size();
							for(int j=1; j<size; j++) {
								states.push_back(i->params[j]);
								printf("%s\n", states.back().c_str());
							}
							printf("selected: %d %s\n", s, states[s].c_str());

							table->SetHeader(header);
							table->ChangeList(&states);
							if (table->GetSelected()!=0) { // certainly a bad fix
								table->SetSelect(s,table->GetSelected()-1);
							}else{
								table->SetSelect(s,0);
							}
						}					
					}
					
					cur->second->SetTooltip(i->tooltip);
				}
				else
				{
					char buf[300];
					sprintf(buf, "command %s (%i) not found.\n", id.c_str(), i->id);
					console->AddText(buf);
					
				}
			}
		}
		GUIbuildMenu::SetBuildMenuOptions(opts);
	}
}




void GUIcontroller::Draw()
{
	if(selectedUnits.CommandsChanged())
		guicontroller->UpdateCommands();


	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, gu->screenx, gu->screeny,0,-1000,1000);
	glMatrixMode(GL_MODELVIEW);		
	glPushMatrix();
	glLoadIdentity();

	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

	glColor3f(1, 1, 1);
	glEnable(GL_TEXTURE_2D);

	mainFrame->Draw();

	GUIframe *over=mainFrame->ControlAtPos(mouse->lastx, mouse->lasty);

	if(over)
	{
		mouse->cursorText="";
		over->SelectCursor();

		string tooltip=over->Tooltip();

		if(!tooltip.empty())
		{
			size_t lf;
			int x=mouse->lastx;
			int y=mouse->lasty+30;
			
			if(mouse->hide)
			{
				x=gu->screenx/2;
				y=gu->screeny/2+30;
			}
			string temp=tooltip;
			int numLines=0;
			int width=0;
			int tempWidth;
			// find out number of lines and width of tooltip
			while((lf=temp.find("\n"))!=string::npos)
			{
				tempWidth=(int)guifont->GetWidth(temp.substr(0, lf));

				if(tempWidth>width) width=tempWidth;
				temp=temp.substr(lf+1, string::npos);
				numLines++;
			}
			tempWidth=(int)guifont->GetWidth(temp.substr(0, lf));
			if(tempWidth>width) width=tempWidth;
			if(tempWidth) numLines++;

			static GLuint tex=0;

			if(!tex)
				tex=Texture("bitmaps/tooltip.bmp");

			glBindTexture(GL_TEXTURE_2D, tex);
					
			glColor4f(1.0, 1.0, 1.0, 0.5);

			// draw bkgnd pane for tooltip
			glPushMatrix();
			glTranslatef(x, y, 0);
			DrawThemeRect(20, 128, width+20, (int)guifont->GetHeight()*numLines+10);
			glPopMatrix();

			x+=10;
			y+=5;

			glColor3f(1.0, 1.0, 1.0);

			temp=tooltip;
			
			// draw tooltip with line wrapping
			while((lf=temp.find("\n"))!=string::npos)
			{
				guifont->PrintColor(x, y, temp.substr(0, lf));

				y+=(int)guifont->GetHeight();
				temp=temp.substr(lf+1, string::npos);		
			}
			guifont->PrintColor(x, y, temp);		
		}
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);


	glColor3f(1, 1, 1);
}

Command GUIcontroller::GetOrderPreview()
{
	return gameControl->GetCommand();
}


const string GUIcontroller::KeyName(int k)
{
	static map<int, string> keyNames;

	if(keyNames.empty())
	{
		for(char c='a';c<='z';++c)
			keyNames[SDLK_a + c - 'a']=string(1, c);

		for(char c='0';c<='9';++c)
			keyNames[SDLK_0 + c - '0']=string(1, c);
			
		// TODO: add more key names

		keyNames[SDLK_SPACE]="space";
		keyNames[SDLK_PAUSE]="pause";
		keyNames[SDLK_RETURN]="enter";
		keyNames[SDLK_BACKSPACE]="backspace";
		keyNames[9]="tab";
		keyNames[SDLK_ESCAPE]="escape";
		keyNames[SDLK_F1]="f1";
		keyNames[SDLK_F2]="f2";
		keyNames[SDLK_F3]="f3";
		keyNames[SDLK_F4]="f4";
		keyNames[SDLK_F5]="f5";
		keyNames[SDLK_F6]="f6";
		keyNames[SDLK_F7]="f7";
		keyNames[SDLK_F8]="f8";
		keyNames[SDLK_F9]="f9";
		keyNames[SDLK_F10]="f10";
		keyNames[SDLK_F11]="f11";
		keyNames[SDLK_F12]="f12";
		keyNames[SDLK_PRINT]="printscreen";
		keyNames[SDLK_BACKQUOTE]="tilde";
		keyNames[226]="<";
		keyNames[188]=",";
		keyNames[190]=".";
		keyNames[SDLK_INSERT]="insert";
		keyNames[SDLK_DELETE]="delete";
		keyNames[SDLK_HOME]="home";
		keyNames[SDLK_END]="end";
		keyNames[SDLK_PAGEUP]="pageup";
		keyNames[SDLK_PAGEDOWN]="pagedown";
		keyNames[SDLK_UP]="up";
		keyNames[SDLK_DOWN]="down";
		keyNames[SDLK_LEFT]="left";
		keyNames[SDLK_RIGHT]="right";
		keyNames[SDLK_BREAK]=	"pause";
		keyNames[SDLK_PLUS]="+";
		keyNames[SDLK_MINUS]="-";
		keyNames[SDLK_EQUALS]="equals";
	}
	
	return keyNames[k];
}




GUIframe* GUIcontroller::CreateControl(const std::string& type, int x, int y, int w, int h, TdfParser& parser)
{
	GUIframe *frame=NULL;
 	if(type=="console")
	{
		console=new GUIconsole(x, y, w, h);
		frame=console;
	}
	else if(type=="game")
	{
		frame=new GUIgame(x, y, w, h, this);
	}
	
	
	return frame;
}



// this function recursively constructs the view hierarchy from a suitable tdf.
void GUIdialogController::CreateUIElement(TdfParser & parser, GUIframe* parent, const string& path)
{
	GUIframe *frame=NULL;
	
	// get properties
	string type=parser.SGetValueDef("none", path+"\\type");
	string identifier=parser.SGetValueDef("", path+"\\id");
	string tooltip=parser.SGetValueDef("", path+"\\tooltip");

	float x=atof(parser.SGetValueDef("-1", path+"\\x").c_str());
	float y=atof(parser.SGetValueDef("-1", path+"\\y").c_str());
	float w=atof(parser.SGetValueDef("-1", path+"\\w").c_str());
	float h=atof(parser.SGetValueDef("-1", path+"\\h").c_str());

	float x2=atof(parser.SGetValueDef("-1", path+"\\x2").c_str());
	float y2=atof(parser.SGetValueDef("-1", path+"\\y2").c_str());

	string caption=parser.SGetValueDef("No Caption", path+"\\caption");

	// scale the frame
	{
		#define ScaleX(a) if(a<=1.0&&a>=0.0) a*=parent->w;
		#define ScaleY(a) if(a<=1.0&&a>=0.0) a*=parent->h;

		ScaleX(x);
		ScaleX(x2);
		ScaleX(w);

		ScaleY(y);
		ScaleY(y2);
		ScaleY(h);

		if(x<0)
		{
			// x is not set, require w and x2
			x=parent->w-x2-w;	
		} 
		else if (w<0)
		{
			if(x2>=0)
				w=parent->w-x2-x;		// w is not set but x2 is
			else
				w=0;		// neither w nor x2 are set; may happen for autosizing buttons
		}

		if(y<0)
		{
			// y is not set, require h and y2
			y=parent->h-y2-h;	
		} 
		else if (h<0)
		{
			if(y2>=0)
				h=parent->h-y2-y;		// w is not set but x2 is
			else
				h=0;		// neither w nor x2 are set
		}

	}

	// create respective control	
	if(type=="none")
	{
		frame=parent;
	}
	else if(type=="buildmenu")
	{
		GUIbuildMenu *bm=new GUIbuildMenu((int)x, (int)y, (int)w, (int)h, makeFunctor((Functor1<UnitDef*>*)0, *this, &GUIdialogController::BuildMenuClicked));
		float num=atof(parser.SGetValueDef("64", path+"\\buildpicsize").c_str());
		ScaleX(num);
		bm->SetBuildPicSize((int)num);
		frame=bm;
	}
	else if(type=="centerbuildmenu")
	{
		GUIbuildMenu *bm=new GUIcenterBuildMenu((int)x, (int)y, (int)w, (int)h, makeFunctor((Functor1<UnitDef*>*)0, *this, &GUIdialogController::BuildMenuClicked));
		float num=atof(parser.SGetValueDef("64", path+"\\buildpicsize").c_str());
		ScaleX(num);
		bm->SetBuildPicSize((int)num);
		frame=bm;
	}
	else if(type=="frame")
	{
		frame=new GUIframe((int)x, (int)y, (int)w, (int)h);
	}
	else if(type=="minimap")
	{
		frame=new GUIminimap();
	}
	else if(type=="tab")
	{
		frame=new GUItab((int)x, (int)y, (int)w, (int)h);
	}
	else if(type=="table")
	{
		GUItable *table=new GUItable((int)x, (int)y, (int)w, (int)h, NULL, makeFunctor((Functor3<GUItable*, int, int>*)0, *this, &GUIdialogController::TableSelection));

		frame=table;
	}
	else if(type=="pane")
	{
		GUIpane *pane=new GUIpane((int)x, (int)y, (int)w, (int)h);
		pane->SetDraggable(parser.SGetValueDef("no", path+"\\draggable")=="yes");
		pane->SetFrame(parser.SGetValueDef("yes", path+"\\frame")=="yes");
		pane->SetResizeable(parser.SGetValueDef("no", path+"\\resizeable")=="yes");
		frame=pane;
	}
	else if(type=="button")
	{
		GUIbutton* b=new GUIbutton((int)x, (int)y, caption, makeFunctor((Functor1<GUIbutton*>*)0, *this, &GUIdialogController::ButtonPressed));	
		b->SetSize((int)w, (int)h);
		frame=b;
	}
	else if(type=="switchbar")
	{
		frame=new GUIswitchBar();
	}
	else if(type=="image")
	{
		frame=new GUIimage((int)x, (int)y, (int)w, (int)h, caption);
	}
	else if(type=="label")
	{
		frame=new GUIlabel((int)x, (int)y, (int)w, (int)h, caption);
	}
	else if(type=="state")
	{
		vector<string> options; 
		map<string, string> vals=parser.GetAllValues(path + "\\states");
		map<string, string>::iterator i=vals.begin();
		map<string, string>::iterator e=vals.end();
		for(; i!=e; i++)
			options.push_back(i->second);

		frame = new GUIstateButton((int)x, (int)y, (int)w, options, makeFunctor((Functor2<GUIstateButton*, int>*)0, *this, &GUIdialogController::StateChanged));
	}
	else if(type=="input")
	{
		GUIinput *input=new GUIinput((int)x, (int)y, (int)w, (int)h,  makeFunctor((Functor1<const string&>*)0, *this, &GUIdialogController::ConsoleInput));
		input->SetCaption(caption);		
		frame=input;
	}
	else if(type=="resourcebar")
	{
		GUIpane *pane=new GUIresourceBar((int)x, (int)y, (int)w, (int)h);
		pane->SetDraggable(parser.SGetValueDef("no", path+"\\draggable")=="yes");
		pane->SetFrame(parser.SGetValueDef("yes", path+"\\frame")=="yes");
		pane->SetResizeable(parser.SGetValueDef("no", path+"\\resizeable")=="yes");
		frame=pane;
	}
	else if(type=="allyresourcebar")
	{
		GUIpane *pane=new GUIallyResourceBar((int)x, (int)y, (int)w, (int)h);
		pane->SetDraggable(parser.SGetValueDef("no", path+"\\draggable")=="yes");
		pane->SetFrame(parser.SGetValueDef("yes", path+"\\frame")=="yes");
		frame=pane;
	}
	else if(type=="infoselection")
	{
		GUIinfoSelection *infoSelection=new GUIinfoSelection((int)x, (int)y, (int)w, (int)h);
		infoSelection->SetDraggable(parser.SGetValueDef("no", path+"\\draggable")=="yes");
		infoSelection->SetFrame(parser.SGetValueDef("yes", path+"\\frame")=="yes");
		infoSelection->SetResizeable(parser.SGetValueDef("no", path+"\\resizeable")=="no");
		float num=atof(parser.SGetValueDef("64", path+"\\buildpicsize").c_str());
		ScaleX(num);
		infoSelection->SetBuildPicSize((int)num);
		frame=infoSelection;
	}

	if(!frame)
		frame=CreateControl(type, (int)x, (int)y, (int)w, (int)h, parser);

	if(!frame)
	{
		printf("unknown control definition %s\n", type.c_str());
		return;	
	}

	if(parser.SGetValueDef("no", path+"\\hidden")=="yes")
		frame->ToggleHidden();
		
	if(!tooltip.empty())
		frame->SetTooltip(tooltip);

	if(frame!=parent)
	{
		frame->SetIdentifier(identifier);

		// add to parent
		GUIswitchBar *bar=dynamic_cast<GUIswitchBar*>(parent);
		if(bar)
		{
			// if it's a switchbar, look if child has a title for switchbars
			string switchName=parser.SGetValueDef(caption, path+"\\switch");
			bar->AddSwitchableChild(frame, switchName);	
		}
		else
		{
			parent->AddChild(frame);
		}

		if(identifier!="")
			controls[identifier]=frame;
	}

	// add children
	vector<string> sects=parser.GetSectionList(path);

	for(int i=0; i<sects.size(); i++)
	{
		printf("found section %s\n", sects[i].c_str());
		if(!sects[i].compare(0, 6, "gadget"))
			CreateUIElement(parser, frame, path+"\\"+sects[i]);
		if(!sects[i].compare(0, 6, "dialog"))
		{
			string id=parser.SGetValueDef("", path+"\\"+sects[i]+"\\id");
			GUIdialogController *controller=dialogControllers[id];
			if(controller)
				controller->CreateUIElement(parser, frame, path+"\\"+sects[i]);
		}
	}
}

