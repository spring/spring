#ifndef NTAI_CEVENT_H
#define NTAI_CEVENT_H

class EData;

class CEvent{
enum TEvent {E_NA};
public:
	CEvent(){
		type = E_NA;
	}
	TEvent GetType(){
		return type;
	}
	EData* GetData();
private:
	TEvent type;
};

#endif
