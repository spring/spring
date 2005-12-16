#ifndef CTAUNT_H
#define CTAUNT_H

class ctaunt: public base{
public:
	ctaunt(){}
	virtual ~ctaunt(){}
	void InitAI(Global* GL){
		G = GL;
	}
	void GotChatMsg(const char* msg,int player){}
	bool taunt();
private:
	bool Taunt;
	Global* G;
};

#endif

