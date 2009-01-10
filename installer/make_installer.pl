#!/usr/bin/perl -w
# Author: Yann Riou (aka bibim)

use strict;
use File::Basename;

my $installerDir=$0;
$installerDir=$ENV{PWD}."/".$installerDir unless($installerDir =~ /^\//);
$installerDir=dirname($installerDir);

chdir("$installerDir/..");
die "Unable to find \"installer\" directory." unless(-d "installer");

my $testBuildString="";
my $tag=`git describe --candidate=0 2>/dev/null`;
if($?) {
  $testBuildString=" -DTEST_BUILD";
  $tag=`git describe`;
  die "Unable to run \"git describe\"." if($?);
  chomp($tag);
  print "Creating test installer for revision $tag\n";
}else{
  chomp($tag);
  print "Creating installer for release $tag\n";
}
system("sh", "installer/tasclient_download.sh");
system("sh", "installer/springlobby_download.sh");
system("makensis -V3$testBuildString -DREVISION=$tag installer/spring.nsi");
