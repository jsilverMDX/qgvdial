my $repo = "https://qgvdial.googlecode.com/svn/trunk";
my $s_make = "make.exe";
my $cmd;
my $dest;

my ($sec,$min,$hour,$day,$month,$yr19,@rest) = localtime(time);
$yr19 += 1900;
$month++;
my $suffix = sprintf("%04d%02d%02d", $yr19, $month, $day);
$dest = sprintf("I:/Uv/releases/qgvdial/%04d-%02d-%02d", $yr19, $month, $day);
if ((!(-e $dest)) or (!(-d $dest))) {
    $cmd = "mkdir $dest";
    print("$cmd\n");
    system($cmd);
}

# Delete any existing version file
if (-f ver.cfg)
{
    unlink(ver.cfg);
}
# Get the latest version file from the repository
$cmd = "svn export $repo/build-files/ver.cfg";
system($cmd);

# Pull out the version from the file
open(QVARFILE, "ver.cfg") or die;
my $qver = <QVARFILE>;
close QVARFILE;

# Get the subversion checkin version
system("svn log $repo --limit=1 | grep \"^r\" > svnlog.txt");
open(QVARFILE, "svnlog.txt") or die;
my $svnver = <QVARFILE>;
close QVARFILE;
unlink "svnlog.txt";

# Parse out the version number from the output we pulled out
$svnver =~ m/^r(\d+)*/;
$svnver = $1;
# Create the version suffix
$qver = "$qver.$svnver";

system("powershell Remove-Item -Recurse -Force qgvdial*");

$cmd = "svn export $repo qgvdial-$qver";
print("$cmd\n");
system($cmd);

# Version changes
$cmd = "cd qgvdial-$qver/src & perl ../build-files/version.pl __QGVDIAL_VERSION__ $qver";
print("$cmd\n");
system($cmd);

# # Fix the QML files
$cmd = "cd qgvdial-$qver & perl ./build-files/fixqml.pl ./qml";
print "$cmd\n";
system($cmd);

# This is the way to enter a directory and setup the remainder variables
my $enterdir = "cd qgvdial-$qver/src & set BUILDDIR=%CD% & set SOURCEDIR=%CD%";

# qmake, make release-gcce, make installer_sis
$cmd = "$enterdir & qmake.exe src.pro -r -spec symbian-sbsv2 CONFIG+=release -after OBJECTS_DIR=obj MOC_DIR=moc UI_DIR=ui RCC_DIR=rcc QMLJSDEBUGGER_PATH=%QTSDK_SLASH%/../QtCreator/share/qtcreator/qml/qmljsdebugger & $s_make release-gcce -w & $s_make installer_sis -w";
print("$cmd\n");
system($cmd);

# Copy the sis files to the outer directory
$cmd = "$enterdir & copy qgvdial.sis $dest/qgvdial_$qver.sis & copy qgvdial_installer.sis $dest/qgvdial_installer_$qver.sis";
print("$cmd\n");
system($cmd);
