/*
AF 2007
*/

namespace ntai {
	class IModule {
	public:
		IModule(){
			G = 0;
			valid = false;
		}
		IModule(Global* GL);
		virtual ~IModule();
		virtual void RecieveMessage(CMessage &message)=0;
		virtual bool Init()=0;//boost::shared_ptr<IModule> me
		void End(){}
		bool IsValid(){
			return valid;
		}
		bool SetValid(bool isvalid){
			valid = isvalid;
			return valid;
		}

		void DestroyModule();

		void operator()(){}
		Global* G;
	protected:
		bool valid;
	};
}
