#!/usr/bin/perl

# from Ubuntu 20.04

use strict;
use warnings FATAL => "all";

my $ccache_dir = "/usr/lib/ccache";
my @gcc_dirs = (
    "/usr/lib/gcc",
    "/usr/lib/gcc-cross",
    "/usr/lib/x86_64-linux-gnu/gcc",
);
my %old_symlinks; # Current compiler names in /usr/lib/ccache
my %new_symlinks; # Compiler names that should be in /usr/lib/ccache
my @standard_names = qw(cc c++);
my $clang_names = qr/^(llvm-)?clang(\+\+)?(-[0-9.]+)?$/;
my $verbose = 0;

sub consider {
    my ($name) = @_;
    if (-x "/usr/bin/$name") {
        $new_symlinks{$name} = 1;
    }
}

sub consider_gcc {
    my ($prefix, $suffix) = @_;
    consider "${prefix}gcc${suffix}";
    consider "${prefix}g++${suffix}";
}

# Find existing standard compiler names.
foreach (@standard_names) {
    consider $_;
}

# Find existing GCC variants.
consider_gcc "", "";
consider_gcc "c89-", "";
consider_gcc "c99-", "";
foreach my $gcc_dir (@gcc_dirs) {
    foreach my $dir (<$gcc_dir/*>) {
        (my $kind = $dir) =~ s|.*/||;
        consider_gcc "$kind-", "";
        foreach (<$dir/*>) {
            if (! -l $_ and -d $_) {
                s|.*/||;
                consider_gcc "", "-$_";
                consider_gcc "$kind-", "-$_";
            }
        }
    }
}

# Find existing clang variants.
foreach (</usr/bin/*clang*>) {
    s|.*/||;
    if (/$clang_names/) {
        consider $_;
    }
}

# Find existing symlinks.
foreach (<$ccache_dir/*>) {
    if (-l) {
        s|.*/||;
        $old_symlinks{$_} = 1;
    }
}

# Remove obsolete symlinks.
foreach (keys %old_symlinks) {
    if (! exists $new_symlinks{$_}) {
        print "Removing $ccache_dir/$_\n" if $verbose;
        unlink "$ccache_dir/$_";
    }
}

# Add missing symlinks.
foreach (keys %new_symlinks) {
    if (! exists $old_symlinks{$_}) {
        print "Adding $ccache_dir/$_\n" if $verbose;
        symlink "../../bin/ccache", "$ccache_dir/$_";
    }
}
