
/* Begin user code block <abstract> */
/*--------------------------------------------------------------------
 *    The MB-system:	mbgrdviz_main.c		10/9/2002
 *    $Id$
 *
 *    Copyright (c) 2002-2009 by
 *    David W. Caress (caress@mbari.org)
 *      Monterey Bay Aquarium Research Institute
 *      Moss Landing, CA 95039
 *    and Dale N. Chayes (dale@ldeo.columbia.edu)
 *      Lamont-Doherty Earth Observatory
 *      Palisades, NY 10964
 *
 *    See README file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/
/* End user code block <abstract> */

/**
 * README: Portions of this file are merged at file generation
 * time. Edits can be made *only* in between specified code blocks, look
 * for keywords <Begin user code> and <End user code>.
 */
/*
 * Generated by the ICS Builder Xcessory (BX).
 *
 * BuilderXcessory Version 6.1.3
 * Code Generator Xcessory 6.1.3 (08/19/04) CGX Scripts 6.1 Motif 2.1 
 *
 */


/* Begin user code block <file_comments> */
/* #define MBGRDVIZ_DEBUG 1 */
/* End user code block <file_comments> */

/*
 * Motif required Headers
 */
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/RepType.h>
#include <Xm/MwmUtil.h>

/**
 * Globally included information.
 */


/*
 * Headers for classes used in this program
 */

/**
 * Common constant and pixmap declarations.
 */
#include "mbgrdviz_creation.h"

/**
 * Convenience functions from utilities file.
 */
extern void RegisterBxConverters(XtAppContext);
extern XtPointer BX_CONVERT(Widget, char *, char *, int, Boolean *);
extern XtPointer BX_DOUBLE(double);
extern XtPointer BX_SINGLE(float);
extern void BX_MENU_POST(Widget, XtPointer, XEvent *, Boolean *);
extern Pixmap XPM_PIXMAP(Widget, char **);
extern void BX_SET_BACKGROUND_COLOR(Widget, ArgList, Cardinal *, Pixel);

/**
 * Declarations for shell callbacks.
 */
extern void do_mbgrdviz_quit(Widget, XtPointer, XtPointer);
extern void BxExitCB(Widget, XtPointer, XtPointer);

/* Begin user code block <globals> */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* MBIO include files */
#include "../../include/mb_status.h"
#include "../../include/mb_define.h"

/* mbview include file */
#include "mbview.h"

/* global mbview variables */
Widget mainWindow;

/* function prototypes */
Widget CreatemainWindow_mbgrdviz(Widget parent);
int do_mbgrdviz_init(int argc, char **argv, int verbosity);
void do_mbgrdviz_sensitivity();
int do_mbgrdviz_dismiss_notify(int instance);
void do_mbgrdviz_fileSelectionBox( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_fileSelectionBox_openoverlay( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_fileSelectionBox_openroute( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_fileSelectionBox_opensite( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_fileSelectionBox_opennav( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_fileSelectionBox_openswath( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_fileSelectionBox_saveroute( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_fileSelectionBox_savewinfrogpts( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_fileSelectionBox_savewinfrogwpt( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_fileSelectionBox_savesite( Widget w, XtPointer client_data, XtPointer call_data);
int do_mbgrdviz_openprimary(char *input_file_ptr);
int do_mbgrdviz_openoverlay(int instance, char *input_file_ptr);
int do_mbgrdviz_opensite(int instance, char *input_file_ptr);
int do_mbgrdviz_savesite(int instance, char *output_file_ptr);
int do_mbgrdviz_openroute(int instance, char *input_file_ptr);
int do_mbgrdviz_saveroute(int instance, char *output_file_ptr);
int do_mbgrdviz_savewinfrogpts(int instance, char *output_file_ptr);
int do_mbgrdviz_savewinfrogwpt(int instance, char *output_file_ptr);
int do_mbgrdviz_saveprofile(int instance, char *output_file_ptr);
int do_mbgrdviz_opennav(int instance, int swathbounds, char *input_file_ptr);
int do_mbgrdviz_readnav(int instance, char *swathfile, 
				int pathstatus, char *pathraw, char *pathprocessed,
				int format, int formatorg, double weight, int *error);
int do_mbgrdviz_readgrd(int instance, char *grdfile,
			int	*grid_projection_mode,
			char	*grid_projection_id,
			float	*nodatavalue,
			int	*nxy,
			int	*nx,
			int	*ny,
			double	*min,
			double	*max,
			double	*xmin,
			double	*xmax,
			double	*ymin,
			double	*ymax,
			double	*dx,
			double	*dy,
			float	**data);
int do_mbgrdviz_opentest(int instance, 
			double	factor1, 
			double	factor2, 
			double	factor3, 
			int	*grid_projection_mode,
			char	*grid_projection_id,
			float	*nodatavalue,
			int	*nxy,
			int	*nx,
			int	*ny,
			double	*min,
			double	*max,
			double	*xmin,
			double	*xmax,
			double	*ymin,
			double	*ymax,
			double	*dx,
			double	*dy,
			float	**data);
void do_mbgrdviz_open_region( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_open_mbeditviz( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_make_survey( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_generate_survey( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_arearoute_dismiss( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_arearoute_parameterchange( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_arearoute_recalc(int instance);
void do_mbgrdviz_arearoute_info(int instance);
void do_mbgrdviz_arearoute_linespacing_increment( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_arearoute_altitude_increment( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_arearoute_depth_increment( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_realtime_start( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_realtimesetup_path_reset( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_realtime_pause( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_realtime_stop( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_realtime_resume( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_realtimesetup_path_apply( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_realtimesetup_icon( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_realtimesetup_path_browse( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_realtimesetup_updaterate( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_realtimesetup_path_test( Widget w, XtPointer client_data, XtPointer call_data);
void do_mbgrdviz_realtimesetup_pathmode( Widget w, XtPointer client_data, XtPointer call_data);

static char rcs_id[] = "$Id$";
static char program_name[] = "MBgrdviz";
static char help_message[] =  "MBgrdviz provides simple interactive 2D/3Dvizualization of GMT grids.";
static char usage_message[] = "mbgrdviz [-Igrdfile -T -V -H]";
char	ifile[MB_PATH_MAXLINE];
char	jfile[MB_PATH_MAXLINE];

/* parsing variables */
extern char *optarg;
extern int optkind;
int	errflg = 0;
int	c;
int	help = 0;
int	flag = 0;

/* use these parameters only when debugging X events */
#ifdef MBGRDVIZ_DEBUG    
char	eventname[64];
XEvent	event;
XAnyEvent *xany;
XKeyEvent *xkey;
XButtonEvent *xbutton;
XMotionEvent *xmotion;
XCrossingEvent *xcrossing;
XFocusChangeEvent *xfocus;
XExposeEvent *xexpose;
XGraphicsExposeEvent *xgraphicsexpose;
XNoExposeEvent *xnoexpose;
XVisibilityEvent *xvisibility;
XCreateWindowEvent *xcreatewindow;
XDestroyWindowEvent *xdestroywindow;
XUnmapEvent *xunmap;
XMapEvent *xmap;
XMapRequestEvent *xmaprequest;
XReparentEvent *xreparent;
XConfigureEvent *xconfigure;
XGravityEvent *xgravity;
XResizeRequestEvent *xresizerequest;
XConfigureRequestEvent *xconfigurerequest;
XCirculateEvent *xcirculate;
XCirculateRequestEvent *xcirculaterequest;
XPropertyEvent *xproperty;
XSelectionClearEvent *xselectionclear;
XSelectionRequestEvent *xselectionrequest;
XSelectionEvent *xselection;
XColormapEvent *xcolormap;
XClientMessageEvent *xclient;
XMappingEvent *xmapping;
XErrorEvent *xerror;
XKeymapEvent *xkeymap;
#endif

/* End user code block <globals> */

/**
 * Change this line via the Output Application Names Dialog.
 */
#define BX_APP_CLASS "MB-System"

int main( int argc, char **argv)
{
    Widget       parent;
    XtAppContext app;
    Arg          args[256];
    Cardinal     ac;
     Widget   topLevelShell;
    Widget   mainWindow_mbgrdviz;
    
    /* Begin user code block <declarations> */
	int	error = MB_ERROR_NO_ERROR;
	int	verbose = 0;
	int	ifileflag = 0;
	int	jfileflag = 0;
	int	testflag = 0;

	/* process argument list */
	while ((c = getopt(argc, argv, "VvHhI:i:J:j:Tt")) != -1)
	  {
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
		case 'I':
		case 'i':
			sscanf (optarg,"%s", ifile);
			flag++;
			ifileflag++;
			break;
		case 'J':
		case 'j':
			sscanf (optarg,"%s", jfile);
			flag++;
			jfileflag++;
			break;
		case 'T':
		case 't':
			flag++;
			testflag++;
			break;
		case '?':
			errflg++;
		}
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

	/* print starting message */
	if (help)
		{
		fprintf(stderr,"\n%s\n\nUsage: %s\n", help_message, usage_message);
		error = MB_ERROR_NO_ERROR;
		exit(error);
		}
    
    /* End user code block <declarations> */
    
    /*
     * Initialize Xt. 
     */
    
    XtSetLanguageProc(NULL, (XtLanguageProc) NULL, NULL); 
    
    /*
     * The applicationShell is created as an unrealized
     * parent for multiple topLevelShells.  The topLevelShells
     * are created as popup children of the applicationShell.
     * This is a recommendation of Paul Asente & Ralph Swick in
     * _X_Window_System_Toolkit_ p. 677.
     */
    
    parent = XtVaOpenApplication ( &app, 
                                   BX_APP_CLASS, 
                                   NULL, 
                                   0, 
                                   &argc, 
                                   argv, 
                                   NULL, 
                                   sessionShellWidgetClass, 
                                   NULL );
    
    RegisterBxConverters(app);
    XmRepTypeInstallTearOffModelConverter();
    
    /* Begin user code block <create_shells> */
    /* End user code block <create_shells> */
    
    /*
     * Create classes and widgets used in this program. 
     */
    
    /* Begin user code block <create_topLevelShell> */
    /* End user code block <create_topLevelShell> */
    
    ac = 0;
    XtSetArg(args[ac], XmNtitle, "MBgrdviz"); ac++;
    XtSetArg(args[ac], XmNx, 108); ac++;
    XtSetArg(args[ac], XmNy, 153); ac++;
    XtSetArg(args[ac], XmNwidth, 260); ac++;
    XtSetArg(args[ac], XmNheight, 215); ac++;
    topLevelShell = XtCreatePopupShell((char *)"topLevelShell",
        topLevelShellWidgetClass,
        parent,
        args, 
        ac);
    XtAddCallback(topLevelShell, XmNdestroyCallback, do_mbgrdviz_quit, (XtPointer)0);
    XtAddCallback(topLevelShell, XmNdestroyCallback, BxExitCB, (XtPointer)0);
    mainWindow_mbgrdviz = (Widget)CreatemainWindow_mbgrdviz(topLevelShell);
    XtManageChild(mainWindow_mbgrdviz);
    XtPopup(XtParent(mainWindow_mbgrdviz), XtGrabNone);
    
    /* Begin user code block <app_procedures> */
    
    /* set top level widget */
    mainWindow = mainWindow_mbgrdviz;
    
    /* End user code block <app_procedures> */
    
    /* Begin user code block <main_loop> */
    
    /* initialize the vizualization widgets code */
    mbview_startup(verbose, parent, app, &error);
    
    /* open the file specified on the command line */
    do_mbgrdviz_init(argc,argv, verbose);
    if (ifileflag > 0)
    	{
    	do_mbgrdviz_openprimary(ifile);
	if (jfileflag > 0)
		{
		do_mbgrdviz_openoverlay(0,jfile);
		}
	}
    else if (testflag > 0)
    	{
        do_mbgrdviz_openprimary(NULL);
	}
    
    /* End user code block <main_loop> */
    
    XtAppMainLoop(app);
    
    /*
     * A return value regardless of whether or not the main loop ends. 
     */
     return(0); 
}
