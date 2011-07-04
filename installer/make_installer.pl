#!/usr/bin/perl -w
#
# @param distributionDir
#

use strict;
use File::Basename;
use File::Copy;
use File::Spec::Functions;

# Evaluate installer and root dirs
my $installerDir=$0;
$installerDir=$ENV{PWD}."/".$installerDir unless($installerDir =~ /^\//);
$installerDir=dirname($installerDir);

chdir("$installerDir/..");
die "Unable to find \"installer\" directory." unless(-d "installer");

my $nsisDefines="";

# Evaluate the engines version
my $tag=`git describe --candidate=0 --tags 2>/dev/null`;
if ($?) {
  $nsisDefines="$nsisDefines -DTEST_BUILD";
  $tag=`git describe --tags`;
  die "Unable to run \"git describe\"." if($?);
  chomp($tag);
  print "Creating test installer for revision $tag\n";
} else {
  chomp($tag);
  print "Creating installer for release $tag\n";
}
$nsisDefines="$nsisDefines -DVERSION_TAG=\"$tag\"";

# Download some files to be included in the installer
system("sh", "installer/springlobby_download.sh");
chdir("$installerDir/downloads");
system("wget", "-N", "http://springrts.com/dl/rapid-spring-latest-win32.7z");
system("wget", "-N", "http://springrts.com/dl/TASServer.jar");
system("wget", "-N", "http://www.springlobby.info/installer/springsettings.exe");
system("wget", "-N", "http://zero-k.info/lobby/setup.exe");
system("wget", "-N", "http://zero-k.info/lobby/setup_icon.ico");
chdir("$installerDir/..");

# Generate the installer
system("makensis -V3 $nsisDefines @ARGV -DNSI_UNINSTALL_FILES=sections/uninstall.nsh -DRAPID_ARCHIVE=downloads/rapid-spring-latest-win32.7z installer/spring.nsi");
