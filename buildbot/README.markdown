# Making the chroot

At least debootstrap and schroot are required. I didn't compile an exhaustive list of other dependencies on the host system, so YMMV.

## Configuring schroot

Add to the end of /etc/schroot/schroot.conf:

	[buildbot-maverick]
	description=Ubuntu Maverick
	location=/fast/buildbot/chroots/maverick
	type=directory
	priority=3
	users=buildbot
	root-groups=root
	personality=linux32
	run-setup-scripts=true
	script-config=script-buildbot

## Building and entering the chroot

	debootstrap --variant=buildd --arch i386 maverick /fast/buildbot/chroots/maverick/ http://ftp.cvut.cz/ubuntu/
	mkdir -p /fast/buildbot/chroots/maverick/home/buildbot/www
	chown buildbot:buildbot /fast/buildbot/chroots/maverick/home/buildbot
	schroot -c buildbot-maverick

# Inside the chroot

## Run (as root)

	echo 'deb http://ftp.cvut.cz/ubuntu maverick universe' >> /etc/apt/sources.list
	sed s/deb/deb-src/g /etc/apt/sources.list >> /etc/apt/sources.list
	apt-get update
	apt-get install nano git-core python-dev python-setuptools

	cd /root
	git clone git://github.com/buildbot/buildbot.git
	cd buildbot
	git checkout v0.8.3p1
	cd slave
	python setup.py install
	mkdir /slave
	chown buildbot:buildbot /slave

Build dependencies for Windows build (except MinGW, see below):

	apt-get install cmake nsis p7zip-full unzip wget
	apt-get install openjdk-6-jdk   #to enable building of Java AIs

Build dependencies for Linux build:

	apt-get install libboost-all-dev libdevil-dev libfreetype6-dev libopenal-dev \
		libogg-dev libvorbis-dev libglew-dev libsdl-dev libxcursor-dev

## Recompile MinGW package with dwarf2 exceptions instead of sjlj exceptions (as root)

First recompile the compiler itself:

	mkdir mingw
	cd mingw
	apt-get build-dep gcc-mingw32
	apt-get source gcc-mingw32
	cd gcc-mingw32-4.4.4
	editor debian/rules   # remove --enable-sjlj-exceptions, add --disable-sjlj-exceptions --with-dwarf2
	dpkg-buildpackage -rfakeroot -b
	cd ..
	dpkg -i gcc-mingw32*.deb
	echo "gcc-mingw32 hold"|dpkg --set-selections

Then recompile the runtime libraries against the new compiler:

	apt-get build-dep mingw32-runtime
	apt-get source mingw32-runtime
	cd mingw32-runtime-3.15.2
	dpkg-buildpackage -rfakeroot -b
	cd ..
	dpkg -i mingw32-runtime*.deb
	echo "mingw32-runtime hold"|dpkg --set-selections

Optionally, remove build dependencies (check what apt-get build-dep installed!):

	apt-get purge autoconf automake autotools-dev cdbs debhelper fdupes gettext \
		gettext-base groff-base html2text intltool intltool-debian libcroco3 \
		libfont-afm-perl libgmp3-dev libgmpxx4ldbl libhtml-format-perl \
		libhtml-parser-perl libhtml-tagset-perl libhtml-tree-perl \
		libmail-sendmail-perl libmailtools-perl libmpfr-dev \
		libsys-hostname-long-perl libunistring0 liburi-perl libwww-perl \
		libxml-parser-perl lzma m4 man-db mingw-w64 po-debconf

## Patch a few things

We set up ld to allow only one execution at the same time, to prevent thrashing the box due to multiple parallel linker jobs.
We "patch" makensis to not sync the installer to disk after every tiny chunk of data it has written.
This speeds installer building up by an order of magnitude.

First, copy over ld, makensis and no-msync.c from spring/buildbot/slave (Spring repository) to the chroot.
Then switch to the directory (inside the chroot!) where you copied those files and do:

For Linux builds:

	mv /usr/bin/ld /usr/bin/ld.orig
	cp ld /usr/bin/ld

For MinGW builds:

	mv /usr/bin/i586-mingw32msvc-ld /usr/bin/i586-mingw32msvc-ld.orig
	cp ld /usr/bin/i586-mingw32msvc-ld
	mv /usr/i586-mingw32msvc/bin/ld /usr/i586-mingw32msvc/bin/ld.orig
	cp ld /usr/i586-mingw32msvc/bin/ld

For makensis:

	gcc -shared -m32 no-msync.c -o no-msync.so
	cp makensis no-msync.so /usr/local/bin/

## Run (as buildbot user)

Substitute MASTER with host:port of the buildmaster and SLAVENAME and PASSWORD with the slave name and password as configured in the master.

	buildslave create-slave /slave MASTER SLAVENAME PASSWORD
	cd /slave
	git clone git://github.com/spring/mingwlibs.git

## ccache

To install ccache simply run `apt-get install ccache` (inside the chroot) and (optionally) tweak the settings (cache size etc.)
