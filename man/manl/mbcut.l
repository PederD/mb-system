.TH mbcut l "14 February 1995"
.SH NAME
\fBmbcut\fP - Removes data in specified regions of a swath,
defined either in terms of acrosstrack distance or 
beam or pixel numbers.

.SH VERSION
Version 4.2

.SH SYNOPSIS
\fBmbcut\fP [\fB-A\fIkind/mode/min/max\fP \fB-B\fIyr/mo/da/hr/mn/sc 
\fB-E\fIyr/mo/da/hr/mn/sc 
\fB-F\fIformat\fP \fB-I\fIinfile\fP
\fB-L\fIlonflip\fP \fB-O\fIoutfile\fP \fB-R\fIwest/east/south/north\fP 
\fB-S\fIspeed\fP 
\fB-V -H\fP]

.SH DESCRIPTION
\fBmbcut\fP removes swath data values that lie in
user-specified ranges of beam numbers, pixel numbers, 
or acrosstrack distances.
Multiple cut ranges can be specified. The "cut" data is 
flagged rather than zeroed whenever possible (some formats
do not support flagging of some data types).
The default input and output streams are stdin and stdout.

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
.B \fB-A\fIkind/mode/min/max\fP
Sets a range of data to be cut; multiple calls of this option
may be made. The \fIkind\fP value indicates whether the
affected data is bathymetry (\fIkind\fP = 0), amplitude 
(\fIkind\fP = 1), or sidescan (\fIkind\fP = 2).
The \fImode\fP indicates whether the cut range is specified
in terms of beam/pixel number (\fImode\fP = 1) or
acrosstrack distance (\fImode\fP = 2). The \fImin\fP
and \fImax\fP values specify the actual cut range.
.TP
.B \fB-B\fIyr/mo/da/hr/mn/sc\fP
Sets the starting time for data allowed in the input data; pings
with times before the starting time will be ignored. 
Default: \fIyr/mo/da/hr/mn/sc\fP = 1962/2/21/10/30/0.
.TP
.B \fB-E\fIyr/mo/da/hr/mn/sc\fP
Sets the ending time for data allowed in the input data; pings
with times after the ending time will be ignored. 
Default: \fIyr/mo/da/hr/mn/sc\fP = 2062/2/21/10/30/0.
.TP
.B \fB-F\fIformat\fP
Sets the data format used in reading the input from stdin
or from a file. This program uses the \fBMBIO\fP library 
and will read or write any multibeam
format supported by \fBMBIO\fP. A list of the multibeam data formats
currently supported by \fBMBIO\fP and their identifier values
is given in the \fBMBIO\fP manual page. Default: \fIformat\fP = 11.
.TP
.B \fB-H\fP
This "help" flag causes the program to print out a description
of its operation and then exit immediately.
.TP
.B \fB-I\fIinfile\fP
Sets the input multibeam data filename. 
This program uses the \fBMBIO\fP library and will read or write any multibeam
format supported by \fBMBIO\fP. A list of the multibeam data formats
currently supported by \fBMBIO\fP and their identifier values
is given in the \fBMBIO\fP manual page. Default: \fIinfile\fP = "stdin".
.TP
.B \fB-O\fIoutfile\fP
Data file to which the output data will be written. The \fBMBIO\fP
format id used is the same as for the input data. If
no output file is specified, the output will be written to
stdout. Default: \fIoutfile\fP = stdout.
.TP
.B \fB-R\fIwest/east/south/north\fP
Sets the longitude and latitude bounds within which multibeam 
data will be read. Only the data which lies within these bounds will
be copied. 
Default: \fIwest\fP=-360, east\fI=360\fP, \fIsouth\fP=-90, \fInorth\fP=90.
.TP
.B \fB-S\fIspeed\fP
Sets the minimum speed in km/hr (5.5 kts ~ 10 km/hr) allowed in 
the input data; pings associated with a smaller ship speed will not be
processed. Default: \fIspeed\fP = 0.
.TP
.B \fB-V\fP
Normally, \fBmbcut\fP works "silently" without outputting
anything to the stderr stream.  If the
\fB-V\fP flag is given, then \fBmbcut\fP works in a "verbose" 
mode and outputs the program version being used, the values
of some important control parameters, and 
all error status messages.

.SH EXAMPLES
Suppose one has a SeaBeam 2100 data file called test.mb41
which contains bathymetry (121 beams in a 120 degree swath), 
beam amplitude (121 beams coincident with bathymetry),
and sidescan data (2000 pixels, roughly a 150 degree swath).
The specular region of the sidescan swath (the center of the
swath) often causes problems when applying simple filters
to the sidescan. To remove the specular region prior to
the use of \fBmbfilter\fP, use \fBmbcut\fP as follows:
 	mbcut -F41 -Itest.mb41 -Otestcut.mb41 -A2/2/-500/500 -V
.br
This will remove one kilometer of data from the center of
the swath.

.SH SEE ALSO
\fBmbsystem\fP(l), \fBmbfilter\fP(l)

.SH BUGS
No one really likes to throw away data.