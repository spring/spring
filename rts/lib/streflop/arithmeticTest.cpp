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

template<class FloatType> inline void writeFloat(ofstream& of, FloatType f) {
    int nbytes = sizeof(f);
    char* thefloat = reinterpret_cast<char*>(&f);
    long check = 1;
    // big endian OK, reverse little endian
    if (*reinterpret_cast<char*>(&check) == 1) {
        char buffer[nbytes];
        for (int i=0; i<nbytes; ++i) buffer[i] = thefloat[nbytes-1-i];
        of.write(buffer,nbytes);
    } else {
        of.write(thefloat,nbytes);
    }
};

#ifdef Extended
// special case for extended, size-dependent
template<> inline void writeFloat<Extended>(ofstream& of, Extended f) {
    int nbytes = 10;
    char* thefloat = reinterpret_cast<char*>(&f);
    long check = 1;
    // big endian OK, reverse little endian
    if (*reinterpret_cast<char*>(&check) == 1) {
        char buffer[nbytes];
        for (int i=0; i<nbytes; ++i) buffer[i] = thefloat[9-i];
        of.write(buffer,nbytes);
    } else {
        of.write(thefloat,nbytes);
    }
};
#endif

template<class FloatType> void doTest(string s, string name) {

    streflop_init<FloatType>();

    ofstream basicfile((s + "_" + name + "_basic.bin").c_str());
    if (!basicfile) {
        cout << "Problem creating binary file: " << basicfile << endl;
        exit(2);
    }

    ofstream infnanfile((s + "_" + name + "_nan.bin").c_str());
    if (!infnanfile) {
        cout << "Problem creating binary file: " << infnanfile << endl;
        exit(3);
    }

    ofstream mathlibfile((s + "_" + name + "_lib.bin").c_str());
    if (!mathlibfile) {
        cout << "Problem creating binary file: " << mathlibfile << endl;
        exit(4);
    }

    FloatType f = 42;

    // Trap NaNs
    feraiseexcept(FE_INVALID);

    // Generate some random numbers and do some post-processing
    // No math function is called before this loop
    for (int i=0; i<10000; ++i) {
        f = RandomIE(f, FloatType(i));
        for (int j=0; j<100; ++j) f += FloatType(0.3) / f + RandomIE<FloatType>(1.0,2.0);
        writeFloat(basicfile, f);
    }
    basicfile.close();

    // Explicit checks for Inf, Nan, Denormals
    // Minimal number somewhere around 10-4932 for long double
    // Check for denormals at the same time
    f = FloatType(0.1); // 0.1 is not perfectly representable in binary
    for (int i=0; i<5000; ++i) {
        f *= FloatType(0.1);
        writeFloat(infnanfile, f);
    }

    // check for round-up to +Inf
    f = FloatType(10.0001); // not representable exactly
    for (int i=0; i<5000; ++i) {
        f *= FloatType(10.0001);
        writeFloat(infnanfile, f);
    }

    // Explicit +inf check
    f = FloatType(+0.0);
    f = FloatType(1.0)/f;
    writeFloat(infnanfile, f);

    // Explicit -inf check
    f = FloatType(-0.0);
    f = FloatType(1.0)/f;
    writeFloat(infnanfile, f);

    // A few NaN checks
    feclearexcept(FE_INVALID);
    f *= FloatType(0.0); // inf * 0
    writeFloat(infnanfile, f);
    f = FloatType(+0.0);
    f = FloatType(1.0)/f;
    FloatType g = FloatType(-0.0);
    g = FloatType(1.0)/g;
    f += g; // inf - inf
    writeFloat(infnanfile, f);
    f = FloatType(+0.0);
    g = FloatType(+0.0);
    f /= g; // 0/0
    writeFloat(infnanfile, f);

    infnanfile.close();

    // Trap NaNs again
    feraiseexcept(FE_INVALID);

    // Call the Math functions
    for (int i=0; i<10000; ++i) {
        f = tanh(cbrt(fabs(log2(sin(FloatType(RandomII(0,i)))+FloatType(2.0)))+FloatType(1.0)));
        writeFloat(mathlibfile, f);
    }

    mathlibfile.close();

}

template<class FloatType> ostream& displayHex(ostream& out, FloatType f) {
    int nbytes = sizeof(f);
    char* thefloat = reinterpret_cast<char*>(&f);
    long check = 1;
    // big endian OK, reverse little endian
    if (*reinterpret_cast<char*>(&check) == 1) {
        char buffer[nbytes];
        for (int i=0; i<nbytes; ++i) buffer[i] = thefloat[nbytes-1-i];
        for (int i=0; i<nbytes-1; ++i) out << hex << ((int)buffer[i] & 0xFF) << dec << " ";
        out << hex << ((int)buffer[nbytes-1] & 0xFF) << dec << " ";
    } else {
        for (int i=0; i<nbytes-1; ++i) out << hex << ((int)thefloat[i] & 0xFF) << dec << " ";
        out << hex << ((int)thefloat[nbytes-1] & 0xFF) << dec << " ";
    }
    return out;
}


int main(int argc, const char** argv) {

    RandomInit(42);

    if (argc<2) {
        cout << "You should provide a base file name for the arithmetic test binary results. This base name will be appended the suffix _basic for basic operations not using the math library, _nan for denormals and NaN operations, and _lib for results calling the math library functions (sqrt, sin, etc.). The extension .bin is then finally appended to the file name." << endl;
        cout << "Example: " << argv[0] << " x87_gcc4.1_linux will produce 3 files: x87_gcc4.1_linux_basic.bin, x87_gcc4.1_linux_nan.bin and x87_gcc4.1_linux_lib.bin" << endl;
        return 1;
    }

    doTest<Simple>(argv[1], "simple");
    doTest<Double>(argv[1], "double");
#if defined(Extended)
    doTest<Extended>(argv[1], "extended");
#endif

    return 0;
}

