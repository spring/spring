libcpuid
========

libcpuid provides CPU identification for the x86 (and x86_64).
For details about the programming API, you might want to
take a look at the project's website on sourceforge
(http://libcpuid.sourceforge.net/). There you'd find a short
[tutorial](http://libcpuid.sf.net/documentation.html), as well
as the full [API reference](http://libcpuid.sf.net/doxy).

Configuring after checkout
--------------------------

Under Linux, where you download the sources, there's no
configure script to run. This is because it isn't a good practice to keep
such scripts in a source control system. To create it, you need to run the
following commands once, after you checkout the libcpuid sources
from github:

        1. run "libtoolize"
        2. run "autoreconf --install"

You need to have `autoconf`, `automake` and `libtool` installed.

After that you can run `./configure` and `make` - this will build
the library.

`make dist` will create a tarball (with "configure" inside) with the
sources.

Prerequisites
-------------

Using libcpuid requires no dependencies on any of the supported OSes.
Building it requires the aforementioned libtool and autotools commands
to be available, which is a matter of installing a few common packages
with related names (e.g. automake, autoconf, libtool).
It also requires a POSIX-compatible shell. On NetBSD, you may need
to install one (credits to @brucelilly):

1. Install a POSIX-compatible shell such as ksh93
   (pkg_add ast-ksh || pkgin in ast-ksh)
2. export CONFIG_SHELL=/usr/pkg/bin/ksh93 (substitute the
   correct path if required)
3. Follow the regular Linux instructions

Testing
-------

After any change to the detection routines or match tables, it's always
a good idea to run `make test`. If some test fails, and you're confident
that the test is wrong and needs fixing, run `make fix-tests`.

You can also add a new test (which is basically a file containing
the raw CPUID data and the expected decoded items) by using
`tests/create_test.py`. The workflow there is as follows:

1. Run "cpuid_tool" with no arguments. It will tell you that it
   has written a pair of files, raw.txt and report.txt. Ensure
   that report.txt contains meaningful data.
2. Run "tests/create_test.py raw.txt report.txt > «my-cpu».test"
3. Use a proper descriptive name for the test (look into tests/amd
   and tests/intel to get an idea) and copy your test file to an
   appropriate place within the tests directory hierarchy.

For non-developers, who still want to contribute tests for the project,
use [this page](http://libcpuid.sourceforge.net/bugreport.php) to report
misdetections or new CPUs that libcpuid doesn't handle well yet.

Users
-----

So far, I'm aware of the following projects which utilize libcpuid (listed alphabetically):

* CPU-X (https://github.com/X0rg/CPU-X)
* fre:ac (https://www.freac.org/)
* I-Nex (https://github.com/i-nex/I-Nex)
* Multiprecision Computing Toolbox for MATLAB (https://www.advanpix.com/)
* ucbench (http://anrieff.net/ucbench)

We'd love to hear from you if you are also using libcpuid and want your project listed above.

Downloads
---------

You can find latest versioned archives [here](https://github.com/anrieff/libcpuid/releases/latest), with binaries for macOS and Windows.

#### Binary packages

Also, libcpuid is available for following systems in official repositories:

* [Debian (since version 11 "Bullseye")](https://packages.debian.org/source/bullseye/libcpuid): `apt install cpuidtool libcpuid14 libcpuid-dev`
* [Fedora (since version 25)](https://src.fedoraproject.org/rpms/libcpuid): `dnf install libcpuid libcpuid-devel`
* [FreeBSD (since version 11)](https://www.freshports.org/sysutils/libcpuid): `pkg install libcpuid`
* [OpenMandriva Lx (since version 4.0 "Nitrogen")](https://github.com/OpenMandrivaAssociation/libcpuid): `dnf install libcpuid-tools libcpuid14 libcpuid-devel`
* [openSUSE Leap (since version 15.1)](https://software.opensuse.org/package/libcpuid): `zypper install libcpuid-tools libcpuid14 libcpuid-devel`
* [Solus](https://packages.getsol.us/shannon/libc/libcpuid/): `eopkg install libcpuid libcpuid-devel`
* [Ubuntu (since version 20.04 "Focal Fossa")](https://packages.ubuntu.com/source/focal/libcpuid) : `apt install cpuidtool libcpuid14 libcpuid-dev`

#### Build tool

* Vcpkg: `vcpkg install cpuid`
