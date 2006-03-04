#include "../helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Planning::InitAI(Global* GLI){
	G = GLI;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

bool Planning::feasable(string s, int builder){
	/*float MetalPercentage = G->cb->GetMetal() / G->cb->GetMetalStorage();
	float EnergyPercentage = G->cb->GetEnergy() / G->cb->GetEnergyStorage();
	int gframe = G->cb->GetCurrentFrame();
	if(gframe>9000){
		if(MetalPercentage > 0.1f) return false;
		if(EnergyPercentage > 0.2f) return false;
	}*/
	/*const UnitDef* ud = G->cb->GetUnitDef(s.c_str());
	if(ud != 0){
		if(G->cb->GetMetalStorage() + ((G->cb->GetEnergyIncome()+G->cb->GetMetalIncome())*ud->buildTime) > ud->metalCost){
			return true;
		}
	}*/
	return false;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

/*int Planning::DrawLine(ARGB colours, float3 A, float3 B, int LifeTime, float Width, bool Arrow){
	// Get New Group
	/*int DrawGroup = G->cb->CreateLineFigure(float3(0,0,0), float3(1,1,1), 1.0f, 0, LifeTime, 0);
	G->cb->SetFigureColor(DrawGroup, colours.red, colours.green, colours.blue,colours.alpha);
	G->cb->CreateLineFigure(A, B, Width, Arrow ? 1 : 0, LifeTime, DrawGroup);
	return DrawGroup;
	return 0;
}*/

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

/*int Planning::DrawLine(float3 A, float3 B, int LifeTime, float Width, bool Arrow){
	// Get New Group
	/*int DrawGroup = G->cb->CreateLineFigure(float3(0,0,0), float3(1,1,1), 1.0f, 0, LifeTime, 0);
	switch(G->cb->GetMyAllyTeam()){
		case 0:
            G->cb->SetFigureColor(DrawGroup, 0.0f, 0.0f, 1.0f, 1.0f);
            break;
        case 1:
            G->cb->SetFigureColor(DrawGroup, 0.0f, 1.0f, 0.0f, 1.0f);
            break;
        case 2:
            G->cb->SetFigureColor(DrawGroup, 1.0f, 1.0f, 1.0f, 1.0f);
            break;
        case 3:
            G->cb->SetFigureColor(DrawGroup, 0.0f, 1.0f, 0.0f, 1.0f);
            break;
        case 4:
			G->cb->SetFigureColor(DrawGroup, 0.0f, 0.0f, 0.75f, 1.0f);
            break;
        case 5:
            G->cb->SetFigureColor(DrawGroup, 1.0f, 0.0f, 1.0f, 1.0f);
            break;
        case 6:
            G->cb->SetFigureColor(DrawGroup, 1.0f, 1.0f, 0.0f, 1.0f);
            break;
        case 7:
			G->cb->SetFigureColor(DrawGroup, 0.0f, 0.0f, 0.0f, 1.0f);
            break;
        case 8:
            G->cb->SetFigureColor(DrawGroup, 0.0f, 1.0f, 1.0f, 1.0f);
            break;
        case 9:
            G->cb->SetFigureColor(DrawGroup, 0.625f, 0.625f, 0.0f, 1.0f);
            break;
	}
    G->cb->CreateLineFigure(A, B, Width, Arrow ? 1 : 0, LifeTime, DrawGroup);
	return DrawGroup;
	return 0;
}*/

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

