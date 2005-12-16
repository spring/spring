// GUIgraph.h: interface for the GUIgraph class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(GUIGRAPH_H)
#define GUIGRAPH_H

#include "GUIframe.h"
#include <vector>

class GUIgraph : public GUIframe
{
public:
	GUIgraph(int x, int y, int w, int h);
	virtual ~GUIgraph();
	
	struct Dataset
	{
		Dataset(const string& c)
		{
			caption=c;
		}
		Dataset() {}
		float r, g, b;
		string caption;
		vector<float> points;	
	};

	void SetDatasets(const vector<Dataset>& dataset,bool showDif,int button,string name);
	
	// how long is one data point? (not implemented)
	void SetTimebase(const float timebase);
protected:
	void PrivateDraw();
	
	float maximum;
	float timebase;

	void BuildList();
	
	vector<Dataset> data[2];
	string graphNames[2];
	
	GLuint displayList;

	bool showDif;
};

#endif // !defined(GUIGRAPH_H)

