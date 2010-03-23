#!/usr/bin/perl -w
#
# @param distributionDir
#

use strict;
use File::Basename;

my $installerDir=$0;
$installerDir=$ENV{PWD}."/".$installerDir unless($installerDir =~ /^\//);
$installerDir=dirname($installerDir);

chdir("$installerDir/..");
die "Unable to find \"installer\" directory." unless(-d "installer");

# Aquire AI Interfaces and Skirmish AI versions
sub getSubDirsVersion {
  my $baseDir=shift;
  my %dirVersions;

  die("Cannot open directory \"$baseDir\": $!") unless(opendir(IMD, $baseDir));
  my @subDirs=grep {-d "$baseDir/$_" && $_ ne "." && $_ ne ".."} readdir(IMD);
  close(IMD);
  foreach my $subDir (@subDirs) {
    if(open(DAT,"$baseDir/$subDir/VERSION")) {
      my $dirVersion=<DAT>;
      close(DAT);
      chomp($dirVersion);
      $dirVersions{$subDir}=$dirVersion;
    }
  }
  return \%dirVersions;
}

sub getVersionVarsString {
  my ($baseDir,$varPrefix)=@_;
  my @vars;

  my $p_dirVersions=getSubDirsVersion($baseDir);
  foreach my $subDir (keys %{$p_dirVersions}) {
    push(@vars,"-D$varPrefix$subDir=$p_dirVersions->{$subDir}");
  }
  my $varsString=join(" ",@vars);
  return $varsString;
}

my $allVersStr= getVersionVarsString("AI/Interfaces/", "AI_INT_VERS_");
$allVersStr= $allVersStr." ".getVersionVarsString("AI/Skirmish/", "SKIRM_AI_VERS_");

my $testBuildString="";
my $tag=`git describe --candidate=0 --tags 2>/dev/null`;
if($?) {
  $testBuildString=" -DTEST_BUILD";
  $tag=`git describe  --tags`;
  die "Unable to run \"git describe\"." if($?);
  chomp($tag);
  print "Creating test installer for revision $tag\n";
}else{
  chomp($tag);
  print "Creating installer for release $tag\n";
}

system("sh", "installer/springlobby_download.sh");
chdir("$installerDir/downloads");
system("wget", "-N", "http://springrts.com/dl/TASServer.jar");
system("wget", "-N", "http://www.springlobby.info/installer/springsettings.exe");
system("wget", "-N", "http://files.caspring.org/caupdater/SpringDownloader.exe");
chdir("$installerDir/..");


# Evaluate the distribution dir
# This is where the build system installed Spring,
# and where the installer generater will grab files from.
my $distDir=$1;
if (($distDir=="") or (not -d $distDir)) {
	print("Distribution directory not found: \"$distDir\"\n");
	$distDir="dist";
	if (not -d $distDir) {
		print("Distribution directory not found: \"$distDir\"\n");
		$distDir="game";
		if (not -d $distDir) {
			print("Distribution directory not found: \"$distDir\"\n");
			die "Unable to find a distribution directory.";
		}
	}
}

system("makensis -V3$testBuildString -DREVISION=$tag -DDIST_DIR=$distDir $allVersStr installer/spring.nsi");
