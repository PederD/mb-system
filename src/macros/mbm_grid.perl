eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
                    & eval 'exec perl -S $0 $argv:q'
                         if 0;
#--------------------------------------------------------------------
#    The MB-system:	mbm_grid.perl	6/11/99
#    $Id: mbm_grid.perl,v 5.2 2001-10-30 00:17:55 caress Exp $
#
#    Copyright (c) 1999, 2000 by
#    D. W. Caress (caress@mbari.org)
#      Monterey Bay Aquarium Research Institute
#      Moss Landing, CA
#    and D. N. Chayes (dale@lamont.ldgo.columbia.edu)
#      Lamont-Doherty Earth Observatory
#      Palisades, NY  10964
#
#    See README file for copying and redistribution conditions.
#--------------------------------------------------------------------
#
# Command:
#   mbm_grid
#
# Purpose:
#   Macro to generate a shellscript of MB-System commands
#   which, when executed, will generate a grid or mosaic of the
#   specified swath sonar data. The grid or mosaic may be of
#   bathymetry, (positive down), topography (positive up),
#   amplitude, or sidescan data. The primary purpose of this
#   macro is to allow the simple, semi-automated production of
#   grids and mosaics with a few command line arguments. The macro
#   can determine the area covered by the swath data and set the
#   grid bounds and dimensions accordingly. For users
#   seeking more control over the grid and mosaic parameters,
#   the full suite of mbgrid and mbmosaic commands are supported.
#   Truly ambitious users may edit the grid shellscript to take
#   advantage of MB-System and GMT capabilites not supported by
#   this macro.
#
# Basic Usage:
#   mbm_grid {-Ifilelist | -Ifile -Fformat} -Oroot
#             -Rwest/east/south/north -V
#
# Additional Options:
#            [-Adatatype -Bborder -Cclip -Dxdim/ydim -Edx/dy/units
#            -Fpriority_range -Ggridkind -H -Llonflip -M -N -Ppings
#            -Sspeed -Ttension -U{azimuth/factor | time}
#            -V -Wscale -Xextend
#            -Ypriority_file -Zbathdef]
#
# Author:
#   David W. Caress
#   Monterey Bay Aquarium Research Institute
#   Moss Landing, CA 95039
#   June 11, 1999
#
# Version:
#   $Id: mbm_grid.perl,v 5.2 2001-10-30 00:17:55 caress Exp $
#
# Revisions:
#   $Log: not supported by cvs2svn $
# Revision 5.1  2001/03/22  21:05:45  caress
# Trying to make release 5.0.beta0
#
# Revision 5.0  2000/12/01  22:58:01  caress
# First cut at Version 5.0.
#
# Revision 4.3  2000/10/03  21:42:17  caress
# Snapshot for Dale.
#
# Revision 4.2  2000/09/11  21:54:49  caress
# Added support for recursive and commented datalists.
#
# Revision 4.1  1999/10/21  20:42:32  caress
# Fixed verbosity.
#
# Revision 4.0  1999/06/25  17:53:53  caress
# Initial version.
#
#
$program_name = "mbm_grid";

# Deal with command line arguments
$command_line = "@ARGV";
&MBGetopts('A:a:B:b:C:c:D:d:E:e:F:f:G:g:HhI:i:L:l:MmO:o:P:p:R:r:S:s:T:t:U:u:VvW:w:X:x:Y:y:Z:z:');

$datatype = 		($opt_A || $opt_a || 1);
$border = 		($opt_B || $opt_b);
$clip = 		($opt_C || $opt_c);
$xdimydim = 		($opt_D || $opt_d);
$xintyint = 		($opt_E || $opt_e);
$format = 		($opt_F);
$priority_range = 	($opt_f);
$gridkind = 		($opt_G || $opt_g);
$help =    		($opt_H || $opt_h);
$file_data = 		($opt_I || $opt_i);
$lonflip =    		($opt_L || $opt_l);
$more = 		($opt_M || $opt_m);
$root =    		($opt_O || $opt_o);
$pings = 		($opt_P || $opt_p);
$bounds = 		($opt_R || $opt_r);
$speed = 		($opt_S || $opt_s);
$tension = 		($opt_T || $opt_t);
$azimuthfactortime = 	($opt_U || $opt_u);
$verbose = 		($opt_V || $opt_v);
$scale = 		($opt_W || $opt_w);
$extend = 		($opt_X || $opt_x);
$priority_file = 	($opt_Y || $opt_y);
$bathdef = 		($opt_Z || $opt_z);

# print out help message if required
if ($help)
	{
	print "\n$program_name:\n";
        print "Macro to generate a shellscript of MB-System commands \n";
        print "which, when executed, will generate a grid or mosaic of the \n";
        print "specified swath sonar data. The grid or mosaic may be of \n";
        print "bathymetry, (positive down), topography (positive up), \n";
        print "amplitude, or sidescan data. The primary purpose of this  \n";
        print "macro is to allow the simple, semi-automated production of \n";
        print "grids and mosaics with a few command line arguments. The macro \n";
        print "can determine the area covered by the swath data and set the  \n";
        print "grid bounds and dimensions accordingly. For users \n";
        print "seeking more control over the grid and mosaic parameters, \n";
        print "the full suite of mbgrid and mbmosaic commands are supported. \n";
        print "Truly ambitious users may edit the grid shellscript to take  \n";
        print "advantage of MB-System and GMT capabilites not supported by  \n";
        print "this macro. \n";
        print "\nBasic Usage:\n\tmbm_grid {-Ifilelist | -Fformat -IFile} -Oroot -V  \n";
        print "\nAdditional Options:\n\t-Rwest/east/south/north \n";
        print "         [-Adatatype -Bborder -Cclip -Dxdim/ydim -Edx/dy/units \n";
        print "         -fpriority_range -Ggridkind -H -Llonflip -M -N -Ppings  \n";
        print "         -Sspeed -Ttension -U{azimuth/factor | time}  \n";
        print "         -V -Wscale -Xextend  \n";
        print "         -Ypriority_file -Zbathdef]  \n";
	exit 0;
	}

# check for input file
if (!$file_data)
	{
	print "\a";
	die "No input file specified - $program_name aborted\n";
	}
elsif (! -e $file_data)
	{
	print "\a";
	die "Specified input file cannot be opened - $program_name aborted\n";
	}

# tell the world we got started
if ($verbose) 
	{
	print "\nRunning $program_name...\n";
	}

# set root name if needed
if (!$root)
	{
	$root = $file_data;
	}

# get format if needed
if (!$format) 
	{
	$line = `mbformat -I $file_data -L`;
	($format) = $line =~ /(\S+)/;
	if ($format == 0)
		{
		$format = -1;
		}
	}

# get limits of file using mbinfo
if ($format > 0)
	{
	push(@files_data, $file_data);
	push(@formats, $format);
	}
else
	{
	# we used to use this perl function
	# 	MBparsedatalist($file_data);
	# but now we use the program mbdatalist
	@mbdatalist = `mbdatalist -F-1 -I$file_data`;
	while (@mbdatalist)
		{
		$line = shift @mbdatalist;
		if ($line =~ /(\S+)\s+(\S+)/)
			{
			($file_mb,$format_mb) = 
				$line =~ /(\S+)\s+(\S+)/;
			push(@files_data, $file_mb);
			push(@formats, $format_mb);
			}
		}
	}

$cnt = -1;
foreach $file_mb (@files_data)
	{
	# use .inf file if it exists and no time or space bounds applied
	$use_inf = 0;
	$file_inf = $file_mb . ".inf";
	if (-r $file_inf && !$bounds)
		{
		if ($verbose) 
			{
			print "Reading mbinfo output from file $file_inf...\n";
			}
		if (open(FILEINF,"<$file_inf"))
			{
			while (<FILEINF>)
				{
				push(@mbinfo, $_);
				}
			close FILEINF;
			$use_inf = 1;
			}
		}

	# if .inf file not accessible or suitable run mbinfo directly 
	if (!$use_inf)
		{
		if ($verbose) 
			{
			print "Running mbinfo on file $file_mb...\n";
			}
		$cnt++;
		if (!$bounds)
			{
			@mbinfo = `mbinfo -F$formats[$cnt] -I$file_mb -G`;
			}
		else
			{
			@mbinfo = `mbinfo -F$formats[$cnt] -I$file_mb -R$bounds -G`;
			}
		}

	# now parse the mbinfo input 
	$nrec_f = 0;
	while (@mbinfo)
		{
		$line = shift @mbinfo;
		if ($line =~ /Number of Records:\s+(\S+)/)
			{
			($nrec_f) = 
				$line =~ /Number of Records:\s+(\S+)/;
			}
		if ($line =~ /Minimum Longitude:\s+(\S+)\s+Maximum Longitude:\s+(\S+)/)
			{
			($xmin_f,$xmax_f) = 
				$line =~ /Minimum Longitude:\s+(\S+)\s+Maximum Longitude:\s+(\S+)/;
			}
		if ($line =~ /Minimum Latitude:\s+(\S+)\s+Maximum Latitude:\s+(\S+)/)
			{
			($ymin_f,$ymax_f) = 
				$line =~ /Minimum Latitude:\s+(\S+)\s+Maximum Latitude:\s+(\S+)/;
			}
		if ($line =~ /Minimum Altitude:\s+(\S+)\s+Maximum Altitude:\s+(\S+)/)
			{
			($altmin_f,$altmax_f) = 
				$line =~ /Minimum Altitude:\s+(\S+)\s+Maximum Altitude:\s+(\S+)/;
			}
		if ($line =~ /Minimum Depth:\s+(\S+)\s+Maximum Depth:\s+(\S+)/)
			{
			($zmin_f,$zmax_f) = 
			$line =~ /Minimum Depth:\s+(\S+)\s+Maximum Depth:\s+(\S+)/;
			}
		if ($line =~ /Minimum Amplitude:\s+(\S+)\s+Maximum Amplitude:\s+(\S+)/)
			{
			($amin_f,$amax_f) = 
			$line =~ /Minimum Amplitude:\s+(\S+)\s+Maximum Amplitude:\s+(\S+)/;
			}
		if ($line =~ /Minimum Sidescan:\s+(\S+)\s+Maximum Sidescan:\s+(\S+)/)
			{
			($smin_f,$smax_f) = 
			$line =~ /Minimum Sidescan:\s+(\S+)\s+Maximum Sidescan:\s+(\S+)/;
			}
		}

	if (!$first_mb && $nrec_f > 0)
		{
		$first_mb = 1;
		$xmin_data = $xmin_f;
		$xmax_data = $xmax_f;
		$ymin_data = $ymin_f;
		$ymax_data = $ymax_f;
		$zmin_data = $zmin_f;
		$zmax_data = $zmax_f;
		$amin_data = $amin_f;
		$amax_data = $amax_f;
		$smin_data = $smin_f;
		$smax_data = $smax_f;
		}
	elsif ($nrec_f > 0)
		{
		$xmin_data = &min($xmin_data, $xmin_f);
		$xmax_data = &max($xmax_data, $xmax_f);
		$ymin_data = &min($ymin_data, $ymin_f);
		$ymax_data = &max($ymax_data, $ymax_f);
		$zmin_data = &min($zmin_data, $zmin_f);
		$zmax_data = &max($zmax_data, $zmax_f);
		$amin_data = &min($amin_data, $amin_f);
		$amax_data = &max($amax_data, $amax_f);
		$smin_data = &min($smin_data, $smin_f);
		$smax_data = &max($smax_data, $smax_f);
		}
	}

# check that there is data
if ($xmin_data >= $xmax_data || $ymin_data >= $ymax_data)
	{
	die "$xmin_data $xmax_data $ymin_data $ymax_data \nDoes not appear to be any data in the input!\n$program_name aborted.\n";
	}
if (($datatype == 1 || $datatype == 2) && ($zmin_data >= $zmax_data))
	{
	die "Does not appear to be any bathymetry data in the input!\n$program_name aborted.\n";
	}
if (($datatype == 3) && ($amin_data >= $amax_data))
	{
	die "Does not appear to be any amplitude data in the input!\n$program_name aborted.\n";
	}
if (($datatype == 4) && ($smin_data >= $smax_data))
	{
	die "Does not appear to be any sidescan data in the input!\n$program_name aborted.\n";
	}

# either use user defined geographic limits
if ($bounds =~ /^\S+\/\S+\/\S+\/\S+$/)
       	{
       	($xmin_raw,$xmax_raw,$ymin_raw,$ymax_raw) = $bounds =~
       		/(\S+)\/(\S+)\/(\S+)\/(\S+)/;
       	$xmin = &GetDecimalDegrees($xmin_raw);
       	$xmax = &GetDecimalDegrees($xmax_raw);
       	$ymin = &GetDecimalDegrees($ymin_raw);
       	$ymax = &GetDecimalDegrees($ymax_raw);
       	$bounds_grid = $bounds;
       	}

# or expand the data limits a bit and ensure a reasonable aspect ratio
else
	{
	$delx = 0.05 * ($xmax_data - $xmin_data);
	$dely = 0.05 * ($ymax_data - $ymin_data);
	$xmin = $xmin_data - $delx;
	$xmax = $xmax_data + $delx;
	$ymin = $ymin_data - $dely;
	$ymax = $ymax_data + $dely;
	$dx = $xmax - $xmin;
	$dy = $ymax - $ymin;
	if ($dy/$dx > 2.0)
		{
		$delx = 0.5 * (0.5 * $dy - $dx);
		$xmin = $xmin - $delx;
		$xmax = $xmax + $delx;
		}
	elsif ($dx/$dy > 2.0)
		{
		$dely = 0.5 * (0.5 * $dx - $dy);
		$ymin = $ymin - $dely;
		$ymax = $ymax + $dely;
		}
	$bounds_grid = sprintf ("%1.8g/%1.8g/%1.8g/%1.8g",
		$xmin, $xmax, $ymin, $ymax);
	}

# get grid interval
if (!$xintyint && !$xdimydim)
	{
	if (!$altmax_f && $bathdef)
		{
		$zmax_data = $altmax_f;
		}
	elsif (!$zmax_data && $bathdef)
		{
		$zmax_data = $bathdef;
		}
	elsif (!$zmax_data)
		{
		$bathdef = 1000.0;
		$zmax_data = $bathdef;
		}
	$xinterval = 0.07 * $zmax_data;
	$xdim = ($xmax - $xmin) * 110000.0 / $xinterval;
	$ydim = ($ymax - $ymin) * 110000.0 / $xinterval;
	if ($xdim > 501 && $xdim > $ydim)
		{
		$xinterval = ($xmax - $xmin) * 110000.0 / 501;
		}
	elsif ($ydim > 501)
		{
		$xinterval = ($ymax - $ymin) * 110000.0 / 501;
		}
	$xintyint = "$xinterval/$xinterval/meters";
	}

# come up with the filenames
if ($datatype < 3)
	{
	$cmdfile = "$root"."_mbgrid.cmd";
	}
else
	{
	$cmdfile = "$root"."_mbmosaic.cmd";
	}
if ($format == -1)
	{
	$file_list = "\$INPUT_FILE";
	}
else
	{
	$file_list = "datalist\$\$";
	}

# open the shellscript file
if (!open(FCMD,">$cmdfile"))
	{
	print "\a";
	die "Cannot open output file $cmdfile\nMacro $program_name aborted.\n";
	}

# write the shellscript header
print FCMD "#! /bin/csh -f\n";
if ($datatype < 3)
	{
	print FCMD "#\n# Shellscript to grid or mosaic swath sonar data\n";
	}
else
	{
	}
print FCMD "# Created by macro $program_name\n";
print FCMD "#\n# This shellscript created by following command line:\n";
print FCMD "# $program_name $command_line\n";

# Define shell variables
print FCMD "#\n# Define shell variables used in this script:\n";
print FCMD "set REGION          = $bounds_grid\n";
print FCMD "set INPUT_FILE      = $file_data\n";
print FCMD "set INPUT_FORMAT    = $format\n";
print FCMD "set ROOT       	= $root\n";

# generate datalist file if needed
if ($format && $format != -1)
	{
	print FCMD "#\n# Make datalist file \n";
	print FCMD "echo Making datalist file...\n";
	print FCMD "echo \$INPUT_FILE \$INPUT_FORMAT >! $file_list\n";
	}

# do mbgrid if bathymetry or topography needed
if ($datatype < 3)
	{
	print FCMD "#\n# Run mbgrid\n";
	print FCMD "echo Running mbgrid...\n";
	printf FCMD "mbgrid -I$file_list \\\n";
	}
# do mbmosaic if sidescan or amplitude needed
else
	{
	print FCMD "#\n# Run mbmosaic\n";
	print FCMD "echo Running mbmosaic...\n";
	printf FCMD "mbmosaic -I$file_list \\\n";
	}
printf FCMD "\t-R\$REGION \\\n";
printf FCMD "\t-O\$ROOT \\\n";
printf FCMD "\t-A$datatype -N \\\n";
if ($xdimydim)
	{
	printf FCMD "\t-D$xdimydim \\\n";
	}
elsif ($xintyint)
	{
	printf FCMD "\t-E$xintyint \\\n";
	}
if ($border)
	{
	printf FCMD "\t-B \\\n";
	}
if ($clip)
	{
	printf FCMD "\t-C$clip \\\n";
	}
if ($priority_range)
	{
	printf FCMD "\t-F$priority_range \\\n";
	}
if ($gridkind)
	{
	printf FCMD "\t-G$gridkind \\\n";
	}
if ($lonflip)
	{
	printf FCMD "\t-L$lonflip \\\n";
	}
if ($more)
	{
	printf FCMD "\t-M \\\n";
	}
if ($speed)
	{
	printf FCMD "\t-S$speed \\\n";
	}
if ($tension)
	{
	printf FCMD "\t-T$tension \\\n";
	}
if ($azimuthfactortime)
	{
	printf FCMD "\t-U$azimuthfactortime \\\n";
	}
if ($scale)
	{
	printf FCMD "\t-W$scale \\\n";
	}
if ($extend)
	{
	printf FCMD "\t-X \\\n";
	}
# mbmosaic only options
if ($datatype > 2)
	{
 	if ($priority_file)
 		{
 		printf FCMD "\t-Y$priority_file \\\n";
 		}
 	if ($bathdef)
 		{
 		printf FCMD "\t-Z$bathdef \\\n";
 		}	
	}
if ($verbose)
	{
	printf FCMD "\t-V \n";
	}
else
	{
	printf FCMD "\n";
	}

# claim it's all over
print FCMD "#\n# All done!\n";
print FCMD "echo All done!\n";

# now close the shellscript and make it executable
close FCMD;
chmod 0775, $cmdfile;

# tell program status
if ($verbose)
	{
	print "\nProgram Status:\n";
	print "--------------\n";
	print "\n--------------\n";
	}

# print out final notes
print "\nGrid generation shellscript <$cmdfile> created.\n";
print "\nInstructions:\n";
print "  Execute <$cmdfile> to generate grid file <$root.grd>.\n";
if ($verbose)
	{
	print "\n--------------\n\n";
	}

# execute shellscript if desired
if ($execute)
	{
	if ($verbose)
		{
		print "Executing shellscript $cmdfile...\n";
		}
	system "$cmdfile &";
	}
exit 0;

#-----------------------------------------------------------------------
sub min {

	# make local variables
	local ($min);
	
	# get the minimum of the arguments
	if ($_[0] < $_[1])
		{
		$min = $_[0];
		}
	else
		{
		$min = $_[1];
		}
	$min;
}
#-----------------------------------------------------------------------
sub max {

	# make local variables
	local ($max);
	
	# get the minimum of the arguments
	if ($_[0] > $_[1])
		{
		$max = $_[0];
		}
	else
		{
		$max = $_[1];
		}
	$max;
}
#-----------------------------------------------------------------------
sub MBparsedatalist {
	local ($FILEDATA, $line, $file_tmp, $format_tmp, @datalists, $datalist);

 	if (open(FILEDATA,"<$_[0]"))
        	{
        	while ($line = <FILEDATA>)
        		{
			if (!($line =~ /^#/)
			    && $line =~ /\S+\s+\S+/)
			    {
        		    ($file_tmp, $format_tmp) = $line =~ /(\S+)\s+(\S+)/;
        		    if ($file_tmp && $format_tmp >= 0)
        		 	{
        			push(@files_data, $file_tmp);
        			push(@formats, $format_tmp);
        			}
			    elsif ($file_tmp && $format_tmp == -1)
				{
        			push(@datalists, $file_tmp);
				}
			    }
        		}
        	close FILEDATA;
        	}

	# loop over datalists 
	foreach $datalist (@datalists)
		{
		MBparsedatalist($datalist);
		}

}
#-----------------------------------------------------------------------
sub GetDecimalDegrees {

	# make local variables
	local ($dec_degrees, $degrees, $minutes, $seconds);

	# deal with dd:mm:ss format
	if ($_[0] =~ /^\S+:\S+:\S+$/)
		{
		($degrees, $minutes, $seconds) 
			= $_[0] =~ /^(\S+):(\S+):(\S+)$/;
		if ($degrees =~ /^-\S+/)
			{
			$dec_degrees = $degrees 
				- $minutes / 60.0 
				- $seconds / 3600.0;
			}
		else
			{
			$dec_degrees = $degrees 
				+ $minutes / 60.0 
				+ $seconds / 3600.0;
			}
		}
	# deal with dd:mm format
	elsif ($_[0] =~ /^\S+:\S+$/)
		{
		($degrees, $minutes) 
			= $_[0] =~ /^(\S+):(\S+)$/;
		if ($degrees =~ /^-\S+/)
			{
			$dec_degrees = $degrees - $minutes / 60.0;
			}
		else
			{
			$dec_degrees = $degrees + $minutes / 60.0;
			}
		}

	# value already in decimal degrees
	else
		{
		$dec_degrees = $_[0];
		}

	# return decimal degrees;
	$dec_degrees;
}

#-----------------------------------------------------------------------
# This version of Getopts has been augmented to support multiple
# calls to the same option. If an arg in argumentative is followed
# by "+" rather than ":",  then the corresponding scalar will
# be concatenated rather than overwritten by multiple calls to
# the same arg.
#
# Usage:
#      do Getopts('a:b+c'); # -a takes arg, -b concatenates args,  
#			    # -c does not take arg. Sets opt_* as a
#                           # side effect.

sub MBGetopts {
    local($argumentative) = @_;
    local(@args,$_,$first,$rest);
    local($errs) = 0;
    local($[) = 0;

    @args = split( / */, $argumentative );
    while(@ARGV && ($_ = $ARGV[0]) =~ /^-(.)(.*)/) {
	($first,$rest) = ($1,$2);
	$pos = index($argumentative,$first);
	if($pos >= $[) {
	    if($args[$pos+1] eq ':') {
		shift(@ARGV);
		if($rest eq '') {
		    ++$errs unless @ARGV;
		    $rest = shift(@ARGV);
		}
		eval "\$opt_$first = \$rest;";
		eval "\$flg_$first = 1;";
	    }
	    elsif($args[$pos+1] eq '+') {
		shift(@ARGV);
		if($rest eq '') {
		    ++$errs unless @ARGV;
		    $rest = shift(@ARGV);
		}
		if (eval "\$opt_$first") {
		    eval "\$opt_$first = \$opt_$first 
				. \":\" . \$rest;";
		}
		else {
		    eval "\$opt_$first = \$rest;";
		}
		eval "\$flg_$first = 1;";
	    }
	    elsif($args[$pos+1] eq '%') {
		shift(@ARGV);
		if($rest ne '') {
		    eval "\$opt_$first = \$rest;";
		}
		else {
		    $rest = $ARGV[0];
		    ($one) = $rest =~ /^-(.).*/;
		    $pos = index($argumentative,$one);
		    if(!$one || $pos < $[) {
			eval "\$opt_$first = \$rest;";
			shift(@ARGV);
		    }
		}
		eval "\$flg_$first = 1;";
	    }
	    else {
		eval "\$opt_$first = 1";
		eval "\$flg_$first = 1;";
		if($rest eq '') {
		    shift(@ARGV);
		}
		else {
		    $ARGV[0] = "-$rest";
		}
	    }
	}
	else {
	    print STDERR "Unknown option: $first\n";
	    ++$errs;
	    if($rest ne '') {
		$ARGV[0] = "-$rest";
	    }
	    else {
		shift(@ARGV);
	    }
	}
    }
    $errs == 0;
}
#-----------------------------------------------------------------------
