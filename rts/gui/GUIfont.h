#ifndef GUIFONT_H

#define GUIFONT_H



#include <string>



class GUIfont

{

public:

	GUIfont(const std::string&,int fontsize);



	void Print(float x, float y, const char* format,...);
	void Print(float x, float y, const std::string& text);

	void Print(float x, float y, float size, const std::string& text);

	void PrintColor(float x, float y, const std::string& text);



	void Print(const std::string& text);

	void PrintColor(const std::string& text);



	void output(float x, float y, const char* text) { Print(x, y, text); }



	float GetWidth(const std::string& text) const;

	float GetHeight() const;



private:

	float charWidth[256];

	float charHeight;

	unsigned int texture;

	unsigned int displaylist;

};



extern GUIfont* guifont;



#endif	// GUIFONT_H

