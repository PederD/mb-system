/********************************************************************
 *
 * Module Name : GSF_INDX.C
 *
 * Author/Date : J. C. Depner / 19 Jan 1995
 *
 * Description : This source file contains the file indexing functions
 *  for the GSF library.
 *
 * Restrictions/Limitations :
 * 1) This library assumes the host computer uses the ASCII character set.
 * 2) This library assumes that the data types u_short and u_long are defined
 *    on the host machine, where a u_short is a 16 bit unsigned integer, and
 *    a u_long is a 32 bit unsigned integer.
 * 3) This library assumes that the type short is at least 16 bits, and that
 *    the type int is at least 32 bits.
 * 4) This library assumes that the Posix compliant functions getpid() and
 *    unlink() are supported and that either the environment variable
 *    GSFTMPDIR is defined (as a valid directory) or the /tmp directory is
 *    available.
 *
 * Change Descriptions :
 * who	when	  what
 * ---	----	  ----
 * jsb	08/25/95  Consolidated the gsfReadIndex and gsfWriteIndex functions
 *		  into gsfRead and gsfWrite.
 * jsb  11/01/95  Completed modifications to indexing to support increase in
 *                gsf file size after initial index file creation.  The size
 *                of the file is now stored in the index file header. Index
 *                files without the expected header are recreated on the first
 *                open.
 * hem  08/20/96  Added #ifdef for percent complete "spinner"; added code to
 *                index single beam pings & summary records.  Changed building
 *                of temporary index files to first try and use the environment
 *                variable GSFTMPDIR & if it is not defined, then use /tmp for
 *                temporary index files used in building an index file.
 *                Removed output of error message from gsfCreateIndex when and
 *                unknown record ID was encountered.
 * hem  10/08/96  Fixed problem with building index files.  The incrementing of
 *                the number of record types was incrementing the wrong pointer
 *                for summary & single beam ping records.  Also added #ifdef to
 *                the end of gsfCreateIndex to stop the blank lines from being
 *                displayed if DISPLAY_SPINNER is not set.
 * jsb  03/29/97  Fixed bug in indexing the swath bathymetry summary record.
 *                Fix made to both Create and Append functions. Indexing the
 *                summary record was added release 1.03. 
 *
 *
 * Classification : Unclassified
 *
 * References : DoDBL Generic Sensor Format Sept. 30, 1993
 *
 *
 * Copyright (C) ACME Software, A Subsidiary of Fly By Night Industries, Inc.
 *
 ********************************************************************/

/* Standard c library includes.    */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef OS2
    #include <process.h>
#else
    #include <unistd.h>
#endif

/* gsf library interface description.  */
#include "gsf.h"
#include "gsf_indx.h"
#include "gsf_ft.h"

/*  Error flag defined in gsf.c */
extern int	gsfError;

/* Prototypes for local functions */
static FILE *open_temp_file(int);
static void close_temp_file(int, FILE *);
static int gsfCreateIndexFile(const char *ndx_file, int handle, GSF_FILE_TABLE *ft);
static int gsfAppendIndexFile(const char *ndx_file, int handle, GSF_FILE_TABLE *ft);

/********************************************************************
 *
 * Function Name : gsfOpenIndex
 *
 * Description : This function attempts to open an index file associated
 *  with a gsf file.  If the file exists it is opened and the header
 *  information is read into memory.  If it does not exist it is created.
 *  Creating the index file is a two pass process.  First, all of the gsf
 *  file is read and the index records are written to a number of small
 *  index files.  Then the small index files are read and written to the
 *  final index file.  The index file header format is as follows :
 *
 *	Type	  Description
 *	----	  -----------
 *	char[16]  Text header containing "INDEX-GSF-"<"version">
 *	long	  The size of the gsf file when the index file was created.
 *	int	  Endian indicator (0x00010203 or 0x03020100 depending
 *		  upon sex of machine, if 0x03020100 then the index
 *		  data must be byte swapped).
 *	int	  Number of different record types in the index file.
 *	int[4]	  16 bytes of reserved space
 *
 *
 *	int	  Record ID of first record type.
 *	long	  Address within the index file of the beginning of
 *		  the index for the first record type.
 *	int	  Number of index records for the first record type.
 *	.
 *	.
 *	.
 *	.
 *	int	  Record ID of last record type.
 *	long	  Address within the index file of the beginning of
 *		  the index for the last record type.
 *	int	  Number of index records for the last record type.
 *
 *
 *  Following the header is the index data.  The format for each index record
 *  is :
 *
 *	time_t	  Posix.4 proposed time seconds.
 *	long	  Posix.4 proposed time nanoseconds.
 *	long	  Address of the associated record within the gsf file for the
 *		  specified record type.
 *	.
 *	.
 *	.
 *	.
 *
 *
 *  After the index file is opened (or created) the index records for those
 *  ping records containing a scale factor subrecord are read into memory to
 *  reduce I/O when reading the ping records.
 *
 *
 * Inputs :
 *  filename = a fully qualified path to the gsf file
 *  handle = gsf file handle assigned by gsfOpen or gsfOpenBuffered
 *  ft = a pointer to the GSF_FILE_TABLE entry for the file specified by
 *     handle.
 *
 * Returns :
 *  This function returns zero if successful, or -1 if an error occured.
 *
 * Error Conditions : ??????????
 *
 ********************************************************************/

int
gsfOpenIndex(const char *filename, int handle, GSF_FILE_TABLE *ft)
{
    int 	     ret;
    int 	     i;
    int 	     j;
    char	     ndx_file[132];
    GSF_INDEX_HEADER index_header;

    /*	Clear the contents of the index_header structure */
    memset (&index_header, 0, sizeof(index_header));

    /*	Clear the last scale factor index.  This is used to decide whether
     *	we need to read a new scale factor record when we read a
     *	GSF_RECORD_SWATH_BATHYMETRY_PING record.
     */
    ft->index_data.last_scale_factor_index = -1;

    /* Clear the index header information in the index_data structure. */
    ft->index_data.number_of_types = 0;
    for (i = 0; i < NUM_REC_TYPES; i++)
    {
	ft->index_data.record_type[i] = -1;
	ft->index_data.number_of_records[i] = 0;
	ft->index_data.start_addr[i] = -1;
    }

    /* Create the GSF index file name (assuming an extension of .n##) */
    strcpy(ndx_file, filename);
    ndx_file[strlen(ndx_file) - 3] = 'n';

    /* Try to open the index file for read. */
    if ((ft->index_data.fp = fopen(ndx_file, "rb")) == NULL)
    {
	ret = gsfCreateIndexFile(ndx_file, handle, ft);
	if (ret)
	{
	    return(-1);
	}
	return(0);
    }

    /* The index file exists. Read the version field, if we don't
     * find the expected text then assume this is an old index file
     * (pre-version 01.01) and that we need to create a new index file.
     */
    fread(index_header.version, GSF_INDEX_VERSION_SIZE, 1, ft->index_data.fp);
    if ((index_header.version[0] == 0) ||
	(strncmp(index_header.version, "INDEX-GSF-", strlen("INDEX-GSF-"))))
    {
	fclose (ft->index_data.fp);
	ret = gsfCreateIndexFile(ndx_file, handle, ft);
	if (ret)
	{
	    return(-1);
	}
	return(0);
    }

    /* If the expected header is found then compair the current gsf file
     * size to the size that existed when the index was file was created,
     * and append to the index file if necessary.
     */
    fread(&index_header.gsfFileSize, 4, 1, ft->index_data.fp);
    if (index_header.gsfFileSize < ft->file_size)
    {
	ret = gsfAppendIndexFile(ndx_file, handle, ft);
	if (ret)
	{
	    return(-1);
	}
	return(0);
    }

    /* If we get here, then the index file that exists is ready to use.
     * Read the endian indicator to detect whether to swap incoming data.
     * If the endian indicator is not kosher, error out.
     */
    fread(&index_header.endian, 4, 1, ft->index_data.fp);
    if (index_header.endian == 0x00010203)
    {
	ft->index_data.swap = 0;
    }
    else if (index_header.endian == 0x03020100)
    {
	ft->index_data.swap = 1;
    }
    else
    {
	gsfError = GSF_CORRUPT_INDEX_FILE_ERROR;
	return (-1);
    }

    /* Read the number of record types in the index file. */
    fread(&index_header.number_record_types, 4, 1, ft->index_data.fp);
    if (ft->index_data.swap)
    {
	SwapLong((unsigned long *) &index_header.number_record_types, 1);
    }
    ft->index_data.number_of_types = index_header.number_record_types;

    /* Make sure we found a valid number_of_types */
    if ((ft->index_data.number_of_types < 1) ||
	(ft->index_data.number_of_types > NUM_REC_TYPES))
    {
	gsfError = GSF_CORRUPT_INDEX_FILE_ERROR;
	return (-1);
    }

    /* Read the four four byte reserved fields */
    fread(&index_header.spare1, 4, 1, ft->index_data.fp);
    fread(&index_header.spare2, 4, 1, ft->index_data.fp);
    fread(&index_header.spare3, 4, 1, ft->index_data.fp);
    fread(&index_header.spare4, 4, 1, ft->index_data.fp);

    /*	For each record type, read the record type, start address and
     *	number of records.
     */
    for (i = 0; i < ft->index_data.number_of_types; i++)
    {
	fread(&j, 4, 1, ft->index_data.fp);
	if (ft->index_data.swap)
	{
	    SwapLong((unsigned long *) &j, 1);
	}

	/*  This is not really necessary but it makes things easier to code.
	 *  Put the record type into the record_type[record_type] member of
	 *  the index_data structure.
	 */
	ft->index_data.record_type[j] = j;

	fread(&ft->index_data.start_addr[j], 4, 1,
	    ft->index_data.fp);
	fread(&ft->index_data.number_of_records[j], 4, 1,
	    ft->index_data.fp);

	if (ft->index_data.swap)
	{
	    SwapLong((unsigned long *) &ft->index_data.start_addr[i], 1);
	    SwapLong((unsigned long *) &ft->index_data.number_of_records[i], 1);
	}
    }

    /* Read the scale factor addresses into memory. */
    if (ft->index_data.record_type[0] != -1)
    {
	ft->index_data.scale_factor_addr =
	    (INDEX_REC *) calloc(ft->index_data.number_of_records[0],
	    sizeof(INDEX_REC));

	/*  Couldn't calloc the memory for the scale factor addresses.  */

	if (ft->index_data.scale_factor_addr == NULL)
	{
	    gsfError = GSF_MEMORY_ALLOCATION_FAILED;
	    return (-1);
	}

	fseek(ft->index_data.fp, ft->index_data.start_addr[0], 0);
	for (i = 0; i < ft->index_data.number_of_records[0]; i++)
	{
	    fread(&ft->index_data.scale_factor_addr[i], sizeof(INDEX_REC), 1,
		ft->index_data.fp);
	    if (ft->index_data.swap)
	    {
		SwapLong((unsigned long *) &ft->index_data.scale_factor_addr[i], 3);
	    }
	}
    }

    return (0);
}

/********************************************************************
 *
 * Function Name : gsfCreateIndexFile
 *
 * Description : This function is called to create a new gsf index file.
 *
 * Inputs :
 *  filename = a fully qualified path to the gsf file
 *  handle = gsf file handle assigned by gsfOpen or gsfOpenBuffered
 *  ft = a pointer to the GSF_FILE_TABLE entry for the file specified by
 *     handle.
 *
 * Returns :
 *  This function returns zero if successful, or -1 if an error occured.
 *
 * Error Conditions :
 *
 ********************************************************************/

static int
gsfCreateIndexFile(const char *ndx_file, int handle, GSF_FILE_TABLE *ft)
{
    int 	     i;
    int 	     j;
    int 	     err;
    int 	     percent;
    int 	     id;
    int 	     old_percent;
    int 	     total_recs;
    int 	     count;
    long	     endian = 0x00010203;
    long	     eof;
    long	     current;
    FILE	    *temp[NUM_REC_TYPES];
    gsfDataID	     data_id;
    gsfRecords	     records;
    INDEX_REC	     index_rec;
    GSF_INDEX_HEADER index_header;

    /* Make sure we can open a new file for write before we start to build
     * the index file.	If the open is successful, close and remove the file,
     * we'll create it again as soon as we need it.
     */
    if ((ft->index_data.fp = fopen(ndx_file, "wb+")) == NULL)
    {
	gsfError = GSF_INDEX_FILE_OPEN_ERROR;
	return (-1);
    }
    fclose (ft->index_data.fp);
    remove (ndx_file);

    /* Clear the array of temporary file descriptors */
    for (i=0; i<NUM_REC_TYPES; i++)
    {
	temp[i] = (FILE *) NULL;
    }

    /* Initialize the contents of the index file header */
    memset (&index_header, 0, sizeof(index_header));
    strncpy(index_header.version, GSF_INDEX_VERSION, GSF_INDEX_VERSION_SIZE);
    index_header.endian = endian;
    index_header.gsfFileSize = ft->file_size;

    /*	Get the address of the end of file so we can compute percentage
     *	processed for creating the index file.
     */
    eof = ft->file_size;
    percent = 0;
    old_percent = -1;

    /*	Read each gsf record and write the time and address to the
     *	temp index files.
     */
    do
    {
	/*  Save the current address within the gsf file.   */

	if ((index_rec.addr = ftell(ft->fp)) == -1 )
	{
	    gsfError = GSF_FILE_TELL_ERROR;
	    return (-1);
	}

	/*  Read the gsf record and check for end of file.  */

	if ((err = gsfRead(handle, GSF_NEXT_RECORD, &data_id, &records,
		    NULL, 0)) != -1)
	{

	    /*	Switch based on the record type that was just read.  */

	    id = data_id.recordID;
	    switch (id)
	    {
		    /*	Don't index the header or the summary.  */

		case GSF_RECORD_HEADER:
		    break;

		case GSF_RECORD_SWATH_BATHY_SUMMARY:

                    /*  If this is the first record of this type open the
                        temp file and increment the number of types.    */
                    
                    if (temp[id] == NULL)
                    {
                        temp[id] = open_temp_file(id);
			index_header.number_record_types++;
                    }

                    /*  Load the index record structure and write to the
                        temp file.	*/

                    index_rec.sec = records.summary.start_time.tv_sec;
                    index_rec.nsec = records.summary.end_time.tv_nsec;
                    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
                    ft->index_data.number_of_records[id]++;

		    break;

		case GSF_RECORD_SWATH_BATHYMETRY_PING:

		    /*	If this ping record actually contained scale
			factor information in the file, make entries to
			two indexes; the standard ping index and the
			index for pings with scale factor subrecords.	*/

		    if (ft->scales_read)
		    {
			/*  If this is the first record of this type
			    open the temp file and increment the
			    number of types.	*/

			if (temp[0] == NULL)
			{
			    temp[0] = open_temp_file(0);
			    if (temp[0] == NULL)
			    {
				return(-1);
			    }
			    index_header.number_record_types++;
			}

			/*  Load the index record structure and write to the
			    temp file.	*/

			index_rec.sec = records.mb_ping.ping_time.tv_sec;
			index_rec.nsec = records.mb_ping.ping_time.tv_nsec;
			fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[0]);
			ft->index_data.number_of_records[0]++;
		    }

		    /*	If this is the first record of this type open the
			temp file and increment the number of types.	*/

		    if (temp[id] == NULL)
		    {
			temp[id] = open_temp_file(id);
			index_header.number_record_types++;
		    }

		    /*	Load the index record structure and write to the
			temp file.  */

		    index_rec.sec = records.mb_ping.ping_time.tv_sec;
		    index_rec.nsec = records.mb_ping.ping_time.tv_nsec;
		    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
		    ft->index_data.number_of_records[id]++;

		    break;

		case GSF_RECORD_SOUND_VELOCITY_PROFILE:

		    /*	If this is the first record of this type open the
			temp file and increment the number of types.	*/

		    if (temp[id] == NULL)
		    {
			temp[id] = open_temp_file(id);
			index_header.number_record_types++;
		    }

		    /*	Load the index record structure and write to the
			temp file.  */

		    index_rec.sec = records.svp.application_time.tv_sec;
		    index_rec.nsec = records.svp.application_time.tv_nsec;
		    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
		    ft->index_data.number_of_records[id]++;

		    break;

		case GSF_RECORD_PROCESSING_PARAMETERS:

		    /*	If this is the first record of this type open the
			temp file and increment the number of types.	*/

		    if (temp[id] == NULL)
		    {
			temp[id] = open_temp_file(id);
			index_header.number_record_types++;
		    }

		    /*	Load the index record structure and write to the
			temp file.  */

		    index_rec.sec = records.process_parameters.param_time.tv_sec;
		    index_rec.nsec =
			records.process_parameters.param_time.tv_nsec;
		    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
		    ft->index_data.number_of_records[id]++;

		    break;

		case GSF_RECORD_SENSOR_PARAMETERS:

		    /*	If this is the first record of this type open the
			temp file and increment the number of types.	*/

		    if (temp[id] == NULL)
		    {
			temp[id] = open_temp_file(id);
			index_header.number_record_types++;
		    }

		    /*	Load the index record structure and write to the
			temp file.  */

		    index_rec.sec = records.sensor_parameters.param_time.tv_sec;
		    index_rec.nsec =
			records.sensor_parameters.param_time.tv_nsec;
		    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
		    ft->index_data.number_of_records[id]++;

		    break;

		case GSF_RECORD_COMMENT:

		    /*	If this is the first record of this type open the
			temp file and increment the number of types.	*/

		    if (temp[id] == NULL)
		    {
			temp[id] = open_temp_file(id);
			index_header.number_record_types++;
		    }

		    /*	Load the index record structure and write to the
			temp file.  */

		    index_rec.sec = records.comment.comment_time.tv_sec;
		    index_rec.nsec = records.comment.comment_time.tv_nsec;
		    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
		    ft->index_data.number_of_records[id]++;

		    break;

		case GSF_RECORD_HISTORY:

		    /*	If this is the first record of this type open the
			temp file and increment the number of types.	*/

		    if (temp[id] == NULL)
		    {
			temp[id] = open_temp_file(id);
			index_header.number_record_types++;
		    }

		    /*	Load the index record structure and write to the
			temp file.  */

		    index_rec.sec = records.history.history_time.tv_sec;
		    index_rec.nsec = records.history.history_time.tv_nsec;
		    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
		    ft->index_data.number_of_records[id]++;

		    break;

		case GSF_RECORD_NAVIGATION_ERROR:

		    /*	If this is the first record of this type open the
			temp file and increment the number of types.	*/

		    if (temp[id] == NULL)
		    {
			temp[id] = open_temp_file(id);
			index_header.number_record_types++;
		    }

		    /*	Load the index record structure and write to the
			temp file.  */

		    index_rec.sec = records.nav_error.nav_error_time.tv_sec;
		    index_rec.nsec = records.nav_error.nav_error_time.tv_nsec;
		    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
		    ft->index_data.number_of_records[id]++;

		    break;

                case GSF_RECORD_SINGLE_BEAM_PING:

                    /*  If this is the first record of this type open the
                        temp file and increment the number of types.    */

                    if (temp[id] == NULL)
                    {
                        temp[id] = open_temp_file(id);
                        index_header.number_record_types++;
                    }

                    /*  Load the index record structure and write to the
                        temp file.	*/
                    
                    index_rec.sec = records.sb_ping.ping_time.tv_sec;
                    index_rec.nsec = records.sb_ping.ping_time.tv_nsec;
                    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
                    ft->index_data.number_of_records[id]++;

                    break;


		default:

		    break;
	    }

#ifdef DISPLAY_SPINNER            
	    /*	Print the percent spinner to stdout.	*/
	    if ((current = ftell(ft->fp)) == -1 )
	    {
		gsfError = GSF_FILE_TELL_ERROR;
		return (-1);
	    }
	    percent = ((double) current  / (double) eof) * 100.0;
	    if (old_percent != percent)
	    {
		printf("Reading GSF file - %03d%% complete\r", percent);
		fflush(stdout);
		old_percent = percent;
	    }
#endif
	}
	else
	{
	    if (gsfError != GSF_READ_TO_END_OF_FILE)
	    {
		return(-1);
	    }
	}
    }
    while (err != -1);

    /* Create the index file */
    if ((ft->index_data.fp = fopen(ndx_file, "wb+")) == NULL)
    {
	gsfError = GSF_INDEX_FILE_OPEN_ERROR;
	return (-1);
    }

    /* Write the index file header. Note that index data is written in navive
     * machine byte order.  There are 16 bytes of space reserved in the header
     * for future information.
     */
    fwrite(index_header.version, GSF_INDEX_VERSION_SIZE, 1, ft->index_data.fp);
    fwrite(&index_header.gsfFileSize, 4, 1, ft->index_data.fp);
    fwrite(&index_header.endian, 4, 1, ft->index_data.fp);
    fwrite(&index_header.number_record_types, 4, 1, ft->index_data.fp);
    fwrite(&index_header.spare1, 4, 1, ft->index_data.fp);
    fwrite(&index_header.spare2, 4, 1, ft->index_data.fp);
    fwrite(&index_header.spare3, 4, 1, ft->index_data.fp);
    fwrite(&index_header.spare4, 4, 1, ft->index_data.fp);

    /* Set the library's table entry for the number of record types for this file */
    ft->index_data.number_of_types = index_header.number_record_types;

    /*	Clear the space for the information (write dummy info for the
     *	record type, number_of_records, and start address).
     */
    for (i = 0, j = 0; i < ft->index_data.number_of_types; i++)
    {
	fwrite(&j, 4, 1, ft->index_data.fp);
	fwrite(&j, 4, 1, ft->index_data.fp);
	fwrite(&j, 4, 1, ft->index_data.fp);
    }

    /*	Reset the counters. */
    count = 0;
    percent = 0;
    old_percent = -1;
    total_recs = 0;
    for (i = 0; i < NUM_REC_TYPES; i++)
    {
	total_recs += ft->index_data.number_of_records[i];
    }

    /*	Read the temp files and build the final index file.   */
    j = 0;
    for (i = 0; i < NUM_REC_TYPES; i++)
    {
	/*  If the temp file pointer is non-NULL then this record type was
	    encountered.    */

	if (temp[i] != NULL)
	{

	    /*	Rewind the temp file and set the start address and record
	     *	type.
	     */
	    fseek(temp[i], 0, 0);
	    ft->index_data.start_addr[i] = ftell(ft->index_data.fp);
	    ft->index_data.record_type[i] = i;

	    /*	Read through the temp file and write to the final file. */
	    while (fread(&index_rec, sizeof(INDEX_REC), 1, temp[i]) == 1)
	    {
		fwrite(&index_rec, sizeof(INDEX_REC), 1,
		    ft->index_data.fp);

		count++;

#ifdef DISPLAY_SPINNER
		/*  Print the percent spinner to stdout.  */
		percent = ((float) (count) / (float) total_recs) * 100.0;
		if (old_percent != percent)
		{
		    printf("Writing index file - %03d%% complete\r", percent);
		    fflush(stdout);
		    old_percent = percent;
		}
#endif
	    }

	    /*	Move to the beginning of the final file and write the
	     *	pertinent information for this record type.  The offset
	     *	is computed as follows :
	     *	j is the counter for the record types stored in the
	     *	index file, times 12 (size of the header info for each
	     *	record type), plus 44 bytes for the format version
	     *	id, gsf file size, endian indicator, total number of record
	     *	types, and reserved space.
	     */
	    fseek(ft->index_data.fp, (j * 12) + 44, 0);
	    fwrite(&ft->index_data.record_type[i], 4, 1,
		ft->index_data.fp);
	    fwrite(&ft->index_data.start_addr[i], 4, 1,
		ft->index_data.fp);
	    fwrite(&ft->index_data.number_of_records[i], 4, 1,
		ft->index_data.fp);
	    /* Advance to the end of the index file */
	    fseek(ft->index_data.fp, 0, SEEK_END);

	    /*	Get rid of the temp files.  */

	    close_temp_file(i, temp[i]);

	    j++;
	}
    }

    /* Read the scale factor addresses into memory. */
    if (ft->index_data.record_type[0] != -1)
    {
	ft->index_data.scale_factor_addr =
	    (INDEX_REC *) calloc(ft->index_data.number_of_records[0],
	    sizeof(INDEX_REC));

	/*  Couldn't calloc the memory for the scale factor addresses.  */

	if (ft->index_data.scale_factor_addr == NULL)
	{
	    gsfError = GSF_MEMORY_ALLOCATION_FAILED;
	    return (-1);
	}

	fseek(ft->index_data.fp, ft->index_data.start_addr[0], 0);
	for (i = 0; i < ft->index_data.number_of_records[0]; i++)
	{
	    fread(&ft->index_data.scale_factor_addr[i], sizeof(INDEX_REC), 1,
		ft->index_data.fp);
	}
    }

    /*	Set the byte swap indicator off.  No need to swap on a
     *	machine of the same sex.
     */
    ft->index_data.swap = 0;

#ifdef DISPLAY_SPINNER
    printf("                                            \r");
#endif
    return (0);
}

/********************************************************************
 *
 * Function Name : gsfAppendIndexFile
 *
 * Description : This function is called to add addition index records
 *   to an existing index file
 *
 * Inputs :
 *  filename = a fully qualified path to the gsf file
 *  handle = gsf file handle assigned by gsfOpen or gsfOpenBuffered
 *  ft = a pointer to the GSF_FILE_TABLE entry for the file specified by
 *     handle.
 *
 * Returns :
 *  This function returns zero if successful, or -1 if an error occured.
 *
 * Error Conditions :
 *
 ********************************************************************/

static int
gsfAppendIndexFile(const char *ndx_file, int handle, GSF_FILE_TABLE *ft)
{
    int 	     i;
    int 	     j;
    int 	     err;
    int 	     id;
    int              last_record_type;
    int              last_record_number;
    int 	     percent;
    int 	     old_percent;
    long	     last_index=0;
    long	     eof;
    long             current;
    long	     endian = 0x00010203;
    FILE	    *temp[NUM_REC_TYPES];
    gsfDataID	     data_id;
    gsfRecords	     records;
    INDEX_REC	     index_rec;
    GSF_INDEX_HEADER index_header;

    /* Clear the index header data structure */
    memset (&index_header, 0, sizeof(index_header));

    /* Clear the array of temporary file descriptors */
    for (i=0; i<NUM_REC_TYPES; i++)
    {
	temp[i] = (FILE *) NULL;
    }

    /* Read the endian indicator to detect whether to swap incoming data.
     * If the endian indicator is not kosher, error out.
     */
    fread(&index_header.endian, 4, 1, ft->index_data.fp);
    if (index_header.endian == 0x00010203)
    {
	ft->index_data.swap = 0;
    }
    else if (index_header.endian == 0x03020100)
    {
	ft->index_data.swap = 1;
    }
    else
    {
	gsfError = GSF_CORRUPT_INDEX_FILE_ERROR;
	return (-1);
    }

    /* Read the number of record types in the index file. */
    fread(&index_header.number_record_types, 4, 1, ft->index_data.fp);
    if (ft->index_data.swap)
    {
	SwapLong((unsigned long *) &index_header.number_record_types, 1);
    }
    ft->index_data.number_of_types = index_header.number_record_types;

    /* Make sure we found a valid number_of_types */
    if ((ft->index_data.number_of_types < 1) ||
	(ft->index_data.number_of_types > NUM_REC_TYPES))
    {
	gsfError = GSF_CORRUPT_INDEX_FILE_ERROR;
	return (-1);
    }

    /* Read the four four byte reserved fields */
    fread(&index_header.spare1, 4, 1, ft->index_data.fp);
    fread(&index_header.spare2, 4, 1, ft->index_data.fp);
    fread(&index_header.spare3, 4, 1, ft->index_data.fp);
    fread(&index_header.spare4, 4, 1, ft->index_data.fp);

    /*	For each record type, read the record type, start address and
     *	number of records.
     */
    for (i = 0; i < ft->index_data.number_of_types; i++)
    {
	fread(&j, 4, 1, ft->index_data.fp);
	if (ft->index_data.swap)
	{
	    SwapLong((unsigned long *) &j, 1);
	}

	/*  This is not really necessary but it makes things easier to code.
	 *  Put the record type into the record_type[record_type] member of
	 *  the index_data structure.
	 */
	ft->index_data.record_type[j] = j;

	fread(&ft->index_data.start_addr[j], 4, 1,
	    ft->index_data.fp);
	fread(&ft->index_data.number_of_records[j], 4, 1,
	    ft->index_data.fp);

	if (ft->index_data.swap)
	{
	    SwapLong((unsigned long *) &ft->index_data.start_addr[i], 1);
	    SwapLong((unsigned long *) &ft->index_data.number_of_records[i], 1);
	}
    }

    /* Read the index file, and create temporary files with index data for
     * each record type.
     */
    for (i=0; i<NUM_REC_TYPES; i++)
    {
	if (ft->index_data.number_of_records[i] > 0)
	{
            /* Move the index file's pointer to the first record for this type */
	    fseek(ft->index_data.fp, ft->index_data.start_addr[i], 0);
	    for(j=0; j<ft->index_data.number_of_records[i]; j++)
	    {
		/* Read the index record from the disk */
		fread(&index_rec, sizeof(INDEX_REC), 1, ft->index_data.fp);
		if (ft->index_data.swap)
		{
		    SwapLong((unsigned long *) &index_rec.sec,	1);
		    SwapLong((unsigned long *) &index_rec.nsec, 1);
		    SwapLong((unsigned long *) &index_rec.addr, 1);
		}
		if (temp[i] == NULL)
		{
		    temp[i] = open_temp_file(i);
		    if (temp[i] == NULL)
		    {
			gsfError = GSF_FOPEN_ERROR;
			return(-1);
		    }
		}
		fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[i]);

		/* Save the address of the last record indexed */
		if (index_rec.addr > last_index)
		{
		    last_record_type = i;
		    last_record_number = j+1;
		    last_index = index_rec.addr;
		}
	    }
	}
    }

    /* Read the currently indexed scale factor addresses into memory. */
    if (ft->index_data.record_type[0] != -1)
    {
	ft->index_data.scale_factor_addr =
	    (INDEX_REC *) calloc(ft->index_data.number_of_records[0],
	    sizeof(INDEX_REC));

	/*  Couldn't calloc the memory for the scale factor addresses.  */

	if (ft->index_data.scale_factor_addr == NULL)
	{
	    gsfError = GSF_MEMORY_ALLOCATION_FAILED;
	    return (-1);
	}

	fseek(ft->index_data.fp, ft->index_data.start_addr[0], 0);
	for (i = 0; i < ft->index_data.number_of_records[0]; i++)
	{
	    fread(&ft->index_data.scale_factor_addr[i], sizeof(INDEX_REC), 1,
		ft->index_data.fp);
	}
    }

    /* If any ping records have been indexed read the last one. This is
     * required to ensure we have the most recent scale factors loaded.
     */ 
    if (ft->index_data.number_of_records[GSF_RECORD_SWATH_BATHYMETRY_PING])
    {
	data_id.recordID = GSF_RECORD_SWATH_BATHYMETRY_PING;
        data_id.record_number = -1;
        err = gsfRead(handle, data_id.recordID, &data_id, &records, NULL, 0);
        if (err < 0)
        {
	    /* gsfError will have been set in gsfRead */
	    return(-1);
        }
    }

    /* If the last indexed record is not the ping record we just loaded, then 
     * go load the last indexed record so that the gsf file pointer is one
     * record beyond the last record we have an index for. 
     */
    if (last_record_type != GSF_RECORD_SWATH_BATHYMETRY_PING)
    {
        data_id.recordID = last_record_type;
        data_id.record_number = last_record_number;
        err = gsfRead(handle, data_id.recordID, &data_id, &records, NULL, 0);
        if (err < 0)
        {
	    /* gsfError will have been set in gsfRead */
	    return(-1);
        }
    }

    /*	Get the address of the end of file so we can compute percentage
     *	processed for creating the index file.
     */
    eof = ft->file_size;
    percent = 0;
    old_percent = -1;

    /*	Read each gsf record and write the time and address to the
     *	temp index files.
     */
    do
    {
	/*  Save the current address within the gsf file.   */

	if ((index_rec.addr = ftell(ft->fp)) == -1 )
	{
	    gsfError = GSF_FILE_TELL_ERROR;
	    return (-1);
	}

	/*  Read the gsf record and check for end of file.  */

	if ((err = gsfRead(handle, GSF_NEXT_RECORD, &data_id, &records,
		    NULL, 0)) != -1)
	{

	    /*	Switch based on the record type that was just read.  */

	    id = data_id.recordID;
	    switch (id)
	    {
		    /*	Don't index the header or the summary.  */

		case GSF_RECORD_HEADER:
		    break;

		case GSF_RECORD_SWATH_BATHY_SUMMARY:

                    /*  If this is the first record of this type open the
                        temp file and increment the number of types.    */
                    
                    if (temp[id] == NULL)
                    {
                        temp[id] = open_temp_file(id);
			index_header.number_record_types++;
                    }

                    /*  Load the index record structure and write to the
                        temp file.	*/

                    index_rec.sec = records.summary.start_time.tv_sec;
                    index_rec.nsec = records.summary.end_time.tv_nsec;
                    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
                    ft->index_data.number_of_records[id]++;

		    break;

		case GSF_RECORD_SWATH_BATHYMETRY_PING:

		    /*	If this ping record actually contained scale
			factor information in the file, make entries to
			two indexes; the standard ping index and the
			index for pings with scale factor subrecords.	*/

		    if (ft->scales_read)
		    {
			/*  If this is the first record of this type
			    open the temp file and increment the
			    number of types.	*/

			if (temp[0] == NULL)
			{
			    temp[0] = open_temp_file(0);
			    if (temp[0] == NULL)
			    {
				return(-1);
			    }
			    index_header.number_record_types++;
			}

			/*  Load the index record structure and write to the
			    temp file.	*/

			index_rec.sec = records.mb_ping.ping_time.tv_sec;
			index_rec.nsec = records.mb_ping.ping_time.tv_nsec;
			fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[0]);
			ft->index_data.number_of_records[0]++;
		    }

		    /*	If this is the first record of this type open the
			temp file and increment the number of types.	*/

		    if (temp[id] == NULL)
		    {
			temp[id] = open_temp_file(id);
			index_header.number_record_types++;
		    }

		    /*	Load the index record structure and write to the
			temp file.  */

		    index_rec.sec = records.mb_ping.ping_time.tv_sec;
		    index_rec.nsec = records.mb_ping.ping_time.tv_nsec;
		    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
		    ft->index_data.number_of_records[id]++;

		    break;

		case GSF_RECORD_SOUND_VELOCITY_PROFILE:

		    /*	If this is the first record of this type open the
			temp file and increment the number of types.	*/

		    if (temp[id] == NULL)
		    {
			temp[id] = open_temp_file(id);
			index_header.number_record_types++;
		    }

		    /*	Load the index record structure and write to the
			temp file.  */

		    index_rec.sec = records.svp.application_time.tv_sec;
		    index_rec.nsec = records.svp.application_time.tv_nsec;
		    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
		    ft->index_data.number_of_records[id]++;

		    break;

		case GSF_RECORD_PROCESSING_PARAMETERS:

		    /*	If this is the first record of this type open the
			temp file and increment the number of types.	*/

		    if (temp[id] == NULL)
		    {
			temp[id] = open_temp_file(id);
			index_header.number_record_types++;
		    }

		    /*	Load the index record structure and write to the
			temp file.  */

		    index_rec.sec = records.process_parameters.param_time.tv_sec;
		    index_rec.nsec =
			records.process_parameters.param_time.tv_nsec;
		    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
		    ft->index_data.number_of_records[id]++;

		    break;

		case GSF_RECORD_SENSOR_PARAMETERS:

		    /*	If this is the first record of this type open the
			temp file and increment the number of types.	*/

		    if (temp[id] == NULL)
		    {
			temp[id] = open_temp_file(id);
			index_header.number_record_types++;
		    }

		    /*	Load the index record structure and write to the
			temp file.  */

		    index_rec.sec = records.sensor_parameters.param_time.tv_sec;
		    index_rec.nsec =
			records.sensor_parameters.param_time.tv_nsec;
		    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
		    ft->index_data.number_of_records[id]++;

		    break;

		case GSF_RECORD_COMMENT:

		    /*	If this is the first record of this type open the
			temp file and increment the number of types.	*/

		    if (temp[id] == NULL)
		    {
			temp[id] = open_temp_file(id);
			index_header.number_record_types++;
		    }

		    /*	Load the index record structure and write to the
			temp file.  */

		    index_rec.sec = records.comment.comment_time.tv_sec;
		    index_rec.nsec = records.comment.comment_time.tv_nsec;
		    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
		    ft->index_data.number_of_records[id]++;

		    break;

		case GSF_RECORD_HISTORY:

		    /*	If this is the first record of this type open the
			temp file and increment the number of types.	*/

		    if (temp[id] == NULL)
		    {
			temp[id] = open_temp_file(id);
			index_header.number_record_types++;
		    }

		    /*	Load the index record structure and write to the
			temp file.  */

		    index_rec.sec = records.history.history_time.tv_sec;
		    index_rec.nsec = records.history.history_time.tv_nsec;
		    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
		    ft->index_data.number_of_records[id]++;

		    break;

		case GSF_RECORD_NAVIGATION_ERROR:

		    /*	If this is the first record of this type open the
			temp file and increment the number of types.	*/

		    if (temp[id] == NULL)
		    {
			temp[id] = open_temp_file(id);
			index_header.number_record_types++;
		    }

		    /*	Load the index record structure and write to the
			temp file.  */

		    index_rec.sec = records.nav_error.nav_error_time.tv_sec;
		    index_rec.nsec = records.nav_error.nav_error_time.tv_nsec;
		    fwrite(&index_rec, sizeof(INDEX_REC), 1, temp[id]);
		    ft->index_data.number_of_records[id]++;

		    break;

		default:

		    fprintf(stderr, "Unknown record ID %d\n", id);
		    break;
	    }

#ifdef DISPLAY_SPINNER
	    /*	Print the percent spinner to stdout.	*/
	    if ((current = ftell(ft->fp)) == -1 )
	    {
		gsfError = GSF_FILE_TELL_ERROR;
		return (-1);
	    }
	    percent = ((double) current  / (double) eof) * 100.0;
	    if (old_percent != percent)
	    {
		printf("Reading GSF file - %03d%% complete\r", percent);
		fflush(stdout);
		old_percent = percent;
	    }
#endif
	}
	else
	{
	    if (gsfError != GSF_READ_TO_END_OF_FILE)
	    {
		return(-1);
	    }
	}
    }
    while (err != -1);

    /* We currently have the index file opened read-only, close the file and open it
     * for reading and writing.
     */
    fclose (ft->index_data.fp);
    if ((ft->index_data.fp = fopen(ndx_file, "wb+")) == NULL)
    {
	gsfError = GSF_INDEX_FILE_OPEN_ERROR;
	return(-1);
    }

    /* Write the index file header information. NOTE that the data are 
     * in native machine byte order.  There are 16 bytes of
     * space reserved in the header for future information.
     */
    strncpy(index_header.version, GSF_INDEX_VERSION, GSF_INDEX_VERSION_SIZE);
    index_header.gsfFileSize = ft->file_size;
    index_header.endian = endian;
    fwrite(index_header.version, GSF_INDEX_VERSION_SIZE, 1, ft->index_data.fp);
    fwrite(&index_header.gsfFileSize, 4, 1, ft->index_data.fp);
    fwrite(&index_header.endian, 4, 1, ft->index_data.fp);
    fwrite(&index_header.number_record_types, 4, 1, ft->index_data.fp);
    fwrite(&index_header.spare1, 4, 1, ft->index_data.fp);
    fwrite(&index_header.spare2, 4, 1, ft->index_data.fp);
    fwrite(&index_header.spare3, 4, 1, ft->index_data.fp);
    fwrite(&index_header.spare4, 4, 1, ft->index_data.fp);

    /* Set the library's table entry for the number of record types for this file */
    ft->index_data.number_of_types = index_header.number_record_types;

    /*	Clear the space for the information (write dummy info for the
     *	record type, number_of_records, and start address).
     */
    for (i = 0, j = 0; i < ft->index_data.number_of_types; i++)
    {
	fwrite(&j, 4, 1, ft->index_data.fp);
	fwrite(&j, 4, 1, ft->index_data.fp);
	fwrite(&j, 4, 1, ft->index_data.fp);
    }

    /*	Read the temp files and build the final index file.   */
    j = 0;
    for (i = 0; i < NUM_REC_TYPES; i++)
    {
	/*  If the temp file pointer is non-NULL then this record type was
	 *  encountered.
	 */
	if (temp[i] != NULL)
	{

	    /*	Rewind the temp file and set the start address and record
	     *	type.
	     */
	    fseek(temp[i], 0, 0);
	    ft->index_data.start_addr[i] = ftell(ft->index_data.fp);
	    ft->index_data.record_type[i] = i;

	    /*	Read through the temp file and write to the final file. */
	    while (fread(&index_rec, sizeof(INDEX_REC), 1, temp[i]) == 1)
	    {
		fwrite(&index_rec, sizeof(INDEX_REC), 1, ft->index_data.fp);
	    }

	    /*	Move to the beginning of the final file and write the
	     *	pertinent information for this record type.  The offset
	     *	is computed as follows :
	     *	j is the counter for the record types stored in the
	     *	index file, times 12 (size of the header info for each
	     *	record type), plus 44 bytes for the format version
	     *	id, gsf file size, endian indicator, total number of record
	     *	types, and reserved space.
	     */
	    fseek(ft->index_data.fp, (j * 12) + 44, 0);
	    fwrite(&ft->index_data.record_type[i], 4, 1,
		ft->index_data.fp);
	    fwrite(&ft->index_data.start_addr[i], 4, 1,
		ft->index_data.fp);
	    fwrite(&ft->index_data.number_of_records[i], 4, 1,
		ft->index_data.fp);
	    /* Advance to the end of the index file */
	    fseek(ft->index_data.fp, 0, SEEK_END);

	    /*	Get rid of the temp files.  */

	    close_temp_file(i, temp[i]);

	    j++;
	}
    }

    /* Read the scale factor addresses into memory. */
    if (ft->index_data.record_type[0] != -1)
    {
	if (ft->index_data.scale_factor_addr == (INDEX_REC *) NULL)
	{
  	    ft->index_data.scale_factor_addr =
	        (INDEX_REC *) calloc(ft->index_data.number_of_records[0],
	        sizeof(INDEX_REC));
        }
	else
	{
  	    ft->index_data.scale_factor_addr =
	        (INDEX_REC *) realloc(ft->index_data.scale_factor_addr, 
		ft->index_data.number_of_records[0] * sizeof(INDEX_REC));
        }


	/*  Couldn't calloc the memory for the scale factor addresses.  */

	if (ft->index_data.scale_factor_addr == NULL)
	{
	    gsfError = GSF_MEMORY_ALLOCATION_FAILED;
	    return (-1);
	}

	fseek(ft->index_data.fp, ft->index_data.start_addr[0], 0);
	for (i = 0; i < ft->index_data.number_of_records[0]; i++)
	{
	    fread(&ft->index_data.scale_factor_addr[i], sizeof(INDEX_REC), 1,
		ft->index_data.fp);
	}
    }

    /*	Set the byte swap indicator off.  No need to swap on a
     *	machine of the same sex.
     */
    ft->index_data.swap = 0;

    printf("                                            \r");

    return(0);
}

/********************************************************************
 *
 * Function Name : open_temp_file
 *
 * Description : This function creates a temporary file to hold the
 *  temp indexes.  The index file name is /tmp/ concatenated with the
 *  process ID, the record type, and .ndx.
 *
 * Inputs :
 *  type = record type
 *
 * Returns :
 *  This function returns the file pointer for the temp file.
 *
 * Error Conditons : ??????????
 *
 ********************************************************************/

static FILE	      *
open_temp_file(int type)
{
    char	    file[132];
    char            dir[132];
    FILE	   *fp;
    
    memset (&dir, 0, sizeof (dir));

#ifdef OS2
    if (getenv ("GSFTMPDIR") == NULL)
        strcpy (dir, "\\tmp");
    else
        strcpy (dir, getenv ("GSFTMPDIR"));
    
    sprintf(file, "%s\\%05d%02d.ndx", dir, _getpid(), type);
#else
    if (getenv ("GSFTMPDIR") == NULL)
        strcpy (dir, "/tmp");
    else
        strcpy (dir, getenv ("GSFTMPDIR"));
    
    sprintf(file, "%s/%05d%02d.ndx", dir, getpid(), type);
#endif

    if ((fp = fopen(file, "wb+")) == NULL)
    {
	perror(file);
	exit(-1);
    }

    return (fp);
}


/********************************************************************
 *
 * Function Name : close_temp_file
 *
 * Description : This function closes and deletes a temporary index file.
 *
 * Inputs :
 *  type = record type
 *  fp = temp file pointer
 *
 * Returns :
 *  nada
 *
 * Error Conditons : ??????????
 *
 ********************************************************************/

static void
close_temp_file(int type, FILE * fp)
{
    char	    file[132];
    char            dir[132];

    fclose(fp);
    memset (&dir, 0, sizeof(dir));
    if (getenv ("GSFTMPDIR") == NULL)
        strcpy (dir, "/tmp");
    else
        strcpy (dir, getenv ("GSFTMPDIR"));
                
    sprintf(file, "%s/%05d%02d.ndx", dir, getpid(), type);
    unlink(file);

    return;
}

/********************************************************************
 *
 * Function Name : gsfCloseIndex
 *
 * Description : This function closes the index file associated with
 *  the gsf file handle passed.
 *
 * Inputs :
 *  handle = gsf file handle assigned by gsfOpen or gsfOpenBuffered
 *
 * Returns :
 *  This function returns zero if successfull, or -1
 *
 * Error Conditions :
 *
 ********************************************************************/

int
gsfCloseIndex(GSF_FILE_TABLE *ft)
{
    if (fclose(ft->index_data.fp))
    {
	gsfError = GSF_FILE_CLOSE_ERROR;
	return (-1);
    }

    /* Free the memory allocated for the scale factor addresses. */
    if (ft->index_data.scale_factor_addr != (INDEX_REC *) NULL)
    {
	free(ft->index_data.scale_factor_addr);
	ft->index_data.scale_factor_addr = (INDEX_REC *) NULL;
    }

    return(0);
}

/******************************************************************************\
*									       *
*   Programmer(s):	David B. Johanson				       *
*									       *
*   Date Written:	February, 1994					       *
*									       *
*   Module Name:	SwapLong					       *
*									       *
*   Module Security							       *
*   Classification:	Unclassified					       *
*									       *
*   Data Security							       *
*   Classification:	Unknown 					       *
*									       *
*   Purpose:		This function swaps four-byte words.		       *
*									       *
*   Inputs:		The base address of the bytes to be swapped and        *
*			the number of words to swap.			       *
*									       *
*   Outputs:		The swapped bytes (by reference)		       *
*									       *
*   Calling Routines:	main						       *
*									       *
*   Routines Called:	None						       *
*									       *
*   Glossary:		longvalue	- the word to be byte-swapped	       *
*			bytevalue	- bytes within the longword	       *
*			data		- union of longvalue, bytevalue        *
*			swap_address	- address of word to be swapped        *
*			byte0		- temporary storage		       *
*			byte1		- temporary storage		       *
*			i		- counter, # of words swapped	       *
*									       *
*   Method:		Load short value with the next word to be swapped      *
*			swap the bytes, repeat till done.		       *
*									       *
*   History:		dbj, 2/94: Modified from function written by D. Avery. *
*			dbj, 6/94: Renamed from SwapBytes to SwapLong.	Added  *
*				  use of typedefs plus other minor mods.       *
*									       *
\******************************************************************************/

void
SwapLong(unsigned long *base_address, long count)
{
    union
    {
	unsigned long	longvalue;	/* the longword to swap 	       */
	unsigned char	bytevalue[4];	/* bytes within the longword	       */
    }
    data;

    unsigned char   byte0,	/* temporary storage		       */
		    byte1;	/* temporary storage		       */
    long	    i;		/* counter for number of words swapped */

    for (i = 0; i < count; i++)
    {
	/* Swap the bytes.							*/

	data.longvalue = *(base_address + i);
	byte0 = data.bytevalue[0];
	byte1 = data.bytevalue[1];
	data.bytevalue[0] = data.bytevalue[3];
	data.bytevalue[1] = data.bytevalue[2];
	data.bytevalue[2] = byte1;
	data.bytevalue[3] = byte0;
	*(base_address + i) = data.longvalue;
    }
    return;
}