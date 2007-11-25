// cached data
// data thats held by the Global class is stored in one of these objects to make things look prettier....
// Also should load/save be implemented it makes it easier to backup data as I can just write this object to disk then bring it back on load up

//class ctri;
class TCommand;

class CCached {
public:
	//
	const unsigned short* losmap;
	set<int> enemies; // enemies in LOS
	//vector<ctri> triangles; // contains all the triangle markers being displayed on map

	bool cheating;

	int allyteam; // the number of this unit ally team
	int team; // the number of this specific team
	unsigned int comID; // the ID of the commander

	int randadd; // if 2 processes attempt to get a random variable in the same frame they'll get the same number, so everytime a random
	//var is needed I increment this number and add it so that the results are always different.

	int* encache;// cached enemy positions to speed up the process of calling the callback interface for the same data so many times
	unsigned int enemy_number; // the number of cached enemy positions
	int lastcacheupdate; // when the cached enemy positions where last updated
	//set<int> cloaked_units;
};
