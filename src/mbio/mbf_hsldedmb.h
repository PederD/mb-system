/*--------------------------------------------------------------------
 *    The MB-system:	mbf_hsldedmb.h	3.00	1/20/93
 *	$Id: mbf_hsldedmb.h,v 3.0 1993-05-14 22:51:24 sohara Exp $
 *
 *    Copyright (c) 1993 by 
 *    D. W. Caress (caress@lamont.ldgo.columbia.edu)
 *    and D. N. Chayes (dale@lamont.ldgo.columbia.edu)
 *    Lamont-Doherty Earth Observatory
 *    Palisades, NY  10964
 *
 *    See README file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/
/*
 * mbf_hsldedmb.h defines the data structure used by MBIO functions
 * to store multibeam data read from the  MBF_HSLDEDMB format (MBIO id 5).  
 *
 * Author:	D. W. Caress
 * Date:	January 20, 1993
 * $Log: not supported by cvs2svn $
 */
/*
 * Notes on the MBF_HSLDEDMB data format:
 *   1. This data format is used to store 59 beam Hydrosweep DS bathymetry
 *      data.  This format was created by Dale Chayes of L-DEO and Dan
 *      Chayes of NRL for use with an early version of a ping editor. 
 *      Most data files in this format consist of Hydrosweep DS data 
 *      collected on the R/V Maurice Ewing or the R/V Thomas Thompson.
 *   2. The data consist of 328 byte records including 1-byte characters,
 *      2-byte integers, and 8-byte integers.
 *   3. The 59 depth values are stored centered in 59 value arrays.  The
 *      center beam is in word 30 of the depth and distance arrays.
 *   4. There is no provision for embedding comments in the data.
 *   5. We expect the use of this format to be superceded by format
 *      MBF_MBLDEOIH.
 *
 * The kind value in the mbf_hsldedmb_struct should always be 1, indicating
 * that the mbf_hsldedmb_data_struct structure holds data; if this format
 * allowed ascii comments then kind = 2 would indicate the presence of
 * a comment.
 *
 * The mbf_hsldedmb_data_struct structure is a direct representation  
 * of the binary data structure used in the MBF_HSLDEDMB format.
 */

struct mbf_hsldedmb_data_struct
	{ 
	unsigned long	seconds;	/* seconds since 1/1/70 00:00:00 */
	unsigned long	microseconds;	/* microseconds */
	unsigned long	alt_seconds;	/* seconds since last survey header */
	unsigned long	alt_microseconds;	/* microseconds */
	long	lat;		/* latitude in degrees times 10000000 */
	long	lon;		/* latitude in degrees times 10000000 */
	short	heading;	/* heading in degrees times 10 */
	short	course;		/* course in degrees times 10 */
	short	speed;		/* speed in m/s times 10 */
	short	pitch;		/* pitch in degrees times 10 */
	short	scale;		/* multiplicative scale times 100 for depth 
					and range values */
	short	depth[59];	/* depths in scaled meters assuming 
					1500 m/s water velocity */
	short	range[59];	/* cross track distances in meters */
	unsigned char	speed_ref;	/* speed reference 
					("B": bottom track) */
	unsigned char	quality;	/* quality flag */
	unsigned long	flag[4];
	};

struct mbf_hsldedmb_struct
	{ 
	int	kind;
	struct mbf_hsldedmb_data_struct data;
	};
