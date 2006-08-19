STandalone REproducible FLOating-Point library
Version 0.2, june 2006.
Nicolas Brodu. See also the acknowledgments below.


For a quick setup guide, see the "usage" sections below.



Presentation:

Floating-point computations are strongly dependent on the FPU hardware implementation, the compiler and its optimizations, and the system mathematical library (libm). Experiments are usually reproducible only on the same machine with the same system library and the same compiler using the same options. Even then, the C++ standard does NOT guarantee reproducible results. Example:

    double x = 1.0;  x /= 10.0;
    double y = x;
    ...
    if (y == x) {
        // THIS IS NOT ALWAYS TRUE!
    }

A related but more general problem is random number generation, often necessary for experiment reproducibility:

    set_random_seed(42);
    double x = random_number();

May not return the same x across different FPUs / systems / compilers / etc.



These problems are related to:

- Some FPU (like x87 on Linux) keep by default an internal precision (80 bits) larger than the type size in memory. In the first example, if x is on the stack but y in a register, the comparison will fail. Worse, whether x and/or y is on the stack or register depends on what you put in the ... section. This problem can be solved by restricting the internal FPU precision to the type size in memory. Unfortunately, on x87, some operations like the transcendental functions are always computed on 80 bits...

- How well your FPU implements the IEEE754 standard, and in particular, denormal number operations. The provided arithmetic_test code reproducibly gives different results between SSE and x87. This problem is NOT generally solved by restricting the internal precision to the type size in memory. There may be relations however, in particular a denormal float may be a normal double, so the precision matters.

- How your compiler interacts with your system math library (libm) especially with optimization flags. In particular, gcc-3.4 and gcc-4.0 series may give different results on the same system with the same library. This problem is partially solved by changing the compiler options.

- Even then, the IEEE754 standard has some loopholes concerning NaN types. In particular, when serializing results, binary files may differ. This problem is solved by comparing numerical values and not bit patterns.



More points to consider:

- SSE has an option to accelerate computations by approximating denormals by 0. For some applications this is plain wrong, but for other applications like real-time DSP, denormals are a real pain and this is just what's needed (reports have been made that a denormal multiply may take as much as 30 times a normal multiply). Due to the default round-to-nearest mode, the denormals tend NOT to cancel to 0 and may instead accumulate. Deactivating denormals is built-in the core SSE FPU, but unfortunately that's not reproducible on x87. In that case, it's possible to check for denormal conditions after each operation (including assignment) and then flush to zero, thanks to a wrapper type, for reproducibility (but at the expense of performance on x87).

- No external dependency. The more dependencies, the more chances of a version mismatch. This library should be standalone, providing the whole libm features without resorting to system specific includes or other packages. This way, it can be included in a project as is, with minimal specialization (and risk of misconfiguration).



Proposed solution:

- Provide Simple, Double and Extended types that match the native types (float, double, long double), but that additionally take care of the FPU internal precision and denormal handling. These types may be simple aliases or C++ wrappers, depending on the FPU and configuration. Note: Extended is the 80-bit type defined by x87.

- Reimplement the libm as a standalone code that uses these types. Note: For extended support, see below.

- As a bonus, provide a random number generator so as to make standalone experiments reproducible.

- Compare with a software floating-point reference implementation.



Usage (programming):

- Include "streflop.h" in place of <math.h>, and link with streflop.a instead of libm. All streflop functions are protected by a namespace, so if another part of your program uses libm there is no risk of confusion at link time. However, including the correct file matters.

- Use the streflop namespace, and the Simple, Double and Extended types as needed, instead of float, double, long double. The streflop types may actually be aliases to the C++ types, or wrapper classes that redefine the operators transparently.

- You should also call streflop_init<FloatType> with FloatType=Simple,Double,Extended before using that type. You should use only this type (ex: Simple) until the next call to streflop_init. That is, separate your code in blocks using one type at a time. In the simplest case, use streflop_init for your chosen type at the beginning of your program and stick to that type later on. These init functions are necessary to set the correct FPU flags. See also the notes below.

- You may have a look at arithmeticTest.cpp and randomTest.cpp for examples.



Usage (standalone build):

- Edit Makefile.common to configure which FPU/denormal setup you choose, by defining one of STREFLOP_SSE, STREFLOP_X87, STREFLOP_SOFT, and optionally STREFLOP_NO_DENORMALS. See the configurations grid below.

- If you're using the software floating-point implementation on a big-endian machine, change the System.h file accordingly. If your target system size has a char type larger than 8 bits, then check Integer.h. In both cases you're on your own (this is untested).

- Check the notes below before changing the compiler options.



Usage (including in a project):

- Copy the whole streflop source somewhere in your project, for example as a "streflop" subdirectory.

- Check the steps for the standalone build. Potentially automate the choices using your build system, like autoconf.

- Call "make -C streflop" somewhere in you own build process, or integrate the streflop build in your build system.

- See the programming usage above. Don't forget to "-Istreflop" and "-Lstreflop -lstreflop.a" when compiling and linking.



Configurations grid:

            |   SSE   |   x87     |   Soft   |
------------+---------+-----------+----------+
denormals   | Simple *| Simple   *| Simple   |
            | Double *| Double   *| Double   |
            |         | Extended *| Extended |
------------+---------+-----------+----------+
no denormal | Simple *| Simple    |
            | Double *| Double    |
            |         | Extended  |
------------+---------+-----------+

One cell in this grid must be selected at configure time. All types within that cell are then available at compile and run time.
Types marked * are aliases to the native float/double/long double, with support by FPU flags.
The other types are wrapper classes that behave like the native types.


Apart for the bit representation of NaN values:

- "Denormals SSE / Denormals Soft" with the same precision should give the same results.

- "No denormal SSE / No denormal x87" with the same precision should give the same results.

- "Denormals x87 extended / Denormals Soft extended" should give the same results.

- "Denormals SSE / Denormals x87" with the same precision may differ but only for some unfrequent occurences involving denormal numbers.

- All other configurations give different results.



Comparison criteria:

- Best performance is achieved by "no denormal/SSE simple". What matters most for performance is wrapper/native, the size, then denormals or not (unless using lots of denormals, in which case "no denormals" may matter more than size or wrapper).

- Best precision is achieved by "denormals extended" modes. What matters most for precision is the the size, then denormal or not.

- Best IEEE754 conformance is achieved by "Soft" modes (and equivalent results above with SSE/x87). Conformance is achieved only for denormals.



Notes:

- Beware of too aggressive optimization options! In particular, since this code relies on reinterpret_cast and unions, the compiler must not assume strict aliasing. For g++ optimization levels 2 and 3, this assumption is unfortunately the default. Similarly, the compiler should not assume that NaN can be ignored, or that the FPU has a constant rounding mode. Ex: -O3 -fno-strict-aliasing -frounding-math -fsignaling-nans.

- You should also set correct FPU options, like -mfpmath=sse -msse -msse2. The -msse2 is important, there are cases where g++ refuses to use SSE (and silently falls back to x87) when using -msse and not -msse2. This also means you cannot reliably use this library with gcc on systems where only sse (but not sse2) is present, like some athlon-xp cores.

- The system libm will almost surely produce different numerical results depending on your FPU, compiler and options, etc. The rationale is, using this library will increase the reproducibility of your experiments compared to using the system libm. If you want guaranteed (but slower) reproducible results across all machine configurations, without caring for denormals or whatever else, then use a multiprecision software library like GNU MP. If you want to use the hardware FPU in a controlled environment that can retain some reproducibility, then use this library. If it does not fit your needs, then improve it: After all, this is free software :)

- The following C99 trap and rounding mode functions are implemented, even for the software floating-point implementation: fe(get|set)round, fe(get|set)env, and feholdexcept. You may call them to change rounding modes and to trap special conditions. These functions are expected to work correctly, insofar as the FPU works as intended*, but they have not been extensively tested.
* in particular, reports have been made that the x87 FPU denormal trap sometimes fails.

- Really beware of aggressive optimization! Separate your code into INDEPENDENT BLOCKS. I mean it. This code is wrong:
    streflop_init<Simple>();
    Simple s = (1.0/4294967295.0);
    displayHex(cout, s) << endl;
    streflop_init<Double>();
    Double d = (1.0/4294967295.0);
    displayHex(cout, d) << endl;
THIS NOT WORK CORRECTLY in -O3, but will do fine in -O0. This is because the compiler "optimizes" the constant computation only once for both lines in -O3, which is plain wrong since the precision is different. The only way to ensure this does not happen is to separate your code in logical units:
void func1() {
    streflop_init<Simple>();
    Simple s = (1.0/4294967295.0);
    displayHex(cout, s) << endl;
}
void func2() {
    streflop_init<Double>();
    Double d = (1.0/4294967295.0);
    displayHex(cout, d) << endl;
}
Even then, you'd better be careful with interprocedural optimization options. If possible, put func1 and func2 in 2 separate compilation units.



BUGS and discrepancies:

- Do what you want with this library, but at your own risks, and according to the LGPL (see the LGPL.txt file).

- There is the possibility of unknown bugs. And this is based on GNU libm 2.4, so any potential bug in that version are almost surely present in streflop too.

- Extended support is INCOMPLETE. Proper functions are missing, in particular the trigonometric functions. The ldbl-96 implementation of the libm does not contain a generic implementation for these files. Since strelop enforces strict separation of Extended and Double functions, these functions were instead implemented by temporarily switching to Double using streflop_init<Double>, calling the function and storing the result on the stack, switching back to streflop_init<Extended>, then converting the result to an Extended number.



Acknowledgments:

- This code heavily relies on GNU Libm, itself depending on Sun's netlib fplibm, GNU MP, and IBM's multi-precision library.

- This code uses the SoftFloat library for the software floating-point implementation.

- The random number generator is the Mersene Twister, created by Takuji Nishimura and Makoto Matsumoto, and adapted to C++ for this project. Please read the (BSD-like) license in Random.cpp if you intend to make binary packages of this library (and according to LGPL).

- Please read the history and copyright information in the accompanying README.txt files in the libm and softfloat directories, as well as the LGPL.txt file in this directory.

- Thanks to Tobi Vollebregt for feedback, Win32 reports, and patches.



How you can help:

- Test the library and report potential bugs.

- Port the library to new FPU and operating systems.

- Help extend the GNU libm first, and only then import that work in this project.



Nicolas Brodu, june 2006.
