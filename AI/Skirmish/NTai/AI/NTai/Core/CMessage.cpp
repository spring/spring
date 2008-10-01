/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/

#include "../Core/include.h"

CMessage::CMessage(string my_type){
	message_type = my_type;
	frame = 0;
	lifetime = -1;
}

CMessage::CMessage(string my_type, vector<float> &myparameters){
	parameters = myparameters;
	frame = 0;
	lifetime = -1;
}

vector<float> CMessage::GetParameters(){
	return parameters;
}

float CMessage::GetParameter(int i){
	if (i <0){
		return -1.0f;
	}
	if(parameters.size() > (unsigned)i){
		return parameters.at(i);
	}
	return -1.0f;
}

void* CMessage::GetOtherParameters(){
	return otherstuff;
}

int CMessage::GetFrame(){
	return frame;
}

void CMessage::SetFrame(int frame){
	this->frame = frame;
}

string CMessage::GetType(){
	return message_type;
}

void CMessage::SetType(string type){
	message_type = type;
}

float3 CMessage::GetFloat3(){
	if(parameters.size() >= 3){
		return float3(parameters[0],parameters[1],parameters[2]);
	}else{
		return UpVector;
	}
}

float CMessage::GetRadius(){
	if(parameters.size() >= 3){
		return parameters[3];
	}else{
		return -1;
	}
}

void CMessage::AddParameter(int p){
	parameters.push_back((float)p);
}

void CMessage::AddParameter(float p){
	parameters.push_back(p);
}

void CMessage::AddParameter(float3 p){
	parameters.push_back(p.x);
	parameters.push_back(p.y);
	parameters.push_back(p.z);
}

void CMessage::SetLifeTime(int lifetime){
	//
	this->lifetime = lifetime;
}

bool CMessage::IsDead(int currentFrame){
	//
	if(lifetime == -1){
		return false;
	}

	return ((currentFrame - lifetime) > frame);
}

int CMessage::GetLifeTime(){
	//
	return lifetime;
}

int CMessage::GetRemainingLifeTime(int currentFrame){
	//
	return (frame + lifetime) - currentFrame;
}

