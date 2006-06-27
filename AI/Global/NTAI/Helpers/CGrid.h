// A class to encapsulate the threat matrix so that it cna be more generalized in order to bring other aspects into the fold.

struct CGData{
};

struct CGridSquare{
	//
	float3 centre;
	float3 corner;
	float3 location;
	map<int,CGData> data; // data[allyteam] = relevant data
};

class CGrid{
public:
	CGrid(Global* GL){ G =GL;}
	virtual CGrid(){ G = 0;}
private:
	Global* G;
};