// Class TCommand
// structure to hold data on cached commands

class TCommand{ 
public:
#ifdef TC_SOURCE
	TCommand(string s):
	  Priority(tc_na),
		  created(0),
		  unit(0),
		  delay(0),
		  source(s),
		  type(B_CMD),
		  clear(false){
			  c.id = CMD_STOP;
			  c.params.clear();
		  }
#endif
#ifndef TC_SOURCE
		  TCommand():
		  Priority(tc_na),
			  created(0),
			  unit(0),
			  delay(0),
			  type(B_CMD),
			  clear(false){
				  c.id = CMD_STOP;
				  c.params.clear();
			  }
#endif
			  virtual ~TCommand(){
				  if(type == B_CMD){
					  if(c.params.empty() == false){
						  c.params.erase(c.params.begin(),c.params.end());
						  c.params.clear();
					  }
				  }
			  }
			  bool operator>(TCommand& tc){
				  return tc.Priority > Priority;
			  }
			  bool operator<(TCommand& tc){
				  return tc.Priority < Priority;
			  }
			  void PushFloat3(float3 p){
				  c.params.push_back(p.x);
				  c.params.push_back(p.y);
				  c.params.push_back(p.z);
			  }
			  void Push(float f){
				  c.params.push_back(f);
			  }
			  void Push(int i){
				  c.params.push_back((float)i);
			  }
			  void ID(int cmd){
				  if(cmd == CMD_IDLE){
					  type = B_IDLE;
				  }else{
					c.id=cmd;
					type = B_CMD;
				  }
			  }
			  int unit; // the unit this command is to be given to
			  int created; // frame number this command was created on
			  t_priority Priority; // The priority of this command
			  Command c; // The command itself
			  bool clear; // if this is true then the command is invalid or already executed and should be deleted
			  int delay; // the frame at which this becomes a valid command
			  btype type;
#ifdef TC_SOURCE
			  string source; // a string telling where this command came from (for debugging purposes only)
#endif
};
