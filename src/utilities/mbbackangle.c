/*--------------------------------------------------------------------
 *    The MB-system:	mbbackangle.c	1/6/95
 *    $Id: mbbackangle.c,v 5.6 2002-09-19 00:28:12 caress Exp $
 *
 *    Copyright (c) 1995, 2000, 2002 by
 *    David W. Caress (caress@mbari.org)
 *      Monterey Bay Aquarium Research Institute
 *      Moss Landing, CA 95039
 *    and Dale N. Chayes (dale@ldeo.columbia.edu)
 *      Lamont-Doherty Earth Observatory
 *      Palisades, NY 10964
 *
 *    See README file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/
/*
 * MBBACKANGLE reads a swath sonar data file and generates a table
 * of the average amplitude or sidescan values as a function of
 * the grazing angle with the seafloor. If bathymetry is
 * not available,  the seafloor is assumed to be flat. The takeoff
 * angle for each beam or pixel arrival is projected to the seafloor;
 * no raytracing is done.
 * The results are dumped to stdout.
 *
 * Author:	D. W. Caress
 * Date:	January 6, 1995
 *
 * $Log: not supported by cvs2svn $
 * Revision 5.5  2002/09/07 04:49:23  caress
 * Added slope mode option to mb_process.
 *
 * Revision 5.4  2002/07/25 19:07:17  caress
 * Release 5.0.beta21
 *
 * Revision 5.3  2002/07/20 20:56:55  caress
 * Release 5.0.beta20
 *
 * Revision 5.2  2001/07/20 00:34:38  caress
 * Release 5.0.beta03
 *
 * Revision 5.1  2001/03/22 21:14:16  caress
 * Trying to make release 5.0.beta0.
 *
 * Revision 5.0  2000/12/01  22:57:08  caress
 * First cut at Version 5.0.
 *
 * Revision 4.12  2000/10/11  01:06:15  caress
 * Convert to ANSI C
 *
 * Revision 4.11  2000/09/30  07:06:28  caress
 * Snapshot for Dale.
 *
 * Revision 4.10  2000/09/11  20:10:02  caress
 * Linked to new datalist parsing functions. Now supports recursive datalists
 * and comments in datalists.
 *
 * Revision 4.9  2000/06/06  20:32:46  caress
 * Now handles amplitude flagging using beamflags.
 *
 * Revision 4.8  1998/10/05  19:19:24  caress
 * MB-System version 4.6beta
 *
 * Revision 4.7  1997/07/25  14:28:10  caress
 * Version 4.5beta2
 *
 * Revision 4.6  1997/04/21  17:19:14  caress
 * MB-System 4.5 Beta Release.
 *
 * Revision 4.5  1996/04/22  13:23:05  caress
 * Now have DTR and MIN/MAX defines in mb_define.h
 *
 * Revision 4.5  1996/04/22  13:23:05  caress
 * Now have DTR and MIN/MAX defines in mb_define.h
 *
 * Revision 4.4  1995/05/12  17:12:32  caress
 * Made exit status values consistent with Unix convention.
 * 0: ok  nonzero: error
 *
 * Revision 4.3  1995/03/06  19:37:59  caress
 * Changed include strings.h to string.h for POSIX compliance.
 *
 * Revision 4.2  1995/03/02  13:49:21  caress
 * Fixed bug related to error messages.
 *
 * Revision 4.1  1995/02/27  14:43:18  caress
 * Fixed bug regarding closing a text input file.
 *
 * Revision 4.0  1995/02/14  21:17:15  caress
 * Version 4.2
 *
 */

/* standard include files */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/* MBIO include files */
#include "../../include/mb_status.h"
#include "../../include/mb_define.h"
#include "../../include/mb_format.h"
#include "../../include/mb_process.h"

/* mode defines */
#define	MBBACKANGLE_AMP	1
#define	MBBACKANGLE_SS	2

int get_bathyslope(int verbose,
	int ndepths, double *depths, double *depthacrosstrack, 
	int nslopes, double *slopes, double *slopeacrosstrack, 
	double acrosstrack, double *depth,  double *slope, 
	int *error);
int set_bathyslope(int verbose,
	int nbath, char *beamflag, double *bath, double *bathacrosstrack,
	int *ndepths, double *depths, double *depthacrosstrack, 
	int *nslopes, double *slopes, double *slopeacrosstrack, 
	int *error);
int output_table(int verbose, FILE *tfp, int ntable, int nping, double time_d,
	int nangles, double angle_max, double dangle, int symmetry,
	int *nmean, double *mean, double *sigma, 
	int *error);

static char rcs_id[] = "$Id: mbbackangle.c,v 5.6 2002-09-19 00:28:12 caress Exp $";

/*--------------------------------------------------------------------*/

main (int argc, char **argv)
{
	static char program_name[] = "mbbackangle";
	static char help_message[] =  
"MBbackangle reads a swath sonar data file and generates \n\t\
a set of tables containing the average sidescan values\n\t\
as a function of the angle of interaction (grazing angle) \n\t\
with the seafloor. Each table represents the symmetrical \n\t\
average function for a user defined number of pings. The \n\t\
tables are output to an \".aga\" file which can be applied \n\t\
by MBprocess.";
	static char usage_message[] = "mbbackangle -Ifile \
[-Akind \
-Fformat -Nnangles/angle_max -Ppings -Zaltitude -V -H]";
	extern char *optarg;
	extern int optkind;
	int	errflg = 0;
	int	c;
	int	help = 0;
	int	flag = 0;

	/* MBIO status variables */
	int	status = MB_SUCCESS;
	int	verbose = 0;
	int	error = MB_ERROR_NO_ERROR;
	char	*message;

	/* MBIO read control parameters */
	int	read_datalist = MB_NO;
	char	read_file[MB_PATH_MAXLINE];
	void	*datalist;
	int	look_processed = MB_DATALIST_LOOK_UNSET;
	double	file_weight;
	int	format;
	int	pings;
	int	lonflip;
	double	bounds[4];
	int	btime_i[7];
	int	etime_i[7];
	double	btime_d;
	double	etime_d;
	double	speedmin;
	double	timegap;
	char	swathfile[MB_PATH_MAXLINE];
	char	amptablefile[MB_PATH_MAXLINE];
	char	sstablefile[MB_PATH_MAXLINE];
	FILE	*atfp;
	FILE	*stfp;
	int	beams_bath;
	int	beams_amp;
	int	pixels_ss;

	/* MBIO read values */
	void	*mbio_ptr = NULL;
	int	kind;
	int	time_i[7];
	double	time_d;
	double	navlon;
	double	navlat;
	double	speed;
	double	heading;
	double	distance;
	double	altitude;
	double	sonardepth;
	char	*beamflag = NULL;
	double	*bath = NULL;
	double	*bathacrosstrack = NULL;
	double	*bathalongtrack = NULL;
	double	*amp = NULL;
	double	*ss = NULL;
	double	*ssacrosstrack = NULL;
	double	*ssalongtrack = NULL;
	char	comment[256];

	/* slope calculation variables */
	int	ndepths;
	double	*depths;
	double	*depthacrosstrack;
	int	nslopes;
	double	*slopes;
	double	*slopeacrosstrack;

	/* angle function variables */
	int	amplitude_on = MB_NO;
	int	sidescan_on = MB_NO;
	int	dump = MB_NO;
	int	symmetry = MB_NO;
	int	nangles = 81;
	double	angle_max = 80.0;
	double	dangle;
	double	angle_start;
	int	pings_avg = 25;
	int	navg = 0;
	int	*nmeanamp = NULL;
	double	*meanamp = NULL;
	double	*sigmaamp = NULL;
	int	*nmeanss = NULL;
	double	*meanss = NULL;
	double	*sigmass = NULL;
	double	altitude_default = 0.0;
	double	time_d_avg;
	int	amp_corr_mode;
	int	ss_corr_mode;
	double	ref_angle;
	double	ref_angle_default = 30.0;

	int	ampkind;
	int	read_data;
	double	bathy;
	double	altitude_use;
	double	slope;
	double	angle;
	int	nrec, namp, nss, ntable;
	int	nrectot = 0;
	int	namptot = 0;
	int	nsstot = 0;
	int	ntabletot = 0;

	/* time, user, host variables */
	time_t	right_now;
	char	date[25], user[MB_PATH_MAXLINE], *user_ptr, host[MB_PATH_MAXLINE];

	int	i, j;
	
	char	*ctime();
	char	*getenv();

	/* get current default values */
	status = mb_defaults(verbose,&format,&pings,&lonflip,bounds,
		btime_i,etime_i,&speedmin,&timegap);

	/* reset pings and timegap */
	pings = 1;
	timegap = 10000000.0;

	/* set default input to stdin */
	strcpy (read_file, "datalist.mb-1");

	/* process argument list */
	while ((c = getopt(argc, argv, "A:a:CcDdF:f:HhI:i:N:n:P:p:R:r:VvZ:z:")) != -1)
	  switch (c) 
		{
		case 'A':
		case 'a':
			sscanf (optarg,"%d", &ampkind);
			if (ampkind == MBBACKANGLE_SS)
				sidescan_on = MB_YES;
			if (ampkind == MBBACKANGLE_AMP)
				amplitude_on = MB_YES;
			flag++;
			break;
		case 'C':
		case 'c':
			symmetry = MB_YES;
			flag++;
			break;
		case 'D':
		case 'd':
			dump = MB_YES;
			flag++;
			break;
		case 'F':
		case 'f':
			sscanf (optarg,"%d", &format);
			flag++;
			break;
		case 'H':
		case 'h':
			help++;
			break;
		case 'I':
		case 'i':
			sscanf (optarg,"%s", read_file);
			flag++;
			break;
		case 'N':
		case 'n':
			sscanf (optarg,"%d/%lf", &nangles, &angle_max);
			flag++;
			break;
		case 'P':
		case 'p':
			sscanf (optarg,"%d", &pings_avg);
			flag++;
			break;
		case 'R':
		case 'r':
			sscanf (optarg,"%lf", &ref_angle_default);
			flag++;
			break;
		case 'V':
		case 'v':
			verbose++;
			break;
		case 'Z':
		case 'z':
			sscanf (optarg,"%lf", &altitude_default);
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
		error = MB_ERROR_BAD_USAGE;
		exit(error);
		}

	/* print starting message */
	if (verbose == 1 || help)
		{
		fprintf(stderr,"\nProgram %s\n",program_name);
		fprintf(stderr,"Version %s\n",rcs_id);
		fprintf(stderr,"MB-system Version %s\n",MB_VERSION);
		}
		
	/* set mode if necessary */
	if (amplitude_on != MB_YES 
		&& sidescan_on != MB_YES)
		{
		amplitude_on = MB_YES;
		sidescan_on = MB_YES;
		}

	/* print starting debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  Program <%s>\n",program_name);
		fprintf(stderr,"dbg2  Version %s\n",rcs_id);
		fprintf(stderr,"dbg2  MB-system Version %s\n",MB_VERSION);
		fprintf(stderr,"dbg2  Control Parameters:\n");
		fprintf(stderr,"dbg2       verbose:      %d\n",verbose);
		fprintf(stderr,"dbg2       help:         %d\n",help);
		fprintf(stderr,"dbg2       format:       %d\n",format);
		fprintf(stderr,"dbg2       pings:        %d\n",pings);
		fprintf(stderr,"dbg2       lonflip:      %d\n",lonflip);
		fprintf(stderr,"dbg2       bounds[0]:    %f\n",bounds[0]);
		fprintf(stderr,"dbg2       bounds[1]:    %f\n",bounds[1]);
		fprintf(stderr,"dbg2       bounds[2]:    %f\n",bounds[2]);
		fprintf(stderr,"dbg2       bounds[3]:    %f\n",bounds[3]);
		fprintf(stderr,"dbg2       btime_i[0]:   %d\n",btime_i[0]);
		fprintf(stderr,"dbg2       btime_i[1]:   %d\n",btime_i[1]);
		fprintf(stderr,"dbg2       btime_i[2]:   %d\n",btime_i[2]);
		fprintf(stderr,"dbg2       btime_i[3]:   %d\n",btime_i[3]);
		fprintf(stderr,"dbg2       btime_i[4]:   %d\n",btime_i[4]);
		fprintf(stderr,"dbg2       btime_i[5]:   %d\n",btime_i[5]);
		fprintf(stderr,"dbg2       btime_i[6]:   %d\n",btime_i[6]);
		fprintf(stderr,"dbg2       etime_i[0]:   %d\n",etime_i[0]);
		fprintf(stderr,"dbg2       etime_i[1]:   %d\n",etime_i[1]);
		fprintf(stderr,"dbg2       etime_i[2]:   %d\n",etime_i[2]);
		fprintf(stderr,"dbg2       etime_i[3]:   %d\n",etime_i[3]);
		fprintf(stderr,"dbg2       etime_i[4]:   %d\n",etime_i[4]);
		fprintf(stderr,"dbg2       etime_i[5]:   %d\n",etime_i[5]);
		fprintf(stderr,"dbg2       etime_i[6]:   %d\n",etime_i[6]);
		fprintf(stderr,"dbg2       speedmin:     %f\n",speedmin);
		fprintf(stderr,"dbg2       timegap:      %f\n",timegap);
		fprintf(stderr,"dbg2       read_file:    %s\n",read_file);
		fprintf(stderr,"dbg2       dump:         %d\n",dump);
		fprintf(stderr,"dbg2       symmetry:     %d\n",symmetry);
		fprintf(stderr,"dbg2       amplitude_on: %d\n",amplitude_on);
		fprintf(stderr,"dbg2       sidescan_on:  %d\n",sidescan_on);
		fprintf(stderr,"dbg2       nangles:      %d\n",nangles);
		fprintf(stderr,"dbg2       angle_max:    %f\n",angle_max);
		fprintf(stderr,"dbg2       ref_angle:    %f\n",ref_angle_default);
		fprintf(stderr,"dbg2       pings_avg:    %d\n",pings_avg);
		fprintf(stderr,"dbg2       angle_max:    %f\n",angle_max);
		fprintf(stderr,"dbg2       altitude:     %f\n",altitude_default);
		}

	/* if help desired then print it and exit */
	if (help)
		{
		fprintf(stderr,"\n%s\n",help_message);
		fprintf(stderr,"\nusage: %s\n", usage_message);
		exit(error);
		}

	/* allocate memory for angle arrays */
	if (amplitude_on == MB_YES)
		{
		if (error == MB_ERROR_NO_ERROR)
			status = mb_malloc(verbose,nangles*sizeof(int),
				&nmeanamp,&error);
		if (error == MB_ERROR_NO_ERROR)
			status = mb_malloc(verbose,nangles*sizeof(double),
				&meanamp,&error);
		if (error == MB_ERROR_NO_ERROR)
			status = mb_malloc(verbose,nangles*sizeof(double),
				&sigmaamp,&error);
		}
	if (sidescan_on == MB_YES)
		{
		if (error == MB_ERROR_NO_ERROR)
			status = mb_malloc(verbose,nangles*sizeof(int),
				&nmeanss,&error);
		if (error == MB_ERROR_NO_ERROR)
			status = mb_malloc(verbose,nangles*sizeof(double),
				&meanss,&error);
		if (error == MB_ERROR_NO_ERROR)
			status = mb_malloc(verbose,nangles*sizeof(double),
				&sigmass,&error);
		}

	/* if error initializing memory then quit */
	if (error != MB_ERROR_NO_ERROR)
		{
		mb_error(verbose,error,&message);
		fprintf(stderr,"\nMBIO Error allocating angle arrays:\n%s\n",message);
		fprintf(stderr,"\nProgram <%s> Terminated\n",
			program_name);
		exit(error);
		}

	/* output some information */
	if (verbose > 0)
		{
		fprintf(stderr, "\nPings to average:    %d\n", pings_avg);
		fprintf(stderr, "Number of angle bins: %d\n", nangles);
		fprintf(stderr, "Maximum angle:         %f\n", angle_max);
		fprintf(stderr, "Default altitude:      %f\n", altitude_default);
		if (amplitude_on == MB_YES)
			fprintf(stderr, "Working on beam amplitude data...\n");
		if (sidescan_on == MB_YES)
			fprintf(stderr, "Working on sidescan data...\n");
		}

	/* get size of bins */
	dangle = 2 * angle_max / (nangles-1);
	angle_start = -angle_max - 0.5*dangle;

	/* initialize histogram */
	if (amplitude_on == MB_YES)
	for (i=0;i<nangles;i++)
		{
		nmeanamp[i] = 0;
		meanamp[i] = 0.0;
		sigmaamp[i] = 0.0;
		}
	if (sidescan_on == MB_YES)
	for (i=0;i<nangles;i++)
		{
		nmeanss[i] = 0;
		meanss[i] = 0.0;
		sigmass[i] = 0.0;
		}

	/* get format if required */
	if (format == 0)
		mb_get_format(verbose,read_file,NULL,&format,&error);

	/* determine whether to read one file or a list of files */
	if (format < 0)
		read_datalist = MB_YES;

	/* open file list */
	if (read_datalist == MB_YES)
	    {
	    if ((status = mb_datalist_open(verbose,&datalist,
					    read_file,look_processed,&error)) != MB_SUCCESS)
		{
		error = MB_ERROR_OPEN_FAIL;
		fprintf(stderr,"\nUnable to open data list file: %s\n",
			read_file);
		fprintf(stderr,"\nProgram <%s> Terminated\n",
			program_name);
		exit(error);
		}
	    if (status = mb_datalist_read(verbose,datalist,
			    swathfile,&format,&file_weight,&error)
			    == MB_SUCCESS)
		read_data = MB_YES;
	    else
		read_data = MB_NO;
	    }
	/* else copy single filename to be read */
	else
	    {
	    strcpy(swathfile, read_file);
	    read_data = MB_YES;
	    }

	/* loop over all files to be read */
	while (read_data == MB_YES)
	{

	/* obtain format array location - format id will 
		be aliased to current id if old format id given */
	status = mb_format(verbose,&format,&error);

	/* initialize reading the swath sonar file */
	if ((status = mb_read_init(
		verbose,swathfile,format,1,lonflip,bounds,
		btime_i,etime_i,speedmin,timegap,
		&mbio_ptr,&btime_d,&etime_d,
		&beams_bath,&beams_amp,&pixels_ss,&error)) != MB_SUCCESS)
		{
		mb_error(verbose,error,&message);
		fprintf(stderr,"\nMBIO Error returned from function <mb_read_init>:\n%s\n",message);
		fprintf(stderr,"\nMultibeam File <%s> not initialized for reading\n",swathfile);
		fprintf(stderr,"\nProgram <%s> Terminated\n",
			program_name);
		exit(error);
		}
		
	/* set correction modes according to format */
	if (format == MBF_SB2100RW
	    || format == MBF_SB2100B1
	    || format == MBF_SB2100B2)
	    ss_corr_mode = MBP_SSCORR_DIVISION;
	else
	    ss_corr_mode = MBP_SSCORR_SUBTRACTION;
	amp_corr_mode = MBP_AMPCORR_SUBTRACTION;
	ref_angle = ref_angle_default;
	
	/* allocate memory for data arrays */
	if (error == MB_ERROR_NO_ERROR)
		status = mb_malloc(verbose,beams_bath*sizeof(char),
					&beamflag,&error);
	if (error == MB_ERROR_NO_ERROR)
		status = mb_malloc(verbose,beams_bath*sizeof(double),
					&bath,&error);
	if (error == MB_ERROR_NO_ERROR)
		status = mb_malloc(verbose,beams_amp*sizeof(double),
					&amp,&error);
	if (error == MB_ERROR_NO_ERROR)
		status = mb_malloc(verbose,beams_bath*sizeof(double),
					&bathacrosstrack,&error);
	if (error == MB_ERROR_NO_ERROR)
		status = mb_malloc(verbose,beams_bath*sizeof(double),
					&bathalongtrack,&error);
	if (error == MB_ERROR_NO_ERROR)
		status = mb_malloc(verbose,pixels_ss*sizeof(double),
					&ss,&error);
	if (error == MB_ERROR_NO_ERROR)
		status = mb_malloc(verbose,pixels_ss*sizeof(double),
					&ssacrosstrack,&error);
	if (error == MB_ERROR_NO_ERROR)
		status = mb_malloc(verbose,pixels_ss*sizeof(double),
					&ssalongtrack,&error);
	if (error == MB_ERROR_NO_ERROR)
		status = mb_malloc(verbose,beams_bath*sizeof(double),
					&depths,&error);
	if (error == MB_ERROR_NO_ERROR)
		status = mb_malloc(verbose,beams_bath*sizeof(double),
					&depthacrosstrack,&error);
	if (error == MB_ERROR_NO_ERROR)
		status = mb_malloc(verbose,(beams_bath+1)*sizeof(double),
					&slopes,&error);
	if (error == MB_ERROR_NO_ERROR)
		status = mb_malloc(verbose,(beams_bath+1)*sizeof(double),
					&slopeacrosstrack,&error);

	/* if error initializing memory then quit */
	if (error != MB_ERROR_NO_ERROR)
		{
		mb_error(verbose,error,&message);
		fprintf(stderr,"\nMBIO Error allocating data arrays:\n%s\n",message);
		fprintf(stderr,"\nProgram <%s> Terminated\n",
			program_name);
		exit(error);
		}

	/* output information */
	if (error == MB_ERROR_NO_ERROR && verbose > 0)
	    {
	    fprintf(stderr, "\nprocessing swath file: %s %d\n", swathfile, format);
	    }

	/* open output file */
	if (error == MB_ERROR_NO_ERROR
		&& dump == MB_YES)
	    {
	    atfp = stdout;
	    stfp = stdout;
	    }
	else if (error == MB_ERROR_NO_ERROR)
	    {
	    if (amplitude_on == MB_YES)
	    	{
	    	strcpy(amptablefile,swathfile);
 	    	strcat(amptablefile,".aga");
	    	if ((atfp = fopen(amptablefile,"w")) == NULL)
		    {
		    error = MB_ERROR_OPEN_FAIL;
		    mb_error(verbose,error,&message);
		    fprintf(stderr, "\nUnable to open output table file %s\n", amptablefile);
		    fprintf(stderr, "Program %s aborted!\n", program_name);
		    exit(error);
		    }
		}
	    if (sidescan_on == MB_YES)
	    	{
	    	strcpy(sstablefile,swathfile);
 	    	strcat(sstablefile,".sga");
	    	if ((stfp = fopen(sstablefile,"w")) == NULL)
		    {
		    error = MB_ERROR_OPEN_FAIL;
		    mb_error(verbose,error,&message);
		    fprintf(stderr, "\nUnable to open output table file %s\n", sstablefile);
		    fprintf(stderr, "Program %s aborted!\n", program_name);
		    exit(error);
		    }
		}
	    }

	/* write to output file */
	if (error == MB_ERROR_NO_ERROR)
	    {
	    /* set comments in table files */
	    if (amplitude_on == MB_YES)
	    	{
		fprintf(atfp,"## Amplitude correction table files generated by program %s\n",program_name);
	    	fprintf(atfp,"## Version %s\n",rcs_id);
	    	fprintf(atfp,"## MB-system Version %s\n",MB_VERSION);
	    	fprintf(atfp,"## Table file format: 1.0.0\n");
	    	right_now = time((time_t *)0);
	    	strncpy(date,ctime(&right_now),24);
	    	if ((user_ptr = getenv("USER")) == NULL)
			user_ptr = getenv("LOGNAME");
	    	if (user_ptr != NULL)
			strcpy(user,user_ptr);
	    	else
			strcpy(user, "unknown");
	    	gethostname(host,MB_PATH_MAXLINE);
	    	fprintf(atfp,"## Run by user <%s> on cpu <%s> at <%s>\n",
			user,host,date);
	    	fprintf(atfp, "## Input swath file:      %s\n", swathfile);
	    	fprintf(atfp, "## Input swath format:    %d\n", format);
	    	fprintf(atfp, "## Output table file:     %s\n", amptablefile);
	    	fprintf(atfp, "## Pings to average:      %d\n", pings_avg);
	    	fprintf(atfp, "## Number of angle bins:  %d\n", nangles);
	    	fprintf(atfp, "## Maximum angle:         %f\n", angle_max);
	   	fprintf(atfp, "## Default altitude:      %f\n", altitude_default);
	   	fprintf(atfp, "## Data type:             beam amplitude\n");
		}

	    if (sidescan_on == MB_YES)
	    	{
		fprintf(stfp,"## Sidescan correction table files generated by program %s\n",program_name);
	    	fprintf(stfp,"## Version %s\n",rcs_id);
	    	fprintf(stfp,"## MB-system Version %s\n",MB_VERSION);
	    	fprintf(stfp,"## Table file format: 1.0.0\n");
	    	right_now = time((time_t *)0);
	    	strncpy(date,ctime(&right_now),24);
	    	if ((user_ptr = getenv("USER")) == NULL)
			user_ptr = getenv("LOGNAME");
	    	if (user_ptr != NULL)
			strcpy(user,user_ptr);
	    	else
			strcpy(user, "unknown");
	    	gethostname(host,MB_PATH_MAXLINE);
	    	fprintf(stfp,"## Run by user <%s> on cpu <%s> at <%s>\n",
			user,host,date);
	    	fprintf(stfp, "## Input swath file:      %s\n", swathfile);
	    	fprintf(stfp, "## Input swath format:    %d\n", format);
	    	fprintf(stfp, "## Output table file:     %s\n", sstablefile);
	    	fprintf(stfp, "## Pings to average:      %d\n", pings_avg);
	    	fprintf(stfp, "## Number of angle bins:  %d\n", nangles);
	    	fprintf(stfp, "## Maximum angle:         %f\n", angle_max);
	   	fprintf(stfp, "## Default altitude:      %f\n", altitude_default);
	   	fprintf(stfp, "## Data type:             sidescan\n");
		}
	    }

	/* initialize counting variables */
	nrec = 0;
	namp = 0;
	nss = 0;
	navg = 0;
	ntable = 0;

	/* read and process data */
	while (error <= MB_ERROR_NO_ERROR)
		{

		/* read a ping of data */
		status = mb_get(verbose,mbio_ptr,&kind,&pings,
				time_i,&time_d,
				&navlon,&navlat,
				&speed,&heading,
				&distance,&altitude,&sonardepth,
				&beams_bath,&beams_amp,&pixels_ss,
				beamflag,bath,amp,bathacrosstrack,bathalongtrack,
				ss,ssacrosstrack,ssalongtrack,
				comment,&error);

		if ((navg > 0
			&& (error == MB_ERROR_TIME_GAP
				|| error == MB_ERROR_EOF))
			|| (navg >= pings_avg))
			{
			/* write out tables */
			time_d_avg /= navg;
			if (amplitude_on == MB_YES)
			output_table(verbose, atfp, ntable, navg, time_d_avg, 
					nangles, angle_max, dangle, symmetry,
					nmeanamp, meanamp, sigmaamp, &error);
			if (sidescan_on == MB_YES)
			output_table(verbose, stfp, ntable, navg, time_d_avg, 
					nangles, angle_max, dangle, symmetry,
					nmeanss, meanss, sigmass, &error);
			ntable++;
					
			/* reinitialize arrays */
			navg = 0;
			time_d_avg = 0.0;
			if (amplitude_on == MB_YES)
			for (i=0;i<nangles;i++)
				{
				nmeanamp[i] = 0;
				meanamp[i] = 0.0;
				sigmaamp[i] = 0.0;
				}
			if (sidescan_on == MB_YES)
			for (i=0;i<nangles;i++)
				{
				nmeanss[i] = 0;
				meanss[i] = 0.0;
				sigmass[i] = 0.0;
				}
			}


		/* process the pings */
		if (error == MB_ERROR_NO_ERROR 
		    || error == MB_ERROR_TIME_GAP)
		    {
		    /* increment record counter */
		    nrec++;
		    navg++;
		    
		    /* increment time */
		    time_d_avg += time_d;

		    /* get the seafloor slopes */
		    if (beams_bath > 0)
			set_bathyslope(verbose, 
				beams_bath,beamflag,bath,bathacrosstrack,
				&ndepths,depths,depthacrosstrack,
				&nslopes,slopes,slopeacrosstrack,
				&error);

		    /* do the amplitude */
		    if (amplitude_on == MB_YES)
		    for (i=0;i<beams_amp;i++)
			{
			if (mb_beam_ok(beamflag[i]))
			    {
			    namp++;
			    if (beams_bath != beams_amp)
				{
				bathy = altitude_default;
				slope = 0.0;
				}
			    else
				{
				status = get_bathyslope(verbose,
				    ndepths,depths,depthacrosstrack,
				    nslopes,slopes,slopeacrosstrack,
				    bathacrosstrack[i],
				    &bathy,&slope,&error);
				if (status != MB_SUCCESS)
				    {
				    bathy = altitude_default;
				    slope = 0.0;
				    status = MB_SUCCESS;
				    error = MB_ERROR_NO_ERROR;
				    }
				}
			    if (bathy > 0.0)
				{
				altitude_use = bathy - sonardepth;
				angle = RTD*(atan(bathacrosstrack[i]/altitude_use)
				    + atan(slope));
				j = (angle - angle_start)/dangle;
				/*j = (int)((fabs(angle) + 0.5 * dangle)/dangle);*/
				if (j >= 0 && j < nangles)
				    {
				    meanamp[j] += amp[i];
				    sigmaamp[j] += amp[i]*amp[i];
				    nmeanamp[j]++;
				    }
				}

			    /* print debug statements */
			    if (verbose >= 5)
				{
				fprintf(stderr,"dbg5       %d %d: slope:%f altitude:%f xtrack:%f ang:%f j:%d\n",
				    nrec, i, slope, altitude_use, bathacrosstrack[i], angle, j);
				}
			    }
			}

		    /* do the sidescan */
		    if (sidescan_on == MB_YES)
		    for (i=0;i<pixels_ss;i++)
			{
			if (ss[i] > 0.0)
			    {
			    nss++;
			    if (beams_bath > 0)
				{
				status = get_bathyslope(verbose,
				    ndepths,depths,depthacrosstrack,
				    nslopes,slopes,slopeacrosstrack,
				    ssacrosstrack[i],
				    &bathy,&slope,&error);
				if (status != MB_SUCCESS)
				    {
				    bathy = altitude_default;
				    slope = 0.0;
				    status = MB_SUCCESS;
				    error = MB_ERROR_NO_ERROR;
				    }
				}
			    else
				{
				bathy = altitude_default;
				slope = 0.0;
				}
			    if (bathy > 0.0)
				{
				altitude_use = bathy - sonardepth;
				angle = RTD*(atan(ssacrosstrack[i]/altitude_use) 
				    + atan(slope));
				j = (angle - angle_start)/dangle;
				/*j = (int)((fabs(angle) + 0.5 * dangle)/dangle);*/
				if (j >= 0 && j < nangles)
				    {
				    meanss[j] += ss[i];
				    sigmass[j] += ss[i]*ss[i];
				    nmeanss[j]++;
				    }
				}

			    /* print debug statements */
			    if (verbose >= 5)
				{
				fprintf(stderr,"dbg5       %d %d: slope:%f altitude:%f xtrack:%f ang:%f j:%d\n",
				    nrec, i, slope, altitude_use, ssacrosstrack[i], angle, j);
				}
			    }
			}

		    }
		}

	/* close the swath sonar file */
	status = mb_close(verbose,&mbio_ptr,&error);
	if (dump == MB_NO && amplitude_on == MB_YES)
		fclose(atfp);
	if (dump == MB_NO && sidescan_on == MB_YES)
		fclose(stfp);
	ntabletot += ntable;
	nrectot += nrec;
	namptot += namp;
	nsstot += nss;

	/* set amplitude correction in parameter file */
	if (amplitude_on == MB_YES)
		status = mb_pr_update_ampcorr(verbose, swathfile, 
			MB_YES, amptablefile, 
			amp_corr_mode, MB_YES, ref_angle, MB_NO, 
			&error);

	/* set sidescan correction in parameter file */
	if (sidescan_on == MB_YES)
		status = mb_pr_update_sscorr(verbose, swathfile, 
			MB_YES, sstablefile, 
			ss_corr_mode, MB_YES, ref_angle, MB_NO, 
			&error);

	/* output information */
	if (error == MB_ERROR_NO_ERROR && verbose > 0)
	    {
	    fprintf(stderr, "%d records processed\n", nrec);
	    if (amplitude_on == MB_YES)
		{
		fprintf(stderr, "%d amplitude data processed\n", namp);
	    	fprintf(stderr, "%d tables written to %s\n", ntable, amptablefile);
		}
	    if (sidescan_on == MB_YES)
		{
		fprintf(stderr, "%d sidescan data processed\n", nss);
	    	fprintf(stderr, "%d tables written to %s\n", ntable, sstablefile);
		}
	    }

	/* deallocate memory used for data arrays */
	mb_free(verbose,&beamflag,&error);
	mb_free(verbose,&bath,&error);
	mb_free(verbose,&amp,&error);
	mb_free(verbose,&bathacrosstrack,&error);
	mb_free(verbose,&bathalongtrack,&error);
	mb_free(verbose,&ss,&error);
	mb_free(verbose,&ssacrosstrack,&error);
	mb_free(verbose,&ssalongtrack,&error);
	mb_free(verbose,&depths,&error);
	mb_free(verbose,&depthacrosstrack,&error);
	mb_free(verbose,&slopes,&error);
	mb_free(verbose,&slopeacrosstrack,&error);

	/* figure out whether and what to read next */
        if (read_datalist == MB_YES)
                {
		if (status = mb_datalist_read(verbose,datalist,
			    swathfile,&format,&file_weight,&error)
			    == MB_SUCCESS)
                        read_data = MB_YES;
                else
                        read_data = MB_NO;
                }
        else
                {
                read_data = MB_NO;
                }

	/* end loop over files in list */
	}
        if (read_datalist == MB_YES)
		mb_datalist_close(verbose,&datalist,&error);

	/* output information */
	if (error == MB_ERROR_NO_ERROR && verbose > 0)
	    {
	    fprintf(stderr, "\n%d total records processed\n", nrectot);
	    if (amplitude_on == MB_YES)
		{
		fprintf(stderr, "%d total amplitude data processed\n", namptot);
	    	fprintf(stderr, "%d total aga tables written\n", ntabletot);
		}
	    if (sidescan_on == MB_YES)
		{
		fprintf(stderr, "%d total sidescan data processed\n", nsstot);
	    	fprintf(stderr, "%d total sga tables written\n", ntabletot);
		}
	    }

	/* deallocate memory used for data arrays */
	if (amplitude_on == MB_YES)
		{
		mb_free(verbose,&nmeanamp,&error);
		mb_free(verbose,&meanamp,&error);
		mb_free(verbose,&sigmaamp,&error);
		}
	if (sidescan_on == MB_YES)
		{
		mb_free(verbose,&nmeanss,&error);
		mb_free(verbose,&meanss,&error);
		mb_free(verbose,&sigmass,&error);
		}

	/* set program status */
	status = MB_SUCCESS;

	/* check memory */
	if (verbose >= 4)
		status = mb_memory_list(verbose,&error);

	/* print output debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  Program <%s> completed\n",
			program_name);
		fprintf(stderr,"dbg2  Ending status:\n");
		fprintf(stderr,"dbg2       status:  %d\n",status);
		}

	/* end it all */
	if (verbose > 0)
		fprintf(stderr,"\n");
	exit(error);
}
/*--------------------------------------------------------------------*/
int set_bathyslope(int verbose,
	int nbath, char *beamflag, double *bath, double *bathacrosstrack,
	int *ndepths, double *depths, double *depthacrosstrack, 
	int *nslopes, double *slopes, double *slopeacrosstrack, 
	int *error)
{
	char	*function_name = "set_bathyslope";
	int	status = MB_SUCCESS;
	int	i;
	

	/* print input debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBBACKANGLE function <%s> called\n",
			function_name);
		fprintf(stderr,"dbg2  Input arguments:\n");
		fprintf(stderr,"dbg2       verbose:         %d\n",verbose);
		fprintf(stderr,"dbg2       nbath:           %d\n",nbath);
		fprintf(stderr,"dbg2       bath:            %d\n",bath);
		fprintf(stderr,"dbg2       bathacrosstrack: %d\n",
			bathacrosstrack);
		fprintf(stderr,"dbg2       bath:\n");
		for (i=0;i<nbath;i++)
			fprintf(stderr,"dbg2         %d %f %f\n", 
				i, bath[i], bathacrosstrack[i]);
		}

	/* first find all depths */
	*ndepths = 0;
	for (i=0;i<nbath;i++)
		{
		if (mb_beam_ok(beamflag[i]))
			{
			depths[*ndepths] = bath[i];
			depthacrosstrack[*ndepths] = bathacrosstrack[i];
			(*ndepths)++;
			}
		}

	/* now calculate slopes */
	*nslopes = *ndepths + 1;
	for (i=0;i<*ndepths-1;i++)
		{
		slopes[i+1] = (depths[i+1] - depths[i])
			/(depthacrosstrack[i+1] - depthacrosstrack[i]);
		slopeacrosstrack[i+1] = 0.5*(depthacrosstrack[i+1] 
			+ depthacrosstrack[i]);
		}
	if (*ndepths > 1)
		{
		slopes[0] = 0.0;
		slopeacrosstrack[0] = depthacrosstrack[0];
		slopes[*ndepths] = 0.0;
		slopeacrosstrack[*ndepths] = 
			depthacrosstrack[*ndepths-1];
		}

	/* print output debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBBACKANGLE function <%s> completed\n",
			function_name);
		fprintf(stderr,"dbg2  Return values:\n");
		fprintf(stderr,"dbg2       ndepths:         %d\n",
			*ndepths);
		fprintf(stderr,"dbg2       depths:\n");
		for (i=0;i<*ndepths;i++)
			fprintf(stderr,"dbg2         %d %f %f\n", 
				i, depths[i], depthacrosstrack[i]);
		fprintf(stderr,"dbg2       nslopes:         %d\n",
			*nslopes);
		fprintf(stderr,"dbg2       slopes:\n");
		for (i=0;i<*nslopes;i++)
			fprintf(stderr,"dbg2         %d %f %f\n", 
				i, slopes[i], slopeacrosstrack[i]);
		fprintf(stderr,"dbg2       error:           %d\n",*error);
		fprintf(stderr,"dbg2  Return status:\n");
		fprintf(stderr,"dbg2       status:          %d\n",status);
		}

	/* return status */
	return(status);
}
/*--------------------------------------------------------------------*/
int get_bathyslope(int verbose,
	int ndepths, double *depths, double *depthacrosstrack, 
	int nslopes, double *slopes, double *slopeacrosstrack, 
	double acrosstrack, double *depth,  double *slope, 
	int *error)
{
	char	*function_name = "get_bathyslope";
	int	status = MB_SUCCESS;
	int	found_depth, found_slope;
	int	idepth, islope;
	int	i;
	

	/* print input debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBBACKANGLE function <%s> called\n",
			function_name);
		fprintf(stderr,"dbg2  Input arguments:\n");
		fprintf(stderr,"dbg2       verbose:         %d\n",verbose);
		fprintf(stderr,"dbg2       ndepths:         %d\n",
			ndepths);
		fprintf(stderr,"dbg2       depths:\n");
		for (i=0;i<ndepths;i++)
			fprintf(stderr,"dbg2         %d %f %f\n", 
				i, depths[i], depthacrosstrack[i]);
		fprintf(stderr,"dbg2       nslopes:         %d\n",
			nslopes);
		fprintf(stderr,"dbg2       slopes:\n");
		for (i=0;i<nslopes;i++)
			fprintf(stderr,"dbg2         %d %f %f\n", 
				i, slopes[i], slopeacrosstrack[i]);
		fprintf(stderr,"dbg2       acrosstrack:     %f\n",acrosstrack);
		}

	/* check if acrosstrack is in defined interval */
	found_depth = MB_NO;
	found_slope = MB_NO;
	if (ndepths > 1)
	if (acrosstrack >= depthacrosstrack[0]
		&& acrosstrack <= depthacrosstrack[ndepths-1])
	    {

	    /* look for depth */
	    idepth = -1;
	    while (found_depth == MB_NO && idepth < ndepths - 2)
		{
		idepth++;
		if (acrosstrack >= depthacrosstrack[idepth]
		    && acrosstrack <= depthacrosstrack[idepth+1])
		    {
		    *depth = depths[idepth] 
			    + (acrosstrack - depthacrosstrack[idepth])
			    /(depthacrosstrack[idepth+1] 
			    - depthacrosstrack[idepth])
			    *(depths[idepth+1] - depths[idepth]);
		    found_depth = MB_YES;
		    *error = MB_ERROR_NO_ERROR;
		    }
		}

	    /* look for slope */
	    islope = -1;
	    while (found_slope == MB_NO && islope < nslopes - 2)
		{
		islope++;
		if (acrosstrack >= slopeacrosstrack[islope]
		    && acrosstrack <= slopeacrosstrack[islope+1])
		    {
		    *slope = slopes[islope] 
			    + (acrosstrack - slopeacrosstrack[islope])
			    /(slopeacrosstrack[islope+1] 
			    - slopeacrosstrack[islope])
			    *(slopes[islope+1] - slopes[islope]);
		    found_slope = MB_YES;
		    *error = MB_ERROR_NO_ERROR;
		    }
		}
	    }

	/* check for failure */
	if (found_depth != MB_YES || found_slope != MB_YES)
		{
		status = MB_FAILURE;
		*error = MB_ERROR_OTHER;
		*depth = 0.0;
		*slope = 0.0;
		}

	/* print output debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBBACKANGLE function <%s> completed\n",
			function_name);
		fprintf(stderr,"dbg2  Return values:\n");
		fprintf(stderr,"dbg2       depth:           %f\n",*depth);
		fprintf(stderr,"dbg2       slope:           %f\n",*slope);
		fprintf(stderr,"dbg2       error:           %d\n",*error);
		fprintf(stderr,"dbg2  Return status:\n");
		fprintf(stderr,"dbg2       status:          %d\n",status);
		}

	/* return status */
	return(status);
}
/*--------------------------------------------------------------------*/
int output_table(int verbose, FILE *tfp, int ntable, int nping, double time_d,
	int nangles, double angle_max, double dangle, int symmetry, 
	int *nmean, double *mean, double *sigma, 
	int *error)
{
	char	*function_name = "get_bathyslope";
	int	status = MB_SUCCESS;
	double	angle, amean, asigma;
	int	time_i[7];
	int	i, j;
	

	/* print input debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBBACKANGLE function <%s> called\n",
			function_name);
		fprintf(stderr,"dbg2  Input arguments:\n");
		fprintf(stderr,"dbg2       verbose:         %d\n", verbose);
		fprintf(stderr,"dbg2       tfp:             %d\n", tfp);
		fprintf(stderr,"dbg2       ntable:          %d\n", ntable);
		fprintf(stderr,"dbg2       nping:           %d\n", nping);
		fprintf(stderr,"dbg2       time_d:          %f\n", time_d);
		fprintf(stderr,"dbg2       nangles:         %d\n", nangles);
		fprintf(stderr,"dbg2       angle_max:       %f\n", angle_max);
		fprintf(stderr,"dbg2       dangle:          %f\n", dangle);
		fprintf(stderr,"dbg2       symmetry:        %d\n", symmetry);
		fprintf(stderr,"dbg2       mean and sigma:\n");
		for (i=0;i<nangles;i++)
			fprintf(stderr,"dbg2         %d %f %d %f %f\n", 
				i, (i * dangle), nmean[i], mean[i], sigma[i]);
		}

	/* process sums and print out results */
	mb_get_date(verbose, time_d, time_i, error);
	fprintf(tfp,"# table: %d\n", ntable);
	fprintf(tfp,"# nping: %d\n", nping);
	fprintf(tfp,"# time:  %4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%6.6d    %16.6f\n",
		time_i[0],time_i[1],time_i[2],
		time_i[3],time_i[4],time_i[5],
		time_i[6],time_d);
	fprintf(tfp,"# nangles: %d\n", nangles);
	for (i=0;i<nangles;i++)
		{
		j = nangles - i - 1;
		angle = -angle_max + i * dangle;
		if (symmetry == MB_YES)
			{
			amean = asigma = 0.0;
			if ((nmean[i] + nmean[j]) > nping / 2)
				{
				amean = (mean[i] + mean[j]) 
					/ (nmean[i] + nmean[j]);
				asigma = sqrt((sigma[i] + sigma[j]) 
						/ (nmean[i] + nmean[j]) 
						- amean * amean);
				}
			fprintf(tfp,"%7.4f %12.4f %12.4f\n", angle, amean, asigma);
			}
		else
			{
			amean = asigma = 0.0;
			if (nmean[i] > nping / 2)
				{
				amean = mean[i] / nmean[i];
				asigma = sqrt(sigma[i] / nmean[i] - amean * amean);
				}
			fprintf(tfp,"%7.4f %12.4f %12.4f\n", angle, amean, asigma);
			}
		}
	fprintf(tfp,"#\n");
	fprintf(tfp,"#\n");

	/* print output debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBBACKANGLE function <%s> completed\n",
			function_name);
		fprintf(stderr,"dbg2  Return values:\n");
		fprintf(stderr,"dbg2       error:           %d\n",*error);
		fprintf(stderr,"dbg2  Return status:\n");
		fprintf(stderr,"dbg2       status:          %d\n",status);
		}

	/* return status */
	return(status);
}
/*--------------------------------------------------------------------*/

