#!/usr/bin/env perl

###############################################################################
 #
 #  This file is part of meryl-utility, a collection of miscellaneous code
 #  used by Meryl, Canu and others.
 #
 #  This software is based on:
 #    'Canu' v2.0              (https://github.com/marbl/canu)
 #  which is based on:
 #    'Celera Assembler' r4587 (http://wgs-assembler.sourceforge.net)
 #    the 'kmer package' r1994 (http://kmer.sourceforge.net)
 #
 #  Except as indicated otherwise, this is a 'United States Government Work',
 #  and is released in the public domain.
 #
 #  File 'README.licenses' in the root directory of this distribution
 #  contains full conditions and disclaimers.
 ##

use strict;
use File::Compare;
use Cwd qw(getcwd);

my $cwd = getcwd();

#  Update version.H with module and version info.
#
#  Expects four parameters on the command line:
#    module-name version-label version-number path-to-version.H
#
#      module-name:       name of the module, e.g., 'verkko' or 'canu'.
#      version-label:     type of release this is:
#                           'snapshot' - query git to discover the version number
#                           'release'  - use the next arg as the version number
#      version-number:    the version string to report, e.g., v2.0.1
#                           MUST be of form 'vMAJOR.MINOR.PATCH'
#      path-to-version.H: path to file 'version.H'; 
#
#  The script will create output file 'path-to-version.H' and write
#  configuration information suitable for inclusion in both C source code and
#  Makefiles.

my $modName  = shift @ARGV;
my $MODNAME  = $modName;       $MODNAME =~ tr/a-z-/A-Z_/;

my $label    = shift @ARGV;   #  Expects snapshot or release
my $version  = shift @ARGV;   #  Expects vM.N.P
my $major    = "0";
my $minor    = "0";

if ($version =~ m!^v(\d+)\.(\d+\.?\d*)$!) {
    $major = $1;
    $minor = $2;
}

my $verFile  = shift @ARGV;

if (($modName eq "") || ($modName eq undef) || ($verFile eq undef)) {
    die "usage: $0 module-name version-label version-number version-file.H\n";
}

#
#  If in a git repo, we can get the actual values.
#

my @submodules;

my $commits  = undef;
my $hash1    = undef;          #  This from 'git describe'
my $hash2    = undef;          #  This from 'git rev-list'
my $revCount = 0;
my $dirty    = undef;
my $dirtya   = undef;
my $dirtyc   = undef;

if (system("git rev-parse --is-inside-work-tree > /dev/null 2>&1") == 0) {
    $label = "snapshot";

    #  Count the number of changes since the last release.
    open(F, "git rev-list HEAD |") or die "Failed to run 'git rev-list'.\n";
    while (<F>) {
        chomp;

        $hash2 = $_  if (!defined($hash2));
        $revCount++;
    }
    close(F);

    #  Find the commit and version we're at.
    open(F, "git describe --tags --long --always --abbrev=40 |") or die "Failed to run 'git describe'.\n";
    while (<F>) {
        chomp;
        if (m/^v(\d+)\.(.+)-(\d+)-g(.{40})$/) {
            $major   = $1;
            $minor   = $2;
            $commits = $3;
            $hash1   = $4;

            $version = "v$major.$minor";
        } else {
            $major   = "0";
            $minor   = "0";
            $commits = "0";
            $hash1   = $_;

            $version = "v$major.$minor";
        }
    }
    close(F);

    #  Decide if we've got locally committed changes.
    open(F, "git status |");
    while (<F>) {
        if (m/is\s+ahead\s/) {
            $dirtya = "ahead of github";
        }
        if (m/not\s+staged\s+for\s+commit/) {
            $dirtyc = "uncommitted changes";
        }
    }
    close(F);

    if    (defined($dirtya) && defined($dirtyc)) {
        $dirty = "ahead of github w/changes";
    }
    elsif (defined($dirtya)) {
        $dirty = "ahead of github";
    }
    elsif (defined($dirtyc)) {
        $dirty = "w/changes";
    }
    else {
        $dirty = "sync'd with github";
    }

    #  Figure out which branch we're on.

    my $branch = "master";

    open(F, "git branch --contains |");   #  Show branches that contain the tip commit.
    while (<F>) {
        next  if (m/detached\sat/);       #  Skip 'detached' states.

        s/^\*{0,1}\s+//;                  #  Remove any '*' annotation
        s/\s+$//;                         #  and all spaces.

        $branch = $_;                     #  Pick the first branch
        last;                             #  mentioned.
    }
    close(F);

    #  If a release branch, save major and minor version numbers.

    if ($branch =~ m/v(\d+)\.(\d+)/) {
        $major = $1;
        $minor = $2;
    }

    #  If on an actual branch, remember the branch name.

    if ($branch ne "master") {
        $label   = "branch";
        $version = $branch;
    }

    #  Get information on any submodules present.

    open(F, "git submodule status |");
    while (<F>) {
        if (m/^(.*)\s+(.*)\s+\((.*)\)$/) {
            push @submodules, "$2 $3 $1";
        }
    }
    close(F);
}

#
#  Dump a new header file, but don't overwrite the original.
#

open(F, "> $verFile.new") or die "Failed to open '$verFile.new' for writing: $!\n";
print F "#if 0\n";
print F "# Automagically generated by version_update.pl!  Do not commit!\n";
print F "#endif\n";
print F "\n";
print F "#if 0\n";
print F "#  These lines communicate the version information to C++.\n";
print F "#endif\n";
print F "#define ${MODNAME}_VERSION_LABEL     \"$label\"\n";
print F "#define ${MODNAME}_VERSION_MAJOR     \"$major\"\n";
print F "#define ${MODNAME}_VERSION_MINOR     \"$minor\"\n";
print F "#define ${MODNAME}_VERSION_COMMITS   \"$commits\"\n";
print F "#define ${MODNAME}_VERSION_REVISION  \"$revCount\"\n";
print F "#define ${MODNAME}_VERSION_HASH      \"$hash1\"\n";
print F "\n";

if      (defined($commits)) {
    print F "#define ${MODNAME}_VERSION           \"$modName $label $version +$commits changes (r$revCount $hash1)\\n\"\n";
} elsif (defined($hash1)) {
    print F "#define ${MODNAME}_VERSION           \"$modName snapshot ($hash1)\\n\"\n";
} elsif ($label  =~ m/release/) {
    print F "#define ${MODNAME}_VERSION           \"$modName $major.$minor\\n\"\n";
} else {
    print F "#define ${MODNAME}_VERSION           \"$modName $label ($version)\\n\"\n";
}

if ($MODNAME ne "MERYL_UTILITY") {
    print F "\n";
    print F "#undef  MERYL_UTILITY_VERSION\n";
    print F "#define MERYL_UTILITY_VERSION ${MODNAME}_VERSION\n";
}

print F "\n";
print F "\n";
print F "#if 0\n";
print F "#\n";
print F "#  These lines communicate version information to make.\n";
print F "#\n";

print F "define BUILDING\n";
if (defined($commits)) {
    print F "Building $label $version +$commits changes (r$revCount $hash1) ($dirty)\n";
} else {
    print F "Building $label $version\n";
}
foreach my $s (@submodules) {
    print F "  with $s\n";
}
print F "endef\n";
print F "#endif\n";

close(F);

#  If they're the same, don't replace the original.  This maintains the
#  timestamp, which makes make happier.

if (compare("$verFile", "$verFile.new") == 0) {
    unlink "$verFile.new";
} else {
    unlink "$verFile";
    rename "$verFile.new", "$verFile";
}

#  Now tell make what file we just made.

print "$verFile\n";

#  That's all, folks!

exit(0);
