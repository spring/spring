#include <iostream>
#include <list>
#include <ctime>

#define NUM_ITERS 100
#define NUM_OBJECTS 100000

using namespace std;

// gcc 4.3.2 -O2, XP 32bit, Core2 E8400
// ned malloc: 2.11 s
// win32 default malloc: 3.171 s

int main()
{
	list<int> l;
	clock_t start, end;

	start = clock();
	for (int i = 0; i<NUM_ITERS; ++i) {
		for (int j = 0; j<NUM_OBJECTS; ++j) {
			l.push_back(j);
		}
		l.clear();
	}
	end = clock();

	cout << "time: " << ((end-start)/(float)CLOCKS_PER_SEC) << endl;
	return 0;
}
