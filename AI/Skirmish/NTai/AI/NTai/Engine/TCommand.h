// Class TCommand
// structure to hold data on cached commands

template<class type> inline std::string to_string( const type & value) {
	std::ostringstream streamOut;
	streamOut << value;
	return streamOut.str();
}

class TCommand{ 
public:

	TCommand(std::string s){
			  clear = false;
			  type = B_CMD;
			  delay = 0;
			  unit = 0;
			  created = 0;
			  Priority = tc_na;
			  c.id = CMD_STOP;
			  c.params.clear();
#ifdef TC_SOURCE
			source = s;
#endif
		  }
		TCommand(int unit, string s){
			created = 0;
			clear = false;
			Priority = tc_na;
			this->unit = unit;
			delay = 0;
			type = B_CMD;
			c.id = CMD_STOP;
			c.params.clear();
#ifdef TC_SOURCE
			source = s;
#endif
		}

		TCommand(){
			clear = false;
			type = B_CMD;
			delay = 0;
			unit = 0;
			created = 0;
			Priority = tc_na;
			c.id = CMD_STOP;
			c.params.clear();
		}

		TCommand(int unit){
			this->unit = unit;
			Priority = tc_na;
			created = 0;
			c.id = CMD_STOP;
			c.params.clear();
			type = B_CMD;
			clear = false;
			delay = 0;
		}
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
		  void PushUnit(int unit){
			  if((-1<unit)&&(unit<MAX_UNITS-1)){
				c.params.push_back((float)unit);
			  }
		  }
		  void ID(int cmd){
			  if(cmd == CMD_IDLE){
				  type = B_IDLE;
			  }else{
				c.id=cmd;
				type = B_CMD;
			  }
		  }
		  std::string toString(){
			  string s = "Command: ID: ";
			  s+= to_string(c.id);
			  s+=" Timeout: ";
			  s+= to_string(c.timeOut);
			  //s+= " options : ";
			  //s+= c.options;
			  if(c.params.empty()){
				  s+= " params: none";
			  }else{
				  s+= " params: ";
				  for(vector<float>::iterator i = c.params.begin(); i!= c.params.end(); ++i){
					  s+= to_string(*i);
					  s+= ", ";
				  }
			  }
#ifdef TC_SOURCE
			  s+= "source of command: ";
			  s+= source;
#endif
			  return s;
		  }
		  void SetSource(string s){
#ifdef TC_SOURCE
			  source = s;
#endif
		  }
//
		  t_priority Priority; // The priority of this command
		  int created; // frame number this command was created on
		  int unit; // the unit this command is to be given to
		  int delay; // the frame at which this becomes a valid command
		  btype type;
		  bool clear; // if this is true then the command is invalid or already executed and should be deleted
		  
		  Command c; // The command itself

#ifdef TC_SOURCE
		  std::string source; // a string telling where this command came from (for debugging purposes only)
#endif
};
