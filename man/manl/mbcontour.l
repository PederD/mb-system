.TH mbcontour l "31 October 1994"
.SH NAME
\fBmbcontour\fP - GMT compatible utility for color fill or color shaded relief
swath plots of multibeam data using Postscript.

.SH VERSION
Version 4.1

.SH SYNOPSIS
\fBmbcontour\fP \fB-J\fIparameters\fP \fB-R\fIwest/east/south/north \fP[\fB-A\fIcont_int/col_int/tic_int/lab_int/tic_int/lab_hgt\fP \fB-B\fItickinfo\fP \fB-b\fIyr/mo/da/hr/mn/sc\fP \fB-C\fIcontourfile\fP \fB-D\fItime_tick/time_annot/date_annot/time_tick_len\fP \fB-E\fIyr/mo/da/hr/mn/sc\fP \fB-F\fIred/green/blue\fP \fB-I\fIdatalist\fP \fB-K\fP \fB-L\fIlonflip\fP \fB-M\fP \fB-N\fInplot\fP \fB-O\fP \fB-P\fP \fB-p\fIpings\fP \fB-Q\fP \fB-T\fItimegap\fP \fB-U\fP \fB-X\fIx-shift\fP \fB-Y\fIy-shift\fP \fB-Z\fIalgorithm\fP \fB-#\fIcopies\fP \fB-V -H\fP]

.SH DESCRIPTION
\fBmbcontour\fP is an utility for swath contouring of multibeam bathymetry 
data using Postscript.  Like \fBmbswath\fP, \fBmbcontour\fP
is fully compatible with the GMT package version 2.1. Two contouring
algorithms are available. The first draws contours between successive
pings; this algorithm is simple and therefore fast but produces poor
looking contours when beams from one ping lie "behind" beams from a
previous ping (this happens for sonars that ping at nonnull pitch
angles or for the "inside" beams when ships make sharp turns). The
second algorithm constructs a triangular network from the available
soundings and contours the network; this approach is complicated
and therefore slow but produces good looking contours under most
conditions. The contour levels 
and colors can be controlled directly or set implicitly using contour 
and color change intervals. Contours can also be set to have ticks 
pointing downhill.

.SH AUTHORSHIP
David W. Caress (caress@lamont.ldeo.columbia.edu)
.br
Dale N. Chayes (dale@lamont.ldeo.columbia.edu)
.br
Lamont-Doherty Earth Observatory
.br
Palisades, NY 10964

.SH OPTIONS
.TP
.B \fB-A\fIcont_int/col_int/tic_int/lab_int/tic_len/lab_hgt\fP
If specific contour levels are not specified with the \fI-C\fP option,
then contours will be
generated in four colors (black, red, green, and blue) with intervals,
annotations, and tickmarks controlled by these parameters.  Contours will be
generated at invervals of \fIcont_int\fP meters.  Color changes
will occur at intervals of \fIcol_int\fP meters.  Contours will have
downhill facing tickmarks \fItic_int\fP inches long every \fItic_int\fP
meters. Contours will have annotations 
\fIlabel_len\fP inches high every \fIlab_int\fP meters.
Defaults: \fIcont_int\fP = 25; \fIcol_int\fP = 100; 
\fItic_int\fP = 100; \fIlab_int\fP = 100; \fItic_int\fP = 0.1; 
\fIlab_hgt\fP = 0.05.
.TP
.B \fB-B\fItickinfo\fP
Sets map boundary tickmark intervals. See \fBpsbasemap\fP for details.
.TP
.B \fB-C\fIcontourfile\fP
Sets the file from which the contour levels and their style and color are read.
This file consists of lines in the format:
 	\fIlevel label tick red green blue\fP
.br
where \fIlevel\fP is the depth value to be contoured in meters, \fIlabel\fP
is either 'a' for annotated or 'n' for not annotated, \fItick\fP is either
't' for tick marks or 'n' for no tick marks, and \fIred\fP, \fIgreen\fP,
and \fIblue\fP set the color of the contour lines (these values can
range from 0 to 255). If a GMT cpt file is given as the \fIcontourfile\fP,
the color boundaries specified in the cpt file will be interpreted as
contour levels to be plotted with unannotated, unticked, black contours.
If contours are specified using the \fB-C\fP option, then any use
of the \fB-A\fP option will be ignored.
.TP
.B \fB-b\fIyr/mo/da/hr/mn/sc\fP
Sets the starting time for data allowed in the input data; pings
with times before the starting time will be ignored. 
Default: \fIyr/mo/da/hr/mn/sc\fP = 1962/2/21/10/30/0.
.TP
.B \fB-D\fItime_tick/time_annot/date_annot/time_tick_len\fP
Sets the parameters governing the annotation of the shiptrack.  
Time marks are made with "X" marks along the shiptrack; annotated
time marks show the time in HH:MM format next to the time mark
and annotated date marks show the time and julian day in
HH:MM/DDD format.  The "X" marks are
\fItime_tick_len\fP inches high for normal time marks and 
1.5 times \fItime_tick_len\fP inches high for annotated time or date
marks.  The interval of time ticks, annotated time ticks, and 
annotated date ticks are given in hours by 
\fItime_tick\fP, \fItime_annot\fP, and \fIdate_annot\fP, respectively.
Defaults: \fItime_tick\fP = 0.25; \fItime_annot\fP = 1.0; 
\fIdate_annot\fP = 4.0; \fItime_tick_len\fP = 0.1.
.TP
.B \fB-E\fIyr/mo/da/hr/mn/sc\fP
Sets the ending time for data allowed in the input data; pings
with times after the ending time will be ignored. 
Default: \fIyr/mo/da/hr/mn/sc\fP = 2062/2/21/10/30/0.
.TP
.B \fB-f\fIformat\fP
Sets the data format used if the input is read from stdin
(the default, which can be overridden with the \fB-I\fP option).
This program uses the \fBMBIO\fP library and will read or write any multibeam
format supported by \fBMBIO\fP. A list of the multibeam data formats
currently supported by \fBMBIO\fP and their identifier values
is given in the \fBMBIO\fP manual page.
.TP
.B \fB-F\fIred/green/blue\fP
Sets the color used for Frame and annotation. [Default is black]
.TP
.B \fB-H\fP
This "help" flag cause the program to print out a description
of its operation and then exit immediately.
.TP
.B \fB-I\fIdatalist\fP
Sets the filename of the file containing a list of the input multibeam
data files and their formats.  The program will read and plot the
data in each one of these files instead of the default, which is
to read and plot a single data stream from stdin.
In the \fIdatalist\fP file, each
data file should be followed by a data format identifier, e.g.:
 	datafile1 11
 	datafile2 24
.br
This program uses the \fBMBIO\fP library and will read or write any multibeam
format supported by \fBMBIO\fP. A list of the multibeam data formats
currently supported by \fBMBIO\fP and their identifier values
is given in the \fBMBIO\fP manual page.
.TP
.B \fB-J\fP
Selects the map projection.  Choose one of the following:
.br
\fB\-Ja\fP\fIlon0/lat0/scale[r]\fP (Lambert Azimuthal Equal-Area).
.br
\fB\-Jb\fP\fIlon0/lat0/lat1/lat2/scale[r]\fP (Albers Conic Equal-Area).
.br
\fB\-Jc\fP\fIlon0/lat0/scale[r]\fP (Cassini).
.br
\fB\-Je\fP\fIlon0/lat0/scale[r]\fP (Azimuthal Equidistant).
.br
\fB\-Jg\fP\fIlon0/lat0/scale[r]\fP (Orthographic).
.br
\fB\-Jh\fP\fIlongitude/radius\fP (Hammer Equal-Area)
.br
\fB\-Ji\fP\fIwidth\fP (Interrupted Sinusoidal)
.br
\fB\-Jl\fP\fIlon0/lat0/lat1/lat2/scale\fP (Lambert Conic Conformal)
.br
\fB\-Jm\fP\fIscale\fP (Mercator)
.br
\fB\-Joa\fP\fIorigin_x/origin_y/azimuth/scale[r]\fP (Oblique Mercator - point and azimuth)
.br
\fB\-Job\fP\fIorigin_x/origin_y/point_x/point_y/scale[r]\fP (Oblique Mercator - two points)
.br
\fB\-Joc\fP\fIorigin_x/origin_y/pole_x/pole_y/scale[r]\fP (Oblique Mercator - point and pole)
.br
\fB\-Jr\fP\fIlongitude/radius\fP (Winkel Tripel)
.br
\fB\-Js\fP\fIlon0/lat0/scale[r]\fP (Stereographic)
.br
\fB\-Jt\fP\fIlongitude/scale\fP (TM - Transverse Mercator)
.br
\fB\-Ju\fP\fIzone/scale\fP (UTM - Universal Transverse Mercator)
.br
\fB\-Jw\fP\fIlongitude/radius\fP (Mollweide Equal-Area)
.br
\fB\-Jp\fP\fIradius\fP (Linear scaling of polar (theta,r) coordinates)
.br
\fB\-Jx\fP\fIxscale[/yscale]\fP (Linear scaling of cartesian coordinates)
.br
More details can be found in the \fBpsbasemap\fP man pages.
.TP
.B \-K
More \fIPostScript\fP code will be appended later [Default terminates the plot system].
.TP
.B \fB-L\fIlonflip\fP
Sets the range of the longitude values returned by the multibeam i/o routines.
If \fIlonflip\fP=-1 then the longitude values will be in
the range from -360 to 0 degrees. If \fIlonflip\fP=0 
then the longitude values will be in
the range from -180 to 180 degrees. If \fIlonflip\fP=1 
then the longitude values will be in
the range from 0 to 360 degrees.
Default: \fIlonflip\fP = 0.
.TP
.B \-M
Force conversion to monochrome image using the [television] YIQ transformation.
.TP
.B \fB-N\fInplot\fP
Sets the number of pings to be read in before each contouring
episode. See the description of the \fB-Z\fIalgorithm\fP option
for advice on reasonable values
Default: \fInplot\fP = 50 unless \fB-Z\fI1\fP is specified,
in which case the default is \fInplot\fP = 5.
.TP
.B \-O
Selects Overlay plot mode [Default initializes a new plot system].
.TP
.B \-P
Selects Portrait plotting mode [GMT Default is Landscape, see gmtdefaults to change this].
.TP
.B \fB-p\fIpings\fP
Sets the ping averaging of the input data. If \fIpings\fP = 1, then
no ping averaging is performed. If \fIpings\fP > 0, then
that number of input pings will be averaged to produce one output
ping.  If \fIpings\fP = 0, then the ping averaging will automatically
be done so that the along-track ping spacing is equal to the across-track
beam spacing.
Default: \fIpings\fP = 1 (no ping averaging).
.TP
.B \fB-Q\fP
Causes the program to plot the triangles constructed as part of the
contouring if the "triangle algorithm" is specified using the
\fB-Z\fI0\fP option. This allows the data distribution to be directly viewed.
.TP
.B \fB-R\fIwest/east/south/north\fP
Sets the longitude and latitude bounds within which multibeam 
data will be read and plotted. Only the data which lies within 
these bounds will be read. 
Default: \fIwest\fP=-360, east\fI=360\fP, \fIsouth\fP=-90, \fInorth\fP=90.
.TP
.B \fB-T\fItimegap\fP
Sets the maximum time gap in minutes between adjacent pings allowed before
the data is considered to have a gap. Default: \fItimegap\fP = 1.
.TP
.B \-U
Draw Unix System time stamp on plot.  Optionally, append a label, or 'c' which will plot
the command string.
.TP
.B \-V
Selects verbose mode, which will send progress reports to stderr [Default runs "silently"].
.TP
.B \-X \-Y
Shift origin of plot by (\fIx-shift,y-shift\fP) inches  [Default is (1,1) for new plots, (0,0) for overlays].
.TP
.B \fB-Z\fIalgorithm\fP
Sets the contouring algorithm to be used. If \fIalgorithm\fP=0,
a simple ping to ping contouring approach is used; this algorithm
is fast but produces poor looking contours when used with data
where beams from one ping may lie "behind" beams from previous
pings (this happens for sonars that ping at nonnull pitch
angles or for the "inside" beams when ships make sharp turns).
If \fIalgorithm\fP=1 then a triangular network is constructed from
the available soundings and this network is in turn contoured;
this algorithm is slow but produces good looking contours in
most cases. It is important to note that the time required for
"triangle" algorithm increases with the square of the number
of beams to be contoured; thus it is sensible to keep the number
of pings contoured at a time small (e.g. use \fB-N\fP\fI5\fP).
The time required for the "ping to ping" algorithm varies linearly
with the number of pings contoured; thus larger numbers of pings
may be reasonably contoured at a time (e.g. use \fB-N\fP\fI50\fP). 
Default: \fIalgorithm\fP = 0.
.TP
.B \-#
Specifies the number of plot copies. [Default is 1]
.SH EXAMPLES
Suppose the user has a Hydrosweep data file in the L-DEO in-house
binary format (\fBMBIO\fP format id 24) called hs_ew9302_161_mn.mb24
which lies in the region w/s/e/n = -32.1874/-26.6236/54.6349/56.7536.
The following will suffice to generate a traditional four-color contour plot:
 	mbcontour -Idatalist -Jm2.44652 -R-25.7252/-23.0683/59.7415/61.0699 
 		-Ba0.5314g0.5314 -A50.0000/250.0000/250.0000/250.0000/0.01/0.1 
 		-p1 -P -X1 -Y1 -K -V > mbcontour.ps
.br
where the file datalist contains:
 	hs_ew9302_161_mn.mb24 24
.br
A plot including a navigation track can also be created using 
\fBmbcontour\fP.  Here the \fB-D\fP flag is used to add a plot
of the ship track annotated with time marks every 0.25 hours,
annotated time marks every hour, and day annotations every four hours:
 	mbcontour -Idatalist -Jm2.44652 -R-25.7252/-23.0683/59.7415/61.0699 
 		-Ba0.5314g0.5314":.Data File hs_ew9302_161_bmn.mb24:" 
 		-D0.25/1/4/0.1 -A50.0000/250.0000/250.0000/250.0000/0.01/0.1 
 		-p1 -P -X1 -Y1 -K -V > hs_ew9302_161_bmn.mb24.ps

.SH BUGS
Probably.