/*--------------------------------------------------------------------
 *    The MB-system:	mbcopy.c	2/4/93
 *    $Id: mbcopy.c,v 4.5 1995-03-06 19:37:59 caress Exp $
 *
 *    Copyright (c) 1993, 1994 by 
 *    D. W. Caress (caress@lamont.ldgo.columbia.edu)
 *    and D. N. Chayes (dale@lamont.ldgo.columbia.edu)
 *    Lamont-Doherty Earth Observatory
 *    Palisades, NY  10964
 *
 *    See README file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/
/*
 * MBCOPY copies an input multibeam data file to an output
 * multibeam data file with the specified conversions.  Options include
 * windowing in time and space and ping averaging.  The input and
 * output data formats may differ, though not all possible combinations
 * make sense.  The default input and output streams are stdin 
 * and stdout.
 *
 * Author:	D. W. Caress
 * Date:	February 4, 1993
 *
 * $Log: not supported by cvs2svn $
 * Revision 4.4  1995/03/02  13:49:21  caress
 * Fixed bug related to error messages.
 *
 * Revision 4.3  1994/10/21  13:02:31  caress
 * Release V4.0
 *
 * Revision 4.2  1994/07/29  19:02:56  caress
 * Changes associated with supporting byte swapped Lynx OS and
 * using unix second time base.
 *
 * Revision 4.1  1994/03/12  01:44:37  caress
 * Added declarations of ctime and/or getenv for compatability
 * with SGI compilers.
 *
 * Revision 4.1  1994/03/12  01:44:37  caress
 * Added declarations of ctime and/or getenv for compatability
 * with SGI compilers.
 *
 * Revision 4.0  1994/03/06  00:13:22  caress
 * First cut at version 4.0
 *
 * Revision 4.1  1994/03/05  22:49:18  caress
 * Fixed significant bug - output arrays were allocated to
 * size of input arrays.  Also added zeroing of beam/pixel
 * values not set in copying from one system to another.
 *
 * Revision 4.0  1994/03/01  18:59:27  caress
 * First cut at new version. Any changes are associated with
 * support of three data types (beam bathymetry, beam amplitude,
 * and sidescan) instead of two (bathymetry and backscatter).
 *
 * Revision 3.1  1993/06/14  17:53:29  caress
 * Fixed stripcomments option so it does what the man page says.
 *
 * Revision 3.0  1993/05/04  22:25:09  dale
 * Initial version.
 *
 */

/* standard include files */
#include <stdio.h>
#include <math.h>
#include <string.h>

/* mbio include files */
#include "../../include/mb_status.h"
#include "../../include/mb_format.h"

/*--------------------------------------------------------------------*/

main (argc, argv)
int argc;
char **argv; 
{
	/* id variables */
	static char rcs_id[] = "$Id: mbcopy.c,v 4.5 1995-03-06 19:37:59 caress Exp $";
	static char program_name[] = "MBCOPY";
	static char help_message[] =  "MBCOPY copies an input multibeam data file to an output \nmultibeam data file with the specified conversions.  Options include \nwindowing in time and space and ping averaging.  The input and \noutput data formats may differ, though not all possible combinations \nmake sense.  The default input and output streams are stdin and stdout.";
	static char usage_message[] = "mbcopy [-Fiformat/oformat -Rw/e/s/n -Ppings -Sspeed -Llonflip\n\t-Byr/mo/da/hr/mn/sc -Eyr/mo/da/hr/mn/sc -Ccommentfile \n\t-N -V -H  -Iinfile -Ooutfile]";

	/* parsing variables */
	extern char *optarg;
	extern int optkind;
	int	errflg = 0;
	int	c;
	int	help = 0;
	int	flag = 0;

	/* MBIO status variables */
	int	status;
	int	verbose = 0;
	int	error = MB_ERROR_NO_ERROR;
	char	*message;

	/* MBIO read control parameters */
	int	iformat = 0;
	int	iformat_num;
	int	pings;
	int	lonflip;
	double	bounds[4];
	int	btime_i[7];
	int	etime_i[7];
	double	btime_d;
	double	etime_d;
	double	speedmin;
	double	timegap;
	char	ifile[128];
	int	ibeams_bath;
	int	ibeams_amp;
	int	ipixels_ss;
	char	*imbio_ptr = NULL;

	/* MBIO write control parameters */
	int	oformat = 0;
	int	oformat_num;
	char	ofile[128];
	int	obeams_bath;
	int	obeams_amp;
	int	opixels_ss;
	char	*ombio_ptr = NULL;

	/* MBIO read and write values */
	char	*store_ptr;
	int	kind;
	int	time_i[7];
	double	time_d;
	double	navlon;
	double	navlat;
	double	speed;
	double	heading;
	double	distance;
	double	*ibath = NULL;
	double	*ibathacrosstrack = NULL;
	double	*ibathalongtrack = NULL;
	double	*iamp = NULL;
	double	*iss = NULL;
	double	*issacrosstrack = NULL;
	double	*issalongtrack = NULL;
	double	*obath = NULL;
	double	*obathacrosstrack = NULL;
	double	*obathalongtrack = NULL;
	double	*oamp = NULL;
	double	*oss = NULL;
	double	*ossacrosstrack = NULL;
	double	*ossalongtrack = NULL;
	int	idata = 0;
	int	icomment = 0;
	int	odata = 0;
	int	ocomment = 0;
	int	nbath, namp, nss;
	int	istart_bath, iend_bath, offset_bath;
	int	istart_amp, iend_amp, offset_amp;
	int	istart_ss, iend_ss, offset_ss;
	char	comment[256];
	int	insertcomments = MB_NO;
	char	commentfile[256];
	int	stripcomments = MB_NO;
	int	fullcopy = MB_NO;

	/* time, user, host variables */
	long int	right_now;
	char	date[25], user[128], host[128];

	FILE	*fp;
	char	*result;
	int	format;
	int	i, j, k, l, m;

	char	*ctime();
	char	*getenv();

	/* get current default values */
	status = mb_defaults(verbose,&format,&pings,&lonflip,bounds,
		btime_i,etime_i,&speedmin,&timegap);

	/* set default input and output */
	iformat = 0;
	oformat = 0;
	strcpy (commentfile, "\0");
	strcpy (ifile, "stdin");
	strcpy (ofile, "stdout");

	/* process argument list */
	while ((c = getopt(argc, argv, "VvHhNnF:f:P:p:L:l:R:r:B:b:E:e:S:s:T:t:C:c:I:i:O:o:")) != -1)
	  switch (c) 
		{
		case 'H':
		case 'h':
			help++;
			break;
		case 'V':
		case 'v':
			verbose++;
			break;
		case 'N':
		case 'n':
			stripcomments = MB_YES;
			break;
		case 'F':
		case 'f':
			i = sscanf (optarg,"%d/%d", &iformat,&oformat);
			if (i == 1)
				oformat = iformat;
			flag++;
			break;
		case 'P':
		case 'p':
			sscanf (optarg,"%d", &pings);
			flag++;
			break;
		case 'L':
		case 'l':
			sscanf (optarg,"%d", &lonflip);
			flag++;
			break;
		case 'R':
		case 'r':
			sscanf (optarg,"%lf/%lf/%lf/%lf", 
				&bounds[0],&bounds[1],&bounds[2],&bounds[3]);
			flag++;
			break;
		case 'B':
		case 'b':
			sscanf (optarg,"%d/%d/%d/%d/%d/%d",
				&btime_i[0],&btime_i[1],&btime_i[2],
				&btime_i[3],&btime_i[4],&btime_i[5]);
			btime_i[6] = 0;
			flag++;
			break;
		case 'E':
		case 'e':
			sscanf (optarg,"%d/%d/%d/%d/%d/%d",
				&etime_i[0],&etime_i[1],&etime_i[2],
				&etime_i[3],&etime_i[4],&etime_i[5]);
			etime_i[6] = 0;
			flag++;
			break;
		case 'S':
		case 's':
			sscanf (optarg,"%lf", &speedmin);
			flag++;
			break;
		case 'T':
		case 't':
			sscanf (optarg,"%lf", &timegap);
			flag++;
			break;
		case 'C':
		case 'c':
			sscanf (optarg,"%s", commentfile);
			insertcomments = MB_YES;
			flag++;
			break;
		case 'I':
		case 'i':
			sscanf (optarg,"%s", ifile);
			flag++;
			break;
		case 'O':
		case 'o':
			sscanf (optarg,"%s", ofile);
			flag++;
			break;
		case '?':
			errflg++;
		}

	/* if error flagged then print it and exit */
	if (errflg)
		{
		fprintf(stderr,"usage: %s\n", usage_message);
		fprintf(stderr,"\nProgram <%s> Terminated\n",
			program_name);
		exit(MB_FAILURE);
		}

	/* print starting message */
	if (verbose == 1)
		{
		fprintf(stderr,"\nProgram %s\n",program_name);
		fprintf(stderr,"Version %s\n",rcs_id);
		fprintf(stderr,"MB-system Version %s\n",MB_VERSION);
		}

	/* print starting debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  Program <%s>\n",program_name);
		fprintf(stderr,"dbg2  Version %s\n",rcs_id);
		fprintf(stderr,"dbg2  MB-system Version %s\n",MB_VERSION);
		fprintf(stderr,"dbg2  Control Parameters:\n");
		fprintf(stderr,"dbg2       verbose:        %d\n",verbose);
		fprintf(stderr,"dbg2       help:           %d\n",help);
		fprintf(stderr,"dbg2       pings:          %d\n",pings);
		fprintf(stderr,"dbg2       lonflip:        %d\n",lonflip);
		fprintf(stderr,"dbg2       bounds[0]:      %f\n",bounds[0]);
		fprintf(stderr,"dbg2       bounds[1]:      %f\n",bounds[1]);
		fprintf(stderr,"dbg2       bounds[2]:      %f\n",bounds[2]);
		fprintf(stderr,"dbg2       bounds[3]:      %f\n",bounds[3]);
		fprintf(stderr,"dbg2       btime_i[0]:     %d\n",btime_i[0]);
		fprintf(stderr,"dbg2       btime_i[1]:     %d\n",btime_i[1]);
		fprintf(stderr,"dbg2       btime_i[2]:     %d\n",btime_i[2]);
		fprintf(stderr,"dbg2       btime_i[3]:     %d\n",btime_i[3]);
		fprintf(stderr,"dbg2       btime_i[4]:     %d\n",btime_i[4]);
		fprintf(stderr,"dbg2       btime_i[5]:     %d\n",btime_i[5]);
		fprintf(stderr,"dbg2       btime_i[6]:     %d\n",btime_i[6]);
		fprintf(stderr,"dbg2       etime_i[0]:     %d\n",etime_i[0]);
		fprintf(stderr,"dbg2       etime_i[1]:     %d\n",etime_i[1]);
		fprintf(stderr,"dbg2       etime_i[2]:     %d\n",etime_i[2]);
		fprintf(stderr,"dbg2       etime_i[3]:     %d\n",etime_i[3]);
		fprintf(stderr,"dbg2       etime_i[4]:     %d\n",etime_i[4]);
		fprintf(stderr,"dbg2       etime_i[5]:     %d\n",etime_i[5]);
		fprintf(stderr,"dbg2       etime_i[6]:     %d\n",etime_i[6]);
		fprintf(stderr,"dbg2       speedmin:       %f\n",speedmin);
		fprintf(stderr,"dbg2       timegap:        %f\n",timegap);
		fprintf(stderr,"dbg2       input format:   %d\n",iformat);
		fprintf(stderr,"dbg2       output format:  %d\n",oformat);
		fprintf(stderr,"dbg2       input file:     %s\n",ifile);
		fprintf(stderr,"dbg2       output file:    %s\n",ofile);
		fprintf(stderr,"dbg2       insert comments:%d\n",insertcomments);
		fprintf(stderr,"dbg2       comment file:   %s\n",commentfile);
		fprintf(stderr,"dbg2       strip comments: %d\n",stripcomments);		fprintf(stderr,"dbg2       strip comments: %d\n",stripcomments);

		}

	/* if help desired then print it and exit */
	if (help)
		{
		fprintf(stderr,"\n%s\n",help_message);
		fprintf(stderr,"\nusage: %s\n", usage_message);
		exit(MB_ERROR_NO_ERROR);
		}

	/* settle the input/output formats */
	if (iformat <= 0 && oformat <= 0)
		{
		iformat = format;
		oformat = format;
		}
	else if (iformat > 0 && oformat <= 0)
		oformat = iformat;

	/* obtain format array locations - format ids will 
		be aliased to current ids if old format ids given */
	if ((status = mb_format(verbose,&iformat,&iformat_num,&error)) 
		!= MB_SUCCESS)
		{
		mb_error(verbose,error,&message);
		fprintf(stderr,"\nMBIO Error returned from function <mb_format> regarding input format %d:\n%s\n",iformat,message);
		fprintf(stderr,"\nProgram <%s> Terminated\n",
			program_name);
		exit(error);
		}
	if ((status = mb_format(verbose,&oformat,&oformat_num,&error)) 
		!= MB_SUCCESS)
		{
		mb_error(verbose,error,&message);
		fprintf(stderr,"\nMBIO Error returned from function <mb_format> regarding output format %d:\n%s\n",oformat,message);
		fprintf(stderr,"\nProgram <%s> Terminated\n",
			program_name);
		exit(error);
		}

	/* determine if full or partial copies will be made */
	if (pings == 1 
		&& mb_system_table[iformat_num] != MB_SYS_NONE 
		&& mb_system_table[iformat_num] == mb_system_table[oformat_num])
		fullcopy = MB_YES;
	else
		fullcopy = MB_NO;

	/* print debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  Copy mode set in program <%s>\n",
			program_name);
		fprintf(stderr,"dbg2       pings:         %d\n",pings);
		fprintf(stderr,"dbg2       iformat:       %d\n",iformat);
		fprintf(stderr,"dbg2       oformat:       %d\n",oformat);
		fprintf(stderr,"dbg2       isystem:       %d\n",
			mb_system_table[iformat]);
		fprintf(stderr,"dbg2       osystem:       %d\n",
			mb_system_table[oformat]);
		fprintf(stderr,"dbg2       fullcopy:      %d\n",fullcopy);
			}

	/* initialize reading the input multibeam file */
	if ((status = mb_read_init(
		verbose,ifile,iformat,pings,lonflip,bounds,
		btime_i,etime_i,speedmin,timegap,
		&imbio_ptr,&btime_d,&etime_d,
		&ibeams_bath,&ibeams_amp,&ipixels_ss,&error)) != MB_SUCCESS)
		{
		mb_error(verbose,error,&message);
		fprintf(stderr,"\nMBIO Error returned from function <mb_read_init>:\n%s\n",message);
		fprintf(stderr,"\nMultibeam File <%s> not initialized for reading\n",ifile);
		fprintf(stderr,"\nProgram <%s> Terminated\n",
			program_name);
		exit(error);
		}

	/* initialize writing the output multibeam file */
	if ((status = mb_write_init(
		verbose,ofile,oformat,&ombio_ptr,
		&obeams_bath,&obeams_amp,&opixels_ss,&error)) != MB_SUCCESS)
		{
		mb_error(verbose,error,&message);
		fprintf(stderr,"\nMBIO Error returned from function <mb_write_init>:\n%s\n",message);
		fprintf(stderr,"\nMultibeam File <%s> not initialized for writing\n",ofile);
		fprintf(stderr,"\nProgram <%s> Terminated\n",
			program_name);
		exit(error);
		}

	/* allocate memory for data arrays */
	status = mb_malloc(verbose,ibeams_bath*sizeof(double),&ibath,&error);
	status = mb_malloc(verbose,ibeams_bath*sizeof(double),&ibathacrosstrack,
				&error);
	status = mb_malloc(verbose,ibeams_bath*sizeof(double),&ibathalongtrack,
				&error);
	status = mb_malloc(verbose,ibeams_amp*sizeof(double),&iamp,&error);
	status = mb_malloc(verbose,ipixels_ss*sizeof(double),&iss,&error);
	status = mb_malloc(verbose,ipixels_ss*sizeof(double),&issacrosstrack,
				&error);
	status = mb_malloc(verbose,ipixels_ss*sizeof(double),&issalongtrack,
				&error);
	status = mb_malloc(verbose,obeams_bath*sizeof(double),&obath,&error);
	status = mb_malloc(verbose,obeams_bath*sizeof(double),&obathacrosstrack,
				&error);
	status = mb_malloc(verbose,obeams_bath*sizeof(double),&obathalongtrack,
				&error);
	status = mb_malloc(verbose,obeams_amp*sizeof(double),&oamp,&error);
	status = mb_malloc(verbose,opixels_ss*sizeof(double),&oss,&error);
	status = mb_malloc(verbose,opixels_ss*sizeof(double),&ossacrosstrack,
				&error);
	status = mb_malloc(verbose,opixels_ss*sizeof(double),&ossalongtrack,
				&error);

	/* if error initializing memory then quit */
	if (error != MB_ERROR_NO_ERROR)
		{
		mb_error(verbose,error,&message);
		fprintf(stderr,"\nMBIO Error allocating data arrays:\n%s\n",message);
		fprintf(stderr,"\nProgram <%s> Terminated\n",
			program_name);
		exit(error);
		}

	/* set up transfer rules */
	if (variable_beams_table[oformat_num] == MB_YES
		&& obeams_bath != ibeams_bath)
		obeams_bath = ibeams_bath;
	if (variable_beams_table[oformat_num] == MB_YES
		&& obeams_amp != ibeams_amp)
		obeams_amp = ibeams_amp;
	if (variable_beams_table[oformat_num] == MB_YES
		&& opixels_ss != ipixels_ss)
		opixels_ss = ipixels_ss;
	setup_transfer_rules(verbose,ibeams_bath,obeams_bath,
		&istart_bath,&iend_bath,&offset_bath,&error);
	setup_transfer_rules(verbose,ibeams_amp,obeams_amp,
		&istart_amp,&iend_amp,&offset_amp,&error);
	setup_transfer_rules(verbose,ipixels_ss,opixels_ss,
		&istart_ss,&iend_ss,&offset_ss,&error);

	/* insert comments from file into output */
	if (insertcomments == MB_YES)
		{
		/* open file */
		if ((fp = fopen(commentfile, "r")) == NULL) 
			{
			error = MB_ERROR_OPEN_FAIL;
			fprintf(stderr,"\nUnable to Open Comment File <%s> for reading\n",commentfile);
			fprintf(stderr,"\nProgram <%s> Terminated\n",
				program_name);
			exit(error);
			}

		/* read and output comment lines */
		strncpy(comment,"\0",256);
		while ((result = fgets(comment,256,fp)) == comment)
			{
			kind = MB_DATA_COMMENT;
			comment[(int)strlen(comment)-1] = '\0';
			status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
			if (error == MB_ERROR_NO_ERROR) ocomment++;
			}

		/* close the file */
		fclose(fp);
		}

	/* write comments to beginning of output file */
	if (stripcomments == MB_NO)
		{
		kind = MB_DATA_COMMENT;
		strncpy(comment,"\0",256);
		sprintf(comment,"This data copied by program %s version %s",
			program_name,rcs_id);
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		strncpy(comment,"\0",256);
		sprintf(comment,"MB-system Version %s",MB_VERSION);
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		right_now = time((long *)0);
		strncpy(date,"\0",25);
		right_now = time((long *)0);
		strncpy(date,ctime(&right_now),24);
		strcpy(user,getenv("USER"));
		gethostname(host,128);
		strncpy(comment,"\0",256);
		sprintf(comment,"Run by user <%s> on cpu <%s> at <%s>",
			user,host,date);
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		strncpy(comment,"\0",256);
		sprintf(comment,"Control Parameters:");
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		strncpy(comment,"\0",256);
		sprintf(comment,"  Input file:         %s",ifile);
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		strncpy(comment,"\0",256);
		sprintf(comment,"  Input MBIO format:  %d",iformat);
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		strncpy(comment,"\0",256);
		sprintf(comment,"  Output file:        %s",ofile);
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		strncpy(comment,"\0",256);
		sprintf(comment,"  Output MBIO format: %d",oformat);
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		strncpy(comment,"\0",256);
		sprintf(comment,"  Ping averaging:     %d",pings);
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		strncpy(comment,"\0",256);
		sprintf(comment,"  Longitude flip:     %d",lonflip);
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		strncpy(comment,"\0",256);
		sprintf(comment,"  Longitude bounds:   %f %f",
			bounds[0],bounds[1]);
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		strncpy(comment,"\0",256);
		sprintf(comment,"  Latitude bounds:    %f %f",
			bounds[2],bounds[3]);
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		strncpy(comment,"\0",256);
		sprintf(comment,"  Begin time:         %d %d %d %d %d %d %d",
			btime_i[0],btime_i[1],btime_i[2],
			btime_i[3],btime_i[4],btime_i[5],btime_i[6]);
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		strncpy(comment,"\0",256);
		sprintf(comment,"  End time:           %d %d %d %d %d %d %d",
			etime_i[0],etime_i[1],etime_i[2],
			etime_i[3],etime_i[4],etime_i[5],etime_i[6]);
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		strncpy(comment,"\0",256);
		sprintf(comment,"  Minimum speed:      %f",speedmin);
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		strncpy(comment,"\0",256);
		sprintf(comment,"  Time gap:           %f",timegap);
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		strncpy(comment,"\0",256);
		sprintf(comment," ");
		status = mb_put(verbose,ombio_ptr,kind,
				time_i,time_d,
				navlon,navlat,speed,heading,
				obeams_bath,obeams_amp,opixels_ss,
				obath,oamp,obathacrosstrack,obathalongtrack,
				oss,ossacrosstrack,ossalongtrack,
				comment,&error);
		if (error == MB_ERROR_NO_ERROR) ocomment++;
		}

	/* read and write */
	while (error <= MB_ERROR_NO_ERROR)
		{
		/* read some data */
		error = MB_ERROR_NO_ERROR;
		status = MB_SUCCESS;
		if (fullcopy == MB_YES)
			status = mb_get_all(verbose,imbio_ptr,&store_ptr,&kind,
				time_i,&time_d,&navlon,&navlat,&speed,
				&heading,&distance,
				&nbath,&namp,&nss,
				ibath,iamp,ibathacrosstrack,ibathalongtrack,
				iss,issacrosstrack,issalongtrack,
				comment,&error);
		else
			status = mb_get(verbose,imbio_ptr,&kind,&pings,
				time_i,&time_d,&navlon,&navlat,&speed,
				&heading,&distance,
				&nbath,&namp,&nss,
				ibath,iamp,ibathacrosstrack,ibathalongtrack,
				iss,issacrosstrack,issalongtrack,
				comment,&error);

		/* increment counter */
		if (error <= MB_ERROR_NO_ERROR 
			&& kind == MB_DATA_DATA)
			idata = idata + pings;
		else if (error <= MB_ERROR_NO_ERROR 
			&& kind == MB_DATA_COMMENT)
			icomment++;

		/* check numbers of input and output beams */
		if (fullcopy == MB_NO
			&& kind == MB_DATA_DATA
			&& error == MB_ERROR_NO_ERROR
			&& nbath != ibeams_bath)
			{
			ibeams_bath = nbath;
			if (variable_beams_table[oformat_num] == MB_YES)
				obeams_bath = ibeams_bath;
			setup_transfer_rules(verbose,ibeams_bath,obeams_bath,
				&istart_bath,&iend_bath,&offset_bath,&error);
			}
		if (fullcopy == MB_NO
			&& kind == MB_DATA_DATA
			&& error == MB_ERROR_NO_ERROR
			&& namp != ibeams_amp)
			{
			ibeams_amp = namp;
			if (variable_beams_table[oformat_num] == MB_YES)
				obeams_amp = ibeams_amp;
			setup_transfer_rules(verbose,ibeams_amp,obeams_amp,
				&istart_amp,&iend_amp,&offset_amp,&error);
			}
		if (fullcopy == MB_NO
			&& kind == MB_DATA_DATA
			&& error == MB_ERROR_NO_ERROR
			&& nss != ipixels_ss)
			{
			ipixels_ss = nss;
			if (variable_beams_table[oformat_num] == MB_YES)
				opixels_ss = ipixels_ss;
			setup_transfer_rules(verbose,ipixels_ss,opixels_ss,
				&istart_ss,&iend_ss,&offset_ss,&error);
			}
			
		/* time gaps do not matter to mbcopy */
		if (error == MB_ERROR_TIME_GAP)
			{
			status = MB_SUCCESS;
			error = MB_ERROR_NO_ERROR;
			}

		/* output error messages */
		if (verbose >= 1 && error == MB_ERROR_COMMENT)
			{
			if (icomment == 1)
				fprintf(stderr,"\nComments:\n");
			fprintf(stderr,"%s\n",comment);
			}
		else if (verbose >= 1 && error < MB_ERROR_NO_ERROR
			&& error >= MB_ERROR_OTHER)
			{
			mb_error(verbose,error,&message);
			fprintf(stderr,"\nNonfatal MBIO Error:\n%s\n",message);
			fprintf(stderr,"Input Record: %d\n",idata);
			fprintf(stderr,"Time: %d %d %d %d %d %d %d\n",
				time_i[0],time_i[1],time_i[2],
				time_i[3],time_i[4],time_i[5],time_i[6]);
			}
		else if (verbose >= 1 && error < MB_ERROR_NO_ERROR)
			{
			mb_error(verbose,error,&message);
			fprintf(stderr,"\nNonfatal MBIO Error:\n%s\n",message);
			fprintf(stderr,"Number of good records so far: %d\n",idata);
			}
		else if (verbose >= 1 && error != MB_ERROR_NO_ERROR 
			&& error != MB_ERROR_EOF)
			{
			mb_error(verbose,error,&message);
			fprintf(stderr,"\nFatal MBIO Error:\n%s\n",message);
			fprintf(stderr,"Last Good Time: %d %d %d %d %d %d %d\n",
				time_i[0],time_i[1],time_i[2],
				time_i[3],time_i[4],time_i[5],time_i[6]);
			}

		/* process some data */
		if (fullcopy == MB_NO
			&& kind == MB_DATA_DATA
			&& error == MB_ERROR_NO_ERROR)
			{
			/* do bathymetry */
			for (j=0;j<offset_bath;j++)
				{
				obath[j] = 0.0;
				obathacrosstrack[j] = 0.0;
				obathalongtrack[j] = 0.0;
				}
			for (i=istart_bath;i<iend_bath;i++)
				{
				j = i + offset_bath;
				obath[j] = ibath[i];
				obathacrosstrack[j] = ibathacrosstrack[i];
				obathalongtrack[j] = ibathalongtrack[i];
				}
			for (j=iend_bath+offset_bath;j<obeams_bath;j++)
				{
				obath[j] = 0.0;
				obathacrosstrack[j] = 0.0;
				obathalongtrack[j] = 0.0;
				}

			/* do amplitudes */
			for (j=0;j<offset_amp;j++)
				{
				oamp[j] = 0.0;
				}
			for (i=istart_amp;i<iend_amp;i++)
				{
				j = i + offset_amp;
				oamp[j] = iamp[i];
				}
			for (j=iend_amp+offset_amp;j<obeams_amp;j++)
				{
				oamp[j] = 0.0;
				}

			/* do sidescan */
			for (j=0;j<offset_ss;j++)
				{
				oss[j] = 0.0;
				ossacrosstrack[j] = 0.0;
				ossalongtrack[j] = 0.0;
				}
			for (i=istart_ss;i<iend_ss;i++)
				{
				j = i + offset_ss;
				oss[j] = iss[i];
				ossacrosstrack[j] = issacrosstrack[i];
				ossalongtrack[j] = issalongtrack[i];
				}
			for (j=iend_ss+offset_ss;j<opixels_ss;j++)
				{
				oss[j] = 0.0;
				ossacrosstrack[j] = 0.0;
				ossalongtrack[j] = 0.0;
				}
			}

		/* write some data */
		if ((error == MB_ERROR_NO_ERROR && kind != MB_DATA_COMMENT)
			|| (kind == MB_DATA_COMMENT && stripcomments == MB_NO))
			{
			if (fullcopy == MB_YES)
				status = mb_put_all(verbose,ombio_ptr,
					store_ptr,MB_NO,kind,
					time_i,time_d,
					navlon,navlat,speed,heading,
					obeams_bath,obeams_amp,opixels_ss,
					obath,oamp,obathacrosstrack,
					obathalongtrack,
					oss,ossacrosstrack,ossalongtrack,
					comment,&error);
			else
				status = mb_put(verbose,ombio_ptr,kind,
					time_i,time_d,
					navlon,navlat,speed,heading,
					obeams_bath,obeams_amp,opixels_ss,
					obath,oamp,obathacrosstrack,
					obathalongtrack,
					oss,ossacrosstrack,ossalongtrack,
					comment,&error);
			if (status == MB_SUCCESS)
				{
				if (kind == MB_DATA_DATA)
					odata++;
				else if (kind == MB_DATA_COMMENT)
					ocomment++;
				}
			else
				{
				mb_error(verbose,error,&message);
				fprintf(stderr,"\nMBIO Error returned from function <mb_put>:\n%s\n",message);
				fprintf(stderr,"\nMultibeam Data Not Written To File <%s>\n",ofile);
				fprintf(stderr,"Output Record: %d\n",odata+1);
				fprintf(stderr,"Time: %d %d %d %d %d %d %d\n",
					time_i[0],time_i[1],time_i[2],
					time_i[3],time_i[4],time_i[5],
					time_i[6]);
				fprintf(stderr,"\nProgram <%s> Terminated\n",
					program_name);
				exit(error);
				}
			}
		}

	/* close the files */
	status = mb_close(verbose,&imbio_ptr,&error);
	status = mb_close(verbose,&ombio_ptr,&error);

	/* deallocate memory for data arrays */
	mb_free(verbose,&ibath,&error); 
	mb_free(verbose,&ibathacrosstrack,&error); 
	mb_free(verbose,&ibathalongtrack,&error); 
	mb_free(verbose,&iamp,&error); 
	mb_free(verbose,&iss,&error); 
	mb_free(verbose,&issacrosstrack,&error); 
	mb_free(verbose,&issalongtrack,&error); 
	mb_free(verbose,&obath,&error); 
	mb_free(verbose,&obathacrosstrack,&error); 
	mb_free(verbose,&obathalongtrack,&error); 
	mb_free(verbose,&oamp,&error); 
	mb_free(verbose,&oss,&error); 
	mb_free(verbose,&ossacrosstrack,&error); 
	mb_free(verbose,&ossalongtrack,&error); 

	/* check memory */
	if (verbose >= 4)
		status = mb_memory_list(verbose,&error);

	/* give the statistics */
	if (verbose >= 1)
		{
		fprintf(stderr,"\n%d input data records\n",idata);
		fprintf(stderr,"%d input comment records\n",icomment);
		fprintf(stderr,"%d output data records\n",odata);
		fprintf(stderr,"%d output comment records\n",ocomment);
		}

	/* end it all */
	exit(status);
}
/*--------------------------------------------------------------------*/
int setup_transfer_rules(verbose,ibeams,obeams,istart,iend,offset,error)
int	verbose;
int	ibeams;
int	obeams;
int	*istart;
int	*iend;
int	*offset;
int	*error;
{
	char	*function_name = "setup_transfer_rules";
	int	status = MB_SUCCESS;

	/* print input debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBCOPY function <%s> called\n",
			function_name);
		fprintf(stderr,"dbg2  Input arguments:\n");
		fprintf(stderr,"dbg2       verbose:    %d\n",verbose);
		fprintf(stderr,"dbg2       ibeams:     %d\n",ibeams);
		fprintf(stderr,"dbg2       obeams:     %d\n",obeams);
		}

	/* set up transfer rules */
	if (ibeams == obeams)
		{
		*istart = 0;
		*iend = ibeams;
		*offset = 0;
		}
	else if (ibeams < obeams)
		{
		*istart = 0;
		*iend = ibeams;
		*offset = obeams/2 - ibeams/2;
		}
	else if (ibeams > obeams)
		{
		*istart = ibeams/2 - obeams/2;
		*iend = *istart + obeams;
		*offset = -*istart;
		}

	/* assume success */
	*error = MB_ERROR_NO_ERROR;
	status = MB_SUCCESS;

	/* print output debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBCOPY function <%s> completed\n",
			function_name);
		fprintf(stderr,"dbg2  Return values:\n");
		fprintf(stderr,"dbg2       error:      %d\n",*error);
		fprintf(stderr,"dbg2       istart:     %d\n",*istart);
		fprintf(stderr,"dbg2       iend:       %d\n",*iend);
		fprintf(stderr,"dbg2       offset:     %d\n",*offset);
		fprintf(stderr,"dbg2  Return status:\n");
		fprintf(stderr,"dbg2       status:     %d\n",status);
		}

	/* return status */
	return(status);
}
/*--------------------------------------------------------------------*/
