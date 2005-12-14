#include "../helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Register::UnitCreated(int unit){
	units.insert(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Register::UnitDestroyed(int unit){
	units.erase(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Draw things on the map
// (to be carted off to the MapGUI class when more work on it has
// been done.)

void Register::Update(){
	if(units.empty() == false){
		for(set<int>::iterator i = units.begin();i != units.end();++i){
			float3 pos = G->cb->GetUnitPos(*i);
			pos.y += 20;
			pos.z -= 20;
			pos.x += 20;
			float3 pos2 = pos;
			pos2.x += 5;
			pos2.z -= 5;
			const deque<Command>* dc = G->cb->GetCurrentUnitCommands(*i);
			const UnitDef* ud = G->cb->GetUnitDef(*i);
			if(dc->empty() == true){
				int d = 0;
				if(ud == 0){
					d = G->cb->CreateLineFigure(pos,pos2,10,0,10,0);
				}else{
					d = G->cb->CreateLineFigure(pos,pos2,max(ud->xsize,ud->ysize),0,10,0);
					pos2.x += max(ud->xsize,ud->ysize);
					pos2.z -= max(ud->xsize,ud->ysize);
				}
				G->cb->SetFigureColor(d,255,255,255,0.3f);
				continue;
			}else{
				int d = 0;
				if(ud == 0){
					d = G->cb->CreateLineFigure(pos,pos2,10,0,20,0);
				}else{
					d = G->cb->CreateLineFigure(pos,pos2,max(ud->xsize,ud->ysize),0,20,0);
					pos2.x += max(ud->xsize,ud->ysize);
					pos2.z -= max(ud->xsize,ud->ysize);
				}
				bool s = false;
				for(deque<Command>::const_iterator c = dc->begin();c != dc->end();++c){
                    if(c->id == CMD_ATTACK){
						G->cb->SetFigureColor(d,255,0,0,0.3f);
						s=true;
						break;
					}
					if(c->id == CMD_WAIT){
						G->cb->SetFigureColor(d,255,255,255,1);
						s=true;
						break;
					}
					if(c->id == CMD_PATROL){
						G->cb->SetFigureColor(d,0,255,255,0.3f);
						s=true;
						break;
					}
					if(c->id == CMD_MOVE){
						G->cb->SetFigureColor(d,0,255,0,0.3f);
						s=true;
						break;
					}
					if(c->id == CMD_STOP){
						G->cb->SetFigureColor(d,200,255,200,0.3f);
						s=true;
						break;
					}
					if(c->id == CMD_RESURRECT){
						G->cb->SetFigureColor(d,150,150,150,0.3f);
						s=true;
						break;
					}
					if(c->id == CMD_REPAIR){
						G->cb->SetFigureColor(d,255,255,0,0.3f);
						s=true;
						break;
					}
					if(c->id == CMD_RECLAIM){
						G->cb->SetFigureColor(d,255,0,255,0.3f);
						s=true;
						break;
					}
					if(c->id == CMD_GUARD){
						G->cb->SetFigureColor(d,0,0,255,0.3f);
						s=true;
						break;
					}
					if(c->id<= -1){
						G->cb->SetFigureColor(d,255,255,0,0.3f);
						s=true;
						break;
					}
				}
				if(s==false){
					G->cb->SetFigureColor(d,200,200,200,0.3f);
				}
			}
		}
	}

	// Draw the mouse on the map

	float3 pos = G->cb->GetMousePos();
	pos.y = G->cb->GetElevation(pos.x,pos.z)+20;

	// Check if we're drawing lines or drawing a 3D model

	if(lines == true){
		float3 pos3 = pos;
		pos3.z += 15;
		pos3.x += 15;
		float3 pos2 = pos;
		pos2.x += 15;
		int d1 = G->cb->CreateLineFigure(pos,pos2,5,0,1,0);//top line
		int d2 = G->cb->CreateLineFigure(pos2,pos3,5,0,1,0);//right line
		pos2 = pos;
		pos2.z += 25;
		int d3 = G->cb->CreateLineFigure(pos,pos2,5,0,1,0);//left line
		int d4 = G->cb->CreateLineFigure(pos2,pos3,5,0,1,0);//top line
		float3 pos4 = pos;
		pos4.x += 7;
		pos4.z += 7;
		float3 pos5 = pos4;
		pos5.x += 3;
		pos5.z += 5;
		int d = G->cb->CreateLineFigure(pos5,pos4,20,1,1,0);

		// Set colours
		switch(G->cb->GetMyTeam()){
			case 0:
				G->cb->SetFigureColor(d1, 0.0f, 0.0f, 1.0f, 0.3f);
				G->cb->SetFigureColor(d2, 0.0f, 0.0f, 1.0f, 0.3f);
				G->cb->SetFigureColor(d3, 0.0f, 0.0f, 1.0f, 0.3f);
				G->cb->SetFigureColor(d4, 0.0f, 0.0f, 1.0f, 0.3f);
				break;
			case 1:
				G->cb->SetFigureColor(d1, 0.0f, 1.0f, 0.0f, 0.3f);
				G->cb->SetFigureColor(d2, 0.0f, 1.0f, 0.0f, 0.3f);
				G->cb->SetFigureColor(d3, 0.0f, 1.0f, 0.0f, 0.3f);
				G->cb->SetFigureColor(d4, 0.0f, 1.0f, 0.0f, 0.3f);
				break;
			case 2:
				G->cb->SetFigureColor(d1, 1.0f, 1.0f, 1.0f, 0.3f);
				G->cb->SetFigureColor(d2, 1.0f, 1.0f, 1.0f, 0.3f);
				G->cb->SetFigureColor(d3, 1.0f, 1.0f, 1.0f, 0.3f);
				G->cb->SetFigureColor(d4, 1.0f, 1.0f, 1.0f, 0.3f);
				break;
			case 3:
				G->cb->SetFigureColor(d1, 0.0f, 1.0f, 0.0f, 0.3f);
				G->cb->SetFigureColor(d2, 0.0f, 1.0f, 0.0f, 0.3f);
				G->cb->SetFigureColor(d3, 0.0f, 1.0f, 0.0f, 0.3f);
				G->cb->SetFigureColor(d4, 0.0f, 1.0f, 0.0f, 0.3f);
				break;
			case 4:
				G->cb->SetFigureColor(d1, 0.0f, 0.0f, 0.75f, 0.3f);
				G->cb->SetFigureColor(d2, 0.0f, 0.0f, 0.75f, 0.3f);
				G->cb->SetFigureColor(d3, 0.0f, 0.0f, 0.75f, 0.3f);
				G->cb->SetFigureColor(d4, 0.0f, 0.0f, 0.75f, 0.3f);
				break;
			case 5:
				G->cb->SetFigureColor(d4, 1.0f, 0.0f, 1.0f, 0.3f);
				G->cb->SetFigureColor(d3, 1.0f, 0.0f, 1.0f, 0.3f);
				G->cb->SetFigureColor(d2, 1.0f, 0.0f, 1.0f, 0.3f);
				G->cb->SetFigureColor(d1, 1.0f, 0.0f, 1.0f, 0.3f);
				break;
			case 6:
				G->cb->SetFigureColor(d4, 1.0f, 1.0f, 0.0f, 0.3f);
				G->cb->SetFigureColor(d3, 1.0f, 1.0f, 0.0f, 0.3f);
				G->cb->SetFigureColor(d2, 1.0f, 1.0f, 0.0f, 0.3f);
				G->cb->SetFigureColor(d1, 1.0f, 1.0f, 0.0f, 0.3f);
				break;
			case 7:
				G->cb->SetFigureColor(d1, 0.0f, 0.0f, 0.0f, 0.3f);
				G->cb->SetFigureColor(d2, 0.0f, 0.0f, 0.0f, 0.3f);
				G->cb->SetFigureColor(d3, 0.0f, 0.0f, 0.0f, 0.3f);
				G->cb->SetFigureColor(d4, 0.0f, 0.0f, 0.0f, 0.3f);
				break;
			case 8:
				G->cb->SetFigureColor(d1, 0.0f, 1.0f, 1.0f, 0.3f);
				G->cb->SetFigureColor(d2, 0.0f, 1.0f, 1.0f, 0.3f);
				G->cb->SetFigureColor(d3, 0.0f, 1.0f, 1.0f, 0.3f);
				G->cb->SetFigureColor(d4, 0.0f, 1.0f, 1.0f, 0.3f);
				break;
			case 9:
				G->cb->SetFigureColor(d1, 0.625f, 0.625f, 0.0f, 0.3f);
				G->cb->SetFigureColor(d2, 0.625f, 0.625f, 0.0f, 0.3f);
				G->cb->SetFigureColor(d3, 0.625f, 0.625f, 0.0f, 0.3f);
				G->cb->SetFigureColor(d4, 0.625f, 0.625f, 0.0f, 0.3f);
				break;
		}
	} else{
		G->cb->DrawUnit("CORCOM",pos,1,1,G->team,true,false);
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

