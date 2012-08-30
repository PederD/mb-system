#     The MB-system:	Makefile.template	5/24/94
#     $Id$
#
#     Copyright (c) 1993-2011 by 
#     David W. Caress (caress@mbari.org)
#       Monterey Bay Aquarium Research Institute
#       Moss Landing, CA 95039
#     and D. N. Chayes (dale@ldeo.columbia.edu)
#       Lamont-Doherty Earth Observatory
#       Palisades, NY  10964
#
#     See README file for copying and redistribution conditions.
#
# Makefile template for MB-system at the top directory level
# Author:	D. W. Caress
# Date:		May 24, 1994

# $Log: Makefile.template,v $
# Revision 5.1  2006/01/11 07:22:23  caress
# Working towards 5.0.8
#
# Revision 5.0  2003/07/27 19:19:01  caress
# Release 5.0.0
#
# Revision 4.6  2001/04/10 00:36:02  caress
# Stopped cleaning html.
#
# Revision 4.5  2000/09/30  07:25:08  caress
# Snapshot for Dale.
#
# Revision 4.4  2000/09/30  05:06:44  caress
# Snapshot for Dale.
#
# Revision 4.3  1994/11/29  01:39:03  caress
# Added "-" to make clean calls so rm failures
# are ignored on SGI's.
#
# Revision 4.2  1994/11/01  15:04:00  caress
# Updates.
#
# Revision 4.1  1994/10/31  14:27:40  caress
# Changed man pages from section 1 to l
#
# Revision 4.0  1994/05/24  18:33:14  caress
# First cut.
#
#
#

all:
	cd src; make all

clean:
	- cd bin; rm -f *
	- cd lib; rm -f *
	- cd include; rm -f *
	- cd src; make clean

