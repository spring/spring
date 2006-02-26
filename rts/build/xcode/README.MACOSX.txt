SpringRTS - Mac OSX Build

Contents:--------------------------------------------------------
  - Building using the existing xcode files (xcode 2.2)
    - Installing libraries
      - ZLib (libz)
      - OpenAL
      - SDL
      - DevIL
      - Boost C++
      - Freetype
      - GLEW
    - Datafiles
    - Mac Dylibs (install_path issue)
  - Building your own xcode files
  - Notes on scons
-----------------------------------------------------------------

----- Building using the existing xcode files (xcode 2.2) -----

The inbuilt xcode files will only work on xcode 2.2 (the current tiger version). They assume that your machine has the libraries configured as specified below, but you can fix the paths under Frameworks if you need to.

Currently only the xcode files for rts have been written. This means that you'll have to run either one of the scripts, wait for someone to port omni...i'll get to that after rts, or run it from the cmd line. I believe that the path will be fixed if you call it from within the bundle because of sdl.

On universal builds:
Once I have instructions for all of the libraries for universal builds finished i'll post a message on the mailing list but for now its ppc only...if you'd like to help each library is marked universal or not.

--- Installing libraries ---

- ZLib -

This ones real easy...you don't...its bundled with mac os already!

- OpenAL - 

Same as ZLib...doesn't get much better.

- SDL -

Not too hard, all you need is the framework, which includes the include files...you don't need the project files or anything else so the sdk isn't really required...grab it if you wan't it.

PS. Don't install into /System/Library/Frameworks...put it in /Library/Frameworks for system wide or ~/Library/Frameworks for single user only. /System is for apple stuff only, it also gets trashed everytime you upgrade to a new version of mac os.

The xcode files will look under /Library/Frameworks.

universal: not sure yet.

download (framework): http://www.libsdl.org/release/SDL-1.2.9.dmg
download (extra [sdk]): http://www.libsdl.org/release/SDL-devel-1.2.9.pkg.tar.gz

website: www.libsdl.org

- DevIL -

This one is a little trickier to understand. To install just grab and run the packages...pretty simple. The issues come from that it isn't a framework (explained  under Mac Dylibs).

Since the paths are fixed the xcode files should find them fine.

A little issue is that they were linked against fink, so if your not using fink you'll have to use this little trick for now, sudo ln -s /usr/local /sw

universal: no (is difficult to port due to deps)

download: http://prdownloads.sourceforge.net/openil/DevIL_Binary_1.6.7.tgz

website: www.imagelib.org

- Boost C++ -

Just grab it and unpack it into the /trunk/rts/build/xcode directory. Run the following commands to build it (i'm assuming you have bjam installed or build already...look at boost.org for instructions on that):

bjam --without-python --with-thread --with-filesystem --with-regex stage

xcode will looks for the dylibs under the stage dir if built where i suggested above. You'll also have to tweak the include dir if you unpack it somewhere else.

(I'll provide universal release dylibs soonish for this so that you don't have to compile it yourself).

universal: yes (i think, only ppc binaries of bjam available)

download: http://prdownloads.sourceforge.net/boost/boost_1_33_1.tar.bz2?download

website: www.boost.org

- Freetype -

At the moment I'm working with the copy of X11 (including X11SDK) installed as an optional under tiger (10.4). You can get x11 for panther as well i believe but freetype isn't very hard to compile either. The paths are fixed for it to be embedded either way so it will run on machines that don't have x11 installed.

universal: yes

download: n/a (i'll add this soon)

website: n/a (i'll add this soon)

- GLEW -

All you need to do is just grab the binaries of glew for mac os (the AGL version...NOT the glx one) and drop them in the /trunk/rts/build/xcode directory.

If you put them elsewhere you'll need to modify the include paths as well.

universal: yes (i think)

download: n/a (i'll add this soon)

website: n/a (i'll add this soon)

--- Datafiles ---

Just drop the datafiles into /trunk/rts/build/xcode and all will be well. ie. unpack to trunk/rts/build/xcode/Data

--- Mac Dylibs (install_path issue) ---

If you find yourself getting error messages about dyld not being able to find dylibs, chances are your dylibs are missing, or you've stumbled on to this dyld bug. Basically your applications grabs the install path from the dylib when it is being compiled and as a side affect, if its installed elsewhere, dyld simply won't find it (why they don't use search paths is beyond all of us, esp when search paths are used for frameworks).

I'm using a script build phase to fix the paths atm. As long as you redirect it to the correct files, it should still copy them appropriately and everything should work fine.

read-this: http://qin.laya.com/tech_coding_help/dylib_linking.html

----- Building your own xcode files -----

If your trying to roll your own xcode files them i'm going to assume you know what your doing anyway...If you don't try just changing paths in the files i've provided or waiting till is get scons working fully.

A couple of things to note...have a look at README.SRCFILES.txt for a description of what files not to compile and what c++ files need to be compiled as objective-c++ (where they don't already have the .mm ext).

----- Notes on scons -----

At the moment I'm using XCode as the build system, but this will be moved to scons after I get most stuff working on mac and fix/remove DevIL for universal/intel builds. The XCode files will then use scons to build and will just be for coding/debugging themselves.

Note that mac builds will generate a .app bundle, not a cmdline program, although you will still be able to run it off the cmdline using the usual Spring.app/Contents/MacOS/SpringRTS.

Lorenz (Krysole) Pretterhofer <krysole@internode.on.net>
