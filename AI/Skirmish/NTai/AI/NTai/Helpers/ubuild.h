// Universal build.h

namespace ntai {

	class Tunit;

	class CUBuild{
	public:
		CUBuild();
		virtual ~CUBuild();

		bool OkBuildSelection(string name);

		/* Initialize object */
		void Init(Global* GL, CUnitTypeData* wu, int uid);

		void SetWater(bool w);

		/* () operator, allows it to be called like this, Object(build); rather than something like object->getbuild(build); */
		string operator() (btype build,float3 pos=ZeroVector);
		
		/* metal extractor*/
		string GetMEX(); 

		/* energy producer */
		string GetPOWER();

		/* random attack unit */
		string GetRAND_ASSAULT();
		
		/* most efficient attack unit (give or take a percentage) */
		string GetASSAULT();
		
		/* factory */
		string GetFACTORY();
		

		/* builders */
		string GetBUILDER();
		
		/* geothermal plant */
		string GetGEO();
		
		/* scouter */
		string GetSCOUT();
		
		/* random unit (no mines) */
		string GetRANDOM();
		
		/* random defense */
		string GetDEFENCE();
		
		/* radar towers */
		string GetRADAR();
		
		/* energy storage */
		string GetESTORE();
		
		/* metal storage */
		string GetMSTORE();
		
		/* missile silos */
		string GetSILO();
		
		/* radar jammers */
		string GetJAMMER();
		
		/* sonar */
		string GetSONAR();
		
		/* antimissile units */
		string GetANTIMISSILE();
		
		/* artillery units */
		string GetARTILLERY();
		
		/* focal mines */
		string GetFOCAL_MINE();
		
		/* submarines */
		string GetSUB();
		
		/* amphibious units (pelican/gimp/triton etc) */
		string GetAMPHIB();
		
		/* mines */
		string GetMINE();

		/* units with air repair pads that floats */
		string GetCARRIER();
		
		/* metal makers */
		string GetMETAL_MAKER();
		
		/* walls etc */
		string GetFORTIFICATION();
		
		/* Targeting facility type buildings */
		string GetTARG();
		
		/* Bomber planes */
		string GetBOMBER();
		
		/* units with shields */
		string GetSHIELD();
		
		/* units that launch missiles, such as the Merl or diplomat or dominator */
		string GetMISSILE_UNIT();
		
		/*  */
		string GetFIGHTER();
		
		/*  */
		string GetGUNSHIP();
		
		/*  */
		string GetHUB();
		
		/*  */
		string GetAIRSUPPORT();

		/* To filter out things like dragons eye in AA, things that serve no purpose other than to simply exist */
		bool Useless(CUnitTypeData* u);

	protected:
		
		/*  */
		bool antistall;

		/*  */
		Global* G;

		/*  */
		int uid;

		/*  */
		CUnitTypeData* utd;
		
		/*  */
		bool water;
	};
}
