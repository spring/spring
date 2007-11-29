/*
AF 2007
*/

#include "../Core/include.h"

CMessage::CMessage(string my_type){
	message_type = my_type;
}

CMessage::CMessage(string my_type, vector<float> &myparameters){
	parameters = myparameters;
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
