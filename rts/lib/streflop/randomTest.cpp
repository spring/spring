/*
    streflop: STandalone REproducible FLOating-Point
    Nicolas Brodu, 2006
    Code released according to the GNU Lesser General Public License

    Heavily relies on GNU Libm, itself depending on netlib fplibm, GNU MP, and IBM MP lib.
    Uses SoftFloat too.

    Please read the history and copyright information in the documentation provided with the source code
*/

#include <iostream>
#include <fstream>
#include <string>
using namespace std;
// clock
#include <time.h>

#include "streflop.h"
using namespace streflop;

template<typename F> void checkNRandom() {
    streflop_init<F>();
    F mean = 0.0;
    F var = 0.0;
    int N = 1000000;
    for (int i=0; i<N; ++i) {
        // mean, var
        F value = NRandom<F>() * 78.9 + 345.6;
        mean += value;
        var += value * value;
    }
    mean /= N;
    var = sqrt(var/N - mean*mean);
    cout << "meanN (should be 345.6): " << mean << endl;
    cout << "varN (should be 78.9): " << var << endl;
}

template<bool IEmin, bool IEmax, typename F> void checkRandom() {
    F mean = 0.0;
    F var = 0.0;
    int N = 1000000;
    for (int i=0; i<N; ++i) {
        // mean, var
        F value = Random<IEmin, IEmax, F>(100.0,700.0);
        mean += value;
        var += value * value;
    }
    mean /= N;
    var = sqrt(var/N - mean*mean);
    cout << "mean<"<<IEmin<<","<<IEmax<<"> (should be 400): " << mean << endl;
    cout << "var<"<<IEmin<<","<<IEmax<<"> = " << var << endl;
}

template<typename FloatType> void showrate( clock_t start, clock_t stop, int reps )
{
    FloatType time = FloatType( stop - start ) / CLOCKS_PER_SEC;
    FloatType rate = reps / time;
    cout << (double)rate << " million per second" << endl;
}

// inspired from Richard J. Wagner Mersenne class timing test
template<typename FloatType> void randomTimings() {
    streflop_init<FloatType>();
    cout << "Test of generation rates in various distributions:" <<endl;
    clock_t start, stop;

    cout << "  Integers in [0,2^32-1]         ";
    start = clock();
    for(int i = 0; i < 50000000; ++i ) Random<SizedUnsignedInteger<32>::Type>();
    stop = clock();
    showrate<FloatType>(start,stop,50);

    cout << "  Integers in [0,100]            ";
    start = clock();
    for(int i = 0; i < 50000000; ++i ) Random<true, true, SizedUnsignedInteger<32>::Type>(0,100);
    stop = clock();
    showrate<FloatType>(start,stop,50);

    cout << "  Reals in [1,2)                 ";
    start = clock();
    for(int i = 0; i < 50000000; ++i ) Random12<true, false, FloatType>();
    stop = clock();
    showrate<FloatType>(start,stop,50);

    cout << "  Reals in [0,1)                 ";
    start = clock();
    for(int i = 0; i < 50000000; ++i ) Random01<true, false, FloatType>();
    stop = clock();
    showrate<FloatType>(start,stop,50);

    cout << "  Reals in [0,7)                 ";
    start = clock();
    for(int i = 0; i < 50000000; ++i ) Random<true, false, FloatType>(0.0, 7.0);
    stop = clock();
    showrate<FloatType>(start,stop,50);

    cout << "  Reals in [1,2]                 ";
    start = clock();
    for(int i = 0; i < 50000000; ++i ) Random12<true, true, FloatType>();
    stop = clock();
    showrate<FloatType>(start,stop,50);

    cout << "  Reals in (1,2)                 ";
    start = clock();
    for(int i = 0; i < 50000000; ++i ) Random12<false, false, FloatType>();
    stop = clock();
    showrate<FloatType>(start,stop,50);

    cout << "  Reals in normal distribution   ";
    start = clock();
    FloatType secondary;
    for(int i = 0; i < 10000000; ++i ) NRandom<FloatType>(2.0, 7.0, &secondary);
    stop = clock();
    showrate<FloatType>(start,stop,20);
}


int main(int argc, const char** argv) {

    cout << "Random seed: " << RandomInit() << endl;

    cout << "Checking Simple ranges" << endl;
    checkNRandom<Simple>();
    checkRandom<true, true, Simple>();
    checkRandom<true, false, Simple>();
    checkRandom<false, true, Simple>();
    checkRandom<false, false, Simple>();
    cout << "Checking Double ranges" << endl;
    checkNRandom<Double>();
    checkRandom<true, true, Double>();
    checkRandom<true, false, Double>();
    checkRandom<false, true, Double>();
    checkRandom<false, false, Double>();
#if defined(Extended)
    cout << "Checking Extended ranges" << endl;
    checkNRandom<Extended>();
    checkRandom<true, true, Extended>();
    checkRandom<true, false, Extended>();
    checkRandom<false, true, Extended>();
    checkRandom<false, false, Extended>();
#endif

    cout << "Checking Simple timings" << endl;
    randomTimings<Simple>();
    cout << "Checking Double timings" << endl;
    randomTimings<Double>();
#if defined(Extended)
    cout << "Checking Extended timings" << endl;
    randomTimings<Extended>();
#endif

    return 0;
}

