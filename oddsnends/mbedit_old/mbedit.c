/************************************************************/
/*                                                          */
/* mbedit.c                                                 */
/*                                                          */
/* THIS PROGRAM ALLOWS YOU TO EDIT THE MULTI-BEAM DATA      */
/*                                                          */
/************************************************************/
/*                                                          */
/* LANGUAGE "C"                                             */
/*                                                          */
/* WRITTEN FOR MOTIF                                        */
/*                                                          */
/************************************************************/

/************************************************************/
/* INCLUDE FILES                                            */
/************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <math.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/MainW.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/MessageB.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#include <Xm/Scale.h>
#include <Xm/FileSB.h>
#include <Xm/RowColumn.h>
#include <Mrm/MrmAppl.h>
#include <Mrm/MrmPublic.h>
#include <Mrm/MrmDecls.h>

#include "mbedit_define.c"

#define MAX_WIDGETS (k_max_widget + 1)
#define ERROR (-1)

#define font1 "-adobe-helvetica-bold-r-normal--14-140-75-75-p-82-iso8859-1"
#define font2 "-misc-fixed-bold-r-normal-*-13-*-75-75-c-70-iso8859-1"
#define font3 "-adobe-courier-medium-r-normal--18-180-75-75-m-100-iso8859-1"

#define charset XmSTRING_DEFAULT_CHARSET

#define EV_MASK (ButtonPressMask | KeyPressMask | ExposureMask )

/************************************************************/
/* GLOBAL VARIABLES                                         */
/************************************************************/
XtAppContext app_context;
static Widget toplevel_widget, main_window_widget;
static Widget widget_array[MAX_WIDGETS]; 
static int toggle_array[MAX_WIDGETS];
Display *display, *dpy, *theDisplay;
Screen *screen, *can_screen, screen_tl;
Window win7, window, frm_xid, can_xid, window_tl, theWindow;
Window root_return, child_return;
Colormap theColormap, colormap;
GC gc, main_gc;
unsigned int theCursorShape;
Cursor theCursor;
XGCValues xgcv;
Pixmap theIconPixmap, crosshairPix;
XWMHints theWMHints;
XSizeHints theSizeHints;
Drawable draw;

XFontStruct *fontStruct;

/* Global mbedit definitions */
int	plot_size_max;
int	mplot_size;
int	buffer_size_max;
int	buffer_size;
int	hold_size;
int	mformat;
int	step = 5;
int	nloaded;
int	ndumped;
int	nbuffer;
int	ngood;
int	icurrent;
int	mnplot;
int	mscale_max;
int	exager;
int	mxscale;
int	myscale;
int	mx_interval;
int	my_interval;
int	mode_pick = 0;
int	ttime_i[7];
int	status;

/* file opening parameters */
int	startup_file = 0;
int	open_files_count = 0;
struct direct **open_files;

/* Values set when "MODE" is selected */
#define	MODE_PICK	0
#define	MODE_ERASE	1
#define	MODE_RESTORE	2

int screen_num, can_screen_num;
int theDepth;

static XmString latin_zero;
static MrmHierarchy s_MrmHierarchy;
static MrmType dummy_class;

static char	*input_file;
static char	output_file[128];
int selected = 0; /* indicates an input file is selected */

int	can_xgid;		/* XG graphics id */
Cursor myCursor;
XColor closest[2];
XColor exact[2];

/* Set the colors used for this program here. */
#define NCOLORS 6
XColor colors[NCOLORS];
unsigned long mpixel_values[NCOLORS];
XColor db_color;

/* Set these to the dimensions of your canvas drawing */
/* area, minus 1, located in mbedit.uil.              */
static int mb_borders[4] =
	{ 0, 1096, 0, 629 };

/************************************************************/
/* END OF GLOBAL VARIABLES                                  */
/************************************************************/

/************************************************************/
/* LIST ALL UID FILES HERE                                  */
/************************************************************/
#include "mbedit_uid_loc.h"

/************************************************************/
/* AUTOMATICALLY DETERMINE THE NUMBER OF UID FILES          */
/************************************************************/

static int db_filename_num = (sizeof db_filename_vec /
                              sizeof db_filename_vec[0]);

/************************************************************/
/* DECLARE FUNCTIONS                                        */
/************************************************************/

static void s_error();
static void create_proc();
static void init_data();
static void setup_data();
static void display_menu();
static void do_load_ok();
static void cancel_file_sel();
static void do_next_buffer();
static void do_buffer_size();
static void do_buffer_hold();
static void do_done();
static void do_quit();
static void do_forward();
static void do_reverse();
static void set_number_pings();
static void set_scale_x();
static void set_scale_y();
static void set_number_step();
static void set_mode_pick();
static void set_mode_erase();
static void set_mode_restore();
static void apply_goto_button();
static void cancel_goto_button();
static void do_x_interval();
static void do_y_interval();
static void do_event();
static void grab_file();
int xg_init();
void xg_free();
void xg_getpixelvalue();
void xg_drawpoint();
void xg_drawline();
void xg_drawrectangle();
void xg_drawtriangle();
void xg_fillrectangle();
void xg_filltriangle();
void xg_drawstring();
void xg_justify();

/************************************************************/
/* LIST THE CALLBACK PROCEDURES USED IN THE UIL FILES AND   */
/* THEIR ADDRESSES SO DRM CAN BIND THEM.                    */
/************************************************************/

static MrmRegisterArg reglist[] = 
	{
	  {"create_proc",		(caddr_t) create_proc},
	  {"do_load_ok",		(caddr_t) do_load_ok},
	  {"cancel_file_sel",		(caddr_t) cancel_file_sel},
	  {"do_next_buffer",		(caddr_t) do_next_buffer},
	  {"do_buffer_size",		(caddr_t) do_buffer_size},
	  {"do_buffer_hold",		(caddr_t) do_buffer_hold},
	  {"do_done",			(caddr_t) do_done},
	  {"do_quit",			(caddr_t) do_quit},
	  {"do_forward",		(caddr_t) do_forward},
	  {"do_reverse",		(caddr_t) do_reverse},
	  {"set_number_pings",		(caddr_t) set_number_pings},
	  {"set_scale_x",		(caddr_t) set_scale_x},
	  {"set_scale_y",		(caddr_t) set_scale_y},
	  {"set_number_step",		(caddr_t) set_number_step},
	  {"set_mode_pick",		(caddr_t) set_mode_pick},
	  {"set_mode_erase",		(caddr_t) set_mode_erase},
	  {"set_mode_restore",		(caddr_t) set_mode_restore},
	  {"apply_goto_button",		(caddr_t) apply_goto_button},
	  {"cancel_goto_button",	(caddr_t) cancel_goto_button},
	  {"do_x_interval",		(caddr_t) do_x_interval},
	  {"do_y_interval",		(caddr_t) do_y_interval},
	  {"do_event",			(caddr_t) do_event},
	  {"grab_file",			(caddr_t) grab_file},
	  {"display_menu",		(caddr_t) display_menu}
	};


/************************************************************/
/* AUTOMATICALLY DETERMINE THE NUMBER OF PROCEDURES TO BIND */
/************************************************************/

static int reglist_num = (sizeof reglist / sizeof reglist[0]);

/************************************************************/
/* SET OPEN STATEMENT FOR FILES                             */
/************************************************************/

FILE *fopen();

/************************************************************/
/* main PROGRAM                                             */
/************************************************************/
unsigned int main(argc, argv)

	int argc;
	String		argv[];
{
	int n;
	Arg arglist[2];

	Cardinal i;


/************************************************************/
/* INITIALIZE DRM                                           */
/************************************************************/

	MrmInitialize();

/************************************************************/
/* INITIALIZE THE X TOOLKIT - WE GET BACK THE TOP LEVEL     */
/* SHELL WIDGET                                             */
/************************************************************/


	XtToolkitInitialize();

	app_context = XtCreateApplicationContext();

	display = XtOpenDisplay(app_context,
		  NULL,
		  argv[0],
		  "MBEDIT",
		  NULL,
		  0,
		  &argc,
		  argv);

	if(display == NULL)
	{
	 fprintf(stderr, "%s: CAN'T OPEN DISPLAY", argv[0]);
	 exit(1);
	}

	n = 0;
	XtSetArg(arglist[n], XmNallowShellResize, TRUE);
	n++;

	toplevel_widget = XtAppCreateShell(
			  argv[0],
			  NULL,
			  applicationShellWidgetClass,
			  display,
			  arglist,
			  n);

/************************************************************/
/* OPEN THE UID FILES                                       */
/************************************************************/

	if (MrmOpenHierarchy( db_filename_num,
			      db_filename_vec,
			      NULL,
			      &s_MrmHierarchy)
		!=MrmSUCCESS) 
		{
		  s_error("CAN'T OPEN HIERARCHY");
		}

/************************************************************/
/* RUN FUNCTION TO INITIALIZE DATA                          */
/************************************************************/

	init_data();

/************************************************************/
/* REGISTER THE NAMES OF THE PROCEDURES THAT DRM NEEDS TO   */
/* BIND.                                                    */
/************************************************************/

	MrmRegisterNames(reglist, reglist_num);

/************************************************************/
/* FETCH THE MAIN WINDOW WIDGET FROM THE UID FILE.          */
/************************************************************/

	if (MrmFetchWidget(s_MrmHierarchy,
			   "window_mbedit",
			   toplevel_widget,
			   &main_window_widget,
			   &dummy_class)
		!= MrmSUCCESS)
		{
		  s_error("CAN'T FETCH MAIN WINDOW");
		}


/************************************************************/
/* MANAGE THE MAIN WINDOW                                   */
/************************************************************/


	XtManageChild(main_window_widget);

/************************************************************/
/* NOW REALIZE THE TOP LEVEL WIDGET - THIS WILL DISPLAY     */
/* THE WIDGET.                                              */
/************************************************************/

	XtRealizeWidget(toplevel_widget);

/************************************************************/
/* THESE WIDGETS ARE FETCHED HERE SO THAT THEY CAN BE       */
/* INITIALIZED BY OTHER PROCESSES EVEN IF THEY HAD NOT BEEN */
/* USED PREVIOUSLY.                                         */
/************************************************************/

	if(widget_array[k_mb_main] == NULL)
	{
	  if(MrmFetchWidget(s_MrmHierarchy,
			    "mbedit_bboard",
			    toplevel_widget,
			    &widget_array[k_mb_main],
	   		    &dummy_class)
		!=MrmSUCCESS)
		{
		  s_error("CAN'T FETCH B BOARD");
		}
	  XtManageChild(widget_array[k_mb_main]);

	}

/**************************************************************/
/* SET UP DISPLAYS SCREENS FONTS AND CURSORS FOR THIS DISPLAY */
/**************************************************************/

	/* Setup the entire screen. */
	display = XtDisplay(widget_array[k_mb_main]);
	screen = DefaultScreenOfDisplay(display);
	screen_num = XDefaultScreen(display);
	frm_xid = XtWindow(widget_array[k_mb_main]);
	colormap = DefaultColormap(display,screen_num);

	/* Setup just the "canvas" part of the screen. */
	theDisplay = XtDisplay(widget_array[k_main_graph]);
	can_screen = DefaultScreenOfDisplay(theDisplay);
	can_xid = XtWindow(widget_array[k_main_graph]);
	can_screen_num = XDefaultScreen(theDisplay);
	theColormap = DefaultColormap(display,can_screen_num);

	/* Setup the "graphics Context" for just the "canvas" */
	xgcv.background = WhitePixelOfScreen(can_screen);
	xgcv.foreground = BlackPixelOfScreen(can_screen);
	xgcv.line_width = 2;
	gc = XCreateGC(theDisplay,can_xid,GCBackground | GCForeground 
		 | GCLineWidth, &xgcv);

	/* Setup the font for just the "canvas" screen. */
	fontStruct = XLoadQueryFont(theDisplay, 
		 "-misc-fixed-bold-r-normal-*-13-*-75-75-c-70-iso8859-1");
	XSetFont(theDisplay,gc,fontStruct->fid);

	XSelectInput(theDisplay, can_xid, EV_MASK );

	/* Load the colors that will be used in this program. */
	status = XLookupColor(theDisplay,colormap,
		"white",&db_color,&colors[0]);
	if(status != 0)
		status = XAllocColor(theDisplay,colormap,&colors[0]);
	if (status == 0)
		{
		fprintf(stderr,"Failure to allocate color: white\n");
		exit(-1);
		}
	status = XLookupColor(theDisplay,colormap,
		"black",&db_color,&colors[1]);
	if(status != 0)
		status = XAllocColor(theDisplay,colormap,&colors[1]);
	if (status == 0)
		{
		fprintf(stderr,"Failure to allocate color: black\n");
		exit(-1);
		}
	status = XLookupColor(theDisplay,colormap,
		"red",&db_color,&colors[2]);
	if(status != 0)
		status = XAllocColor(theDisplay,colormap,&colors[2]);
	if (status == 0)
		{
		fprintf(stderr,"Failure to allocate color: red\n");
		exit(-1);
		}
	status = XLookupColor(theDisplay,colormap,
		"green",&db_color,&colors[3]);
	if(status != 0)
		status = XAllocColor(theDisplay,colormap,&colors[3]);
	if (status == 0)
		{
		fprintf(stderr,"Failure to allocate color: green\n");
		exit(-1);
		}
	status = XLookupColor(theDisplay,colormap,
		"blue",&db_color,&colors[4]);
	if(status != 0)
		status = XAllocColor(theDisplay,colormap,&colors[4]);
	if (status == 0)
		{
		fprintf(stderr,"Failure to allocate color: blue\n");
		exit(-1);
		}
	status = XLookupColor(theDisplay,colormap,
		"coral",&db_color,&colors[5]);
	if(status != 0)
		status = XAllocColor(theDisplay,colormap,&colors[5]);
	if (status == 0)
		{
		fprintf(stderr,"Failure to allocate color: coral\n");
		exit(-1);
		}
	for (i=0;i<NCOLORS;i++)
		{
		mpixel_values[i] = colors[i].pixel;
		}

	/* Setup initial cursor. This will be changed when changing "MODE". */
	myCursor = XCreateFontCursor(theDisplay, XC_target);
	XAllocNamedColor(theDisplay,colormap,"red",&closest[0],&exact[0]);
	XAllocNamedColor(theDisplay,colormap,"coral",&closest[1],&exact[1]);
	XRecolorCursor(theDisplay,myCursor,&closest[0],&closest[1]);
	XDefineCursor(theDisplay,can_xid,myCursor);


/************************************************************/
/* RUN FUNCTION TO SETUP THE SCREENS                        */
/************************************************************/

	/* initialize graphics */
	can_xgid = xg_init(theDisplay, can_xid, mb_borders, font2 );

	status = mbedit_set_graphics(can_xgid, mb_borders, 
			NCOLORS, mpixel_values);

	/* initialize mbedit proper */
	status = mbedit_init(argc,argv,&startup_file);

	setup_data();


/************************************************************/
/* NOW SIT IN MAIN LOOP AND WAIT FOR THE USER TO INPUT      */
/* DATA INTO THE USER INTERFACE                             */
/************************************************************/

	XtAppMainLoop(app_context);

}
/************************************************************/
/************************************************************/


/************************************************************/
/* THIS IS THE END OF THE MAIN PROGRAM                      */
/************************************************************/

/************************************************************/
/* INITIALIZE THE widget array AND THE toggle_array TO NULL */
/* AND 0. THE WIDGET ARRAYS WILL BE SET WHEN THEY ARE       */
/* MANAGED. THE TOGGLE ARRAYS WILL BE USED FOR THE MODE     */
/* SELECTIONS.                                              */
/************************************************************/
static void init_data()
{
	int i;

	for (i = 0; i < MAX_WIDGETS;	 i++)
	  {
	    widget_array[i] = NULL;
	    toggle_array[i] = 0;
	  }

}


/************************************************************/
/* SETUP THE DISPLAYS                                       */
/************************************************************/
static void setup_data()
{
	char time_text[10];

	/* get some default values from mbedit */
	status = mbedit_get_defaults(&plot_size_max,&mplot_size,
			&buffer_size_max,&buffer_size,
			&hold_size,&mformat,
			&mscale_max,&mxscale,&myscale,
			&mx_interval,&my_interval,ttime_i);
	exager = 100*mxscale/myscale;

	/* set values in widgets */
/*	sprintf(time_text,"%4.4d",ttime_i[0]);
	fprintf(stderr,"time_text: %s\n",time_text);
	XmTextSetString(widget_array[k_glf_year], time_text);

	sprintf(time_text,"%2.2d",ttime_i[1]);
	fprintf(stderr,"time_text: %s\n",time_text);
	XmTextSetString(widget_array[k_glf_month], time_text);

	sprintf(time_text,"%2.2d",ttime_i[2]);
	fprintf(stderr,"time_text: %s\n",time_text);
	XmTextSetString(widget_array[k_glf_day], time_text);

	sprintf(time_text,"%2.2d",ttime_i[3]);
	fprintf(stderr,"time_text: %s\n",time_text);
	XmTextSetString(widget_array[k_glf_hour], time_text);

	sprintf(time_text,"%2.2d",ttime_i[4]);
	fprintf(stderr,"time_text: %s\n",time_text);
	XmTextSetString(widget_array[k_glf_min], time_text);

	sprintf(time_text,"%2.2d",ttime_i[5]);
	fprintf(stderr,"time_text: %s\n",time_text);
	XmTextSetString(widget_array[k_glf_sec], time_text);
*/

}


/************************************************************/
/* THIS DISPLAYS THE PRELIMINARY ERROR MESSAGES.            */
/* ADDITIONAL ERROR DIALOG BOXES WILL BE ADDED TO CHECK AND */
/* NOTIFY THE USER IF BAD DATA WAS ENTERED INTO ANY FIELD.  */
/************************************************************/

static void s_error(problem_string)
	char *problem_string;
	{
	  printf("%s\n", problem_string);
	  exit(0);
	}


/************************************************************/
/* THIS FUNCTION CHANGES THE WIDGET ARRAY VALUE FROM "NULL" */
/* TO THE int VALUE ASSIGNED TO IT. THIS WAY YOU KNOW ITS   */
/* BEEN CALLED AND WON'T RECREATE IT AND WASTE MEMORY SPACE.*/
/************************************************************/

static void create_proc(w, tag, reason)
	Widget w;
	int *tag;
	unsigned long *reason;
{
	  int widget_num = *tag;
	  widget_array[widget_num] = w;

	switch (widget_num)
	  {
	  case k_pick_button:
	     {
		XmToggleButtonSetState(widget_array[k_pick_button], 1, FALSE);
	     }
	  break;
	  case k_erase_button:
	     {
		XmToggleButtonSetState(widget_array[k_erase_button], 0, FALSE);
	     }
	  break;
	  case k_restore_button:
	     {
		XmToggleButtonSetState(widget_array[k_restore_button], 0, FALSE);
	     }
	  break;
	defualt:
	  break;
	  }
}


/********************************************************************/
/* THIS FUNCTION IS USED TO DISPLAY MENUS CALLED FROM PULLDOWN MENUS*/
/********************************************************************/
static void display_menu(w, tag, list)
	Widget w;
	int *tag;
	XmListCallbackStruct *list;
{
	int widget_num = *tag;

	switch (widget_num)
	{

	  /* Display the Giant Leap Forward Menu. */
	  case k_display_goto_menu:
	  {
	    if(widget_array[k_goto_menu] == NULL)
            {
              if(MrmFetchWidget(s_MrmHierarchy,
                            "goto_menu",
                            toplevel_widget,
                            &widget_array[k_goto_menu],
                            &dummy_class)
                !=MrmSUCCESS)
                {
                  s_error("CAN'T FETCH GOTO MENU");
                }

              XtManageChild(widget_array[k_goto_menu]);
             }
	     else
	     {
              XtUnmanageChild(widget_array[k_goto_menu]);
              XtManageChild(widget_array[k_goto_menu]);
	     }
	  break;
	  }

	  /* display the file selection menu. */
	  case k_file_menu:
	  {
	    if(widget_array[k_file_menu] == NULL)
            {
              if(MrmFetchWidget(s_MrmHierarchy,
                            "controls_load",
                            toplevel_widget,
                            &widget_array[k_file_menu],
                            &dummy_class)
                !=MrmSUCCESS)
                {
                  s_error("CAN'T FETCH FILE MENU");
                }

              XtManageChild(widget_array[k_file_menu]);
             }
	     else
	     {
              XtManageChild(widget_array[k_file_menu]);
	     }
	  break;

	  default:
	  break;
	  }
	}
/********************************************************************/
/* END SWITCH                                                       */
/********************************************************************/
}

/************************************************************/
/* CODE FROM "mbedit_stubs.c"                               */
/************************************************************/

/********************************************************************/
/* File selector routine called by scandir().                       */
/* Return TRUE if filename is not "." or "..".                      */
/********************************************************************/

 int open_files_select(entry)
	struct direct	*entry;
{
	char		*ptr;
	if ((strcmp(entry->d_name, ".") == 0) ||
	    (strcmp(entry->d_name, "..") == 0))
		return (FALSE);
	else
		return(TRUE);
}

/********************************************************************/
/* Notify callback function for `slider_number_pings'.              */
/********************************************************************/

static void set_number_pings(w, tag, scale)
	Widget w;
	int *tag;
	XmScaleCallbackStruct *scale;
{

	/* Read the value of the slider bar for number of pings displayed */
	mplot_size = scale->value;

	status = mbedit_action_plot(mxscale,myscale,
		mx_interval,my_interval,mplot_size,&nbuffer,
		&ngood,&icurrent,&mnplot);
	if (status == 0) XBell(theDisplay,100);
}

/********************************************************************/
/* Notify callback function for `button_done'.                      */
/********************************************************************/
 
static void do_done(w, tag, list)
	Widget w;
	int *tag;
	XmListCallbackStruct *list;
{
	status = mbedit_action_done(buffer_size,&ndumped,&nloaded,
			&nbuffer,&ngood,&icurrent);
	if (status == 0) XBell(theDisplay,100);
}

/********************************************************************/
/* Notify callback function for `slider_scale_x'.                   */
/********************************************************************/
 
static void set_scale_x(w, tag, scale)
	Widget w;
	int *tag;
	XmScaleCallbackStruct *scale;
{
	mxscale = scale->value;
	myscale = mxscale/(0.01*exager);

	status = mbedit_action_plot(mxscale,myscale,
		mx_interval,my_interval,mplot_size,&nbuffer,
		&ngood,&icurrent,&mnplot);
	if (status == 0) XBell(theDisplay,100);
	
}

/********************************************************************/
/* Notify callback function for `button_quit'.                      */
/********************************************************************/
 
static void do_quit(w, tag, list)
	Widget w;
	int *tag;
	XmListCallbackStruct *list;
{

	status = mbedit_action_quit(buffer_size,&ndumped,&nloaded,
			&nbuffer,&ngood,&icurrent);
	if (status == 0) XBell(theDisplay,100);
	
		exit(0);

}

/********************************************************************/
/* Notify callback function for `slider_scale_y'.                   */
/********************************************************************/
 
static void set_scale_y(w, tag, scale)
	Widget w;
	int *tag;
	XmScaleCallbackStruct *scale;
{
	exager = scale->value;
	myscale = mxscale/(0.01*exager);
	
	status = mbedit_action_plot(mxscale,myscale,
		mx_interval,my_interval,mplot_size,&nbuffer,
		&ngood,&icurrent,&mnplot);
	if (status == 0) XBell(theDisplay,100);
	

}

/********************************************************************/
/* Notify callback function for `slider_number_step'.               */
/********************************************************************/
 
static void set_number_step(w, tag, scale)
	Widget w;
	int *tag;
	XmScaleCallbackStruct *scale;
{

	step = scale->value;

}

 
/********************************************************************/
/* IF THE CANCEL BUTTON IN THE FILE SELECTION WIDGET IS SELECTED    */
/* THEN REMOVE THE SCREEN AND NOTHING ELSE.                         */
/********************************************************************/
 
static void cancel_file_sel(fs, client_data, cbs)
	Widget fs;
	XtPointer client_data;
	XmFileSelectionBoxCallbackStruct *cbs;
{
	XtUnmanageChild(widget_array[k_file_bb_box]);
}

/********************************************************************/
/* do_load_ok                                                       */
/*                                                                  */
/* Called by:                                                       */
/*           k_file_sel_box - "OK" button                           */
/*                                                                  */
/* Functions called:                                                */
/*                  None                                            */
/*                                                                  */
/* This function reads the selected input file and loads it into    */
/* the variable input_file. It then adds an 'e' to the end of the   */
/* name and loads it into the variable output_file.                 */
/*                                                                  */
/********************************************************************/
 
static void do_load_ok(fs, client_data, cbs)
	Widget fs;
	XtPointer client_data;
	XmFileSelectionBoxCallbackStruct *cbs;
{
	/* local definitions */
	char	*suffix;
	int	len;
	static  char *format_text;

	len = 0;

/*****************************************************************/
/* Read the selected input file name, add an "e" to the name and */
/* write it into the output file name. You can then edit the     */
/* output file name if you wish then hit the "OK" at the bottom  */
/* and the program will use the input_file for input and write   */
/* new edited data into output_file.                             */
/*****************************************************************/

	if(!XmStringGetLtoR(cbs->value,charset, &input_file))
	{
		fprintf(stderr,"\n%s input file name\n",input_file);
		selected = 0;
	}
	else
	{
		if ((suffix = strstr(input_file,".mb")) != NULL)
			len = strlen(suffix);
		if (len >= 4 && len <= 5)
			{
			strncpy(output_file,"\0",128);
			strncpy(output_file,input_file,strlen(input_file)-len);
			strcat(output_file,"e");
			strcat(output_file,suffix);
			}
		else
			{
			strcpy(output_file,input_file);
			strcat(output_file,".ed");
			}
		selected = 1;
	}

	/* read the mbio format number from the screen */
	format_text = XmTextGetString(widget_array[k_mbio_format]);
	sscanf(format_text, "%d", &mformat);


	if (selected > 0)
		{
		/* remove the file selection menu screen */
		XtUnmanageChild(widget_array[k_file_bb_box]);

		/* process the output file name */
		status = mbedit_set_output_file(output_file);

		/* process input file name */
		status = mbedit_action_open(input_file,
				mformat,hold_size,buffer_size,
				mxscale,myscale,mx_interval,
				my_interval,mplot_size,
				&ndumped,&nloaded,&nbuffer,
				&ngood,&icurrent,&mnplot);
		if (status == 0) XBell(theDisplay,100);

		/* display data from chosen file */
		status = mbedit_action_plot(mxscale,myscale,
		         mx_interval,my_interval,mplot_size,&nbuffer,
		         &ngood,&icurrent,&mnplot);
		if (status == 0) XBell(theDisplay,100);

		/* set widget values */
		setup_data();

		}
	else
		{
		fprintf(stderr,"\nno input multibeam file selected\n");
		}
}

/********************************************************************/
/* grab_file                                                        */
/*                                                                  */
/* Called by:                                                       */
/*           k_grab_file_button - 'OK' at bottom of the file        */
/*                                selection menu.                   */
/*                                                                  */
/* Functions called:                                                */
/*                  mbedit_set_output_file                          */
/*                  mbedit_action_open                              */
/*                                                                  */
/* Called by the "OK" at the bottom of the file selection widget.   */
/* This function reads the input file name, the output file name    */
/* and the mbio format text. It then loads the input file selected  */
/* and removes itself.                                              */
/*                                                                  */
/********************************************************************/
 
static void grab_file(w, tag, list)
	Widget w;
	int *tag;
	XmListCallbackStruct *list;
{
	/* local definitions */
	static  char *format_text;

	/* read the mbio format number from the screen */
	format_text = XmTextGetString(widget_array[k_mbio_format]);
	sscanf(format_text, "%d", &mformat);


	if (selected > 0)
		{
		/* remove the file selection menu screen */
		XtUnmanageChild(widget_array[k_file_bb_box]);

		/* process the output file name */
		status = mbedit_set_output_file(output_file);

		/* process input file name */
		status = mbedit_action_open(input_file,
				mformat,hold_size,buffer_size,
				mxscale,myscale,mx_interval,
				my_interval,mplot_size,
				&ndumped,&nloaded,&nbuffer,
				&ngood,&icurrent,&mnplot);
		if (status == 0) XBell(theDisplay,100);

		/* display data from chosen file */
		status = mbedit_action_plot(mxscale,myscale,
		         mx_interval,my_interval,mplot_size,&nbuffer,
		         &ngood,&icurrent,&mnplot);
		if (status == 0) XBell(theDisplay,100);

		/* set widget values */
		setup_data();

		}
	else
		{
		fprintf(stderr,"\nno input multibeam file selected\n");
		}

}


/********************************************************************/
/* Notify callback function for `button_next_buffer'.               */
/********************************************************************/
 
static void do_next_buffer(w, tag, list)
	Widget w;
	int *tag;
	XmListCallbackStruct *list;
{
	/* get next buffer */
	status = mbedit_action_next_buffer(hold_size,buffer_size,
			mxscale,myscale,mx_interval,my_interval,mplot_size,
			&ndumped,&nloaded,&nbuffer,
			&ngood,&icurrent,&mnplot);
	if (status == 0) XBell(theDisplay,100);

	/* set widget values */
	setup_data();

}

/********************************************************************/
/* Notify callback function for `button_forward'.                   */
/********************************************************************/
 
static void do_forward(w, tag, list)
	Widget w;
	int *tag;
	XmListCallbackStruct *list;
{

	status = mbedit_action_step(step,mxscale,myscale,mx_interval,
				    my_interval,mplot_size,&nbuffer,
				    &ngood,&icurrent,&mnplot);
	if (status == 0) XBell(theDisplay,100);

}

/********************************************************************/
/* SCROLL THROUGH PINGS IN A REVERSE ORDER                          */
/********************************************************************/
 
static void do_reverse(w, tag, list)
	Widget w;
	int *tag;
	XmListCallbackStruct *list;
{
	status = mbedit_action_step(-step,mxscale,myscale,
		mx_interval,my_interval,mplot_size,&nbuffer,
		&ngood,&icurrent,&mnplot);
	if (status == 0) XBell(theDisplay,100);

}

/**********************************************************************/
/* READ THE VALUE OF THE NUMBER OF BUFFER SIZE SLIDER BAR.            */
/**********************************************************************/
 
static void do_buffer_size(w, tag, scale)
	Widget w;
	int *tag;
	XmScaleCallbackStruct *scale;
{
	buffer_size = scale->value;	

}

/**********************************************************************/
/* READ THE VALUE OF THE NUMBER OF BUFFER HOLD SIZE SLIDER BAR.       */
/**********************************************************************/
 
static void do_buffer_hold(w, tag, scale)
	Widget w;
	int *tag;
	XmScaleCallbackStruct *scale;
{
	hold_size = scale->value;	
	
}

/********************************************************************/
/* Notify callback function for `textfield_x_interval'.             */
/********************************************************************/
 
static void do_x_interval(w, tag, scale)
	Widget w;
	int *tag;
	XmScaleCallbackStruct *scale;
{
	mx_interval = scale->value;

	if(mx_interval > 0 && mx_interval < 10000)
	{
	status = mbedit_action_plot(mxscale,myscale,
		mx_interval,my_interval,mplot_size,&nbuffer,
		&ngood,&icurrent,&mnplot);
	if (status == 0) XBell(theDisplay,100);
	}	
}

/********************************************************************/
/* Notify callback function for `textfield_y_interval'.             */
/********************************************************************/
 
static void do_y_interval(w, tag, scale)
	Widget w;
	int *tag;
	XmScaleCallbackStruct *scale;
{
	my_interval = scale->value;

	if(my_interval > 0 && my_interval < 1000)
	{
	status = mbedit_action_plot(mxscale,myscale,
		mx_interval,my_interval,mplot_size,&nbuffer,
		&ngood,&icurrent,&mnplot);
	if (status == 0) XBell(theDisplay,100);
	}	

}

/********************************************************************/
/* Event callback function for `canvas_mbedit'.                     */
/********************************************************************/
 
static void do_event(w, data, cbs)
	Widget w;
	XtPointer data;
	XmDrawingAreaCallbackStruct *cbs;
{
	static Position x_loc, y_loc;

	XEvent  *event = cbs->event;

	KeySym keysym;
	int key_num;
	char buffer[1];
	int actual;
	static char *pings_to_first_step_text;
	int *x, *y;
	int root_x_return, root_y_return,win_x,win_y;
	unsigned int mask_return;

	/* check for data file loaded at startup */
	if (startup_file)
		{
		startup_file = 0;
		status = mbedit_action_plot(mxscale,myscale,
			mx_interval,my_interval,mplot_size,&nbuffer,
			&ngood,&icurrent,&mnplot);
		if (status == 0) XBell(theDisplay,100);
		} /* end startup file */

	/* If there is input in the drawing area */
	if (cbs->reason == XmCR_INPUT)
	{
	  if(event->xany.type == KeyPress)
	  {
	  /* Get key pressed - buffer[0] */
	  actual = XLookupString((XKeyEvent *)event, buffer, 1, &keysym, NULL);

	  /* process events */
	  switch (buffer[0])
		{
		case 'M':
		case 'm':
		case 'Z':
		case 'z':
			status = mbedit_action_mouse_pick(
				x_loc, y_loc,
				mxscale,myscale,
				mx_interval,my_interval,mplot_size,
				&nbuffer,&ngood,&icurrent,&mnplot);
			status = mbedit_action_bad_ping(
				mxscale,myscale,
				mx_interval,my_interval,mplot_size,
				&nbuffer,&ngood,&icurrent,&mnplot);
			break;
		case 'K':
		case 'k':
		case 'S':
		case 's':
			status = mbedit_action_good_ping(
				mxscale,myscale,
				mx_interval,my_interval,mplot_size,
				&nbuffer,&ngood,&icurrent,&mnplot);
			break;
		case 'J':
		case 'j':
		case 'A':
		case 'a':
			status = mbedit_action_left_ping(
				mxscale,myscale,
				mx_interval,my_interval,mplot_size,
				&nbuffer,&ngood,&icurrent,&mnplot);
			break;
		case 'L':
		case 'l':
		case 'D':
		case 'd':
			status = mbedit_action_right_ping(
				mxscale,myscale,
				mx_interval,my_interval,mplot_size,
				&nbuffer,&ngood,&icurrent,&mnplot);
			break;
		case 'U':
		case 'u':
		case 'Q':
		case 'q':
				{
				mode_pick = MODE_PICK;
				XmToggleButtonSetState(widget_array[k_pick_button], 1, FALSE);
				XmToggleButtonSetState(widget_array[k_erase_button], 0, FALSE);
				XmToggleButtonSetState(widget_array[k_restore_button], 0, FALSE);

				myCursor = XCreateFontCursor(theDisplay, XC_target);
				XAllocNamedColor(theDisplay,colormap,"red",&closest[0],&exact[0]);
				XAllocNamedColor(theDisplay,colormap,"coral",&closest[1],&exact[1]);
				XRecolorCursor(theDisplay,myCursor,&closest[0],&closest[1]);
				XDefineCursor(theDisplay,can_xid,myCursor);
				}
			break;
		case 'I':
		case 'i':
		case 'W':
		case 'w':
				{
				mode_pick = MODE_ERASE;
				XmToggleButtonSetState(widget_array[k_pick_button], 0, FALSE);
				XmToggleButtonSetState(widget_array[k_erase_button], 1, FALSE);
				XmToggleButtonSetState(widget_array[k_restore_button], 0, FALSE);

				myCursor = XCreateFontCursor(theDisplay, XC_exchange);
				XAllocNamedColor(theDisplay,colormap,"red",&closest[0],&exact[0]);
				XAllocNamedColor(theDisplay,colormap,"coral",&closest[1],&exact[1]);
				XRecolorCursor(theDisplay,myCursor,&closest[0],&closest[1]);
				XDefineCursor(theDisplay,can_xid,myCursor);
				}
			break;
		case 'O':
		case 'o':
		case 'E':
		case 'e':
				{
				mode_pick = MODE_RESTORE;
				XmToggleButtonSetState(widget_array[k_pick_button], 0, FALSE);
				XmToggleButtonSetState(widget_array[k_erase_button], 0, FALSE);
				XmToggleButtonSetState(widget_array[k_restore_button], 1, FALSE);

				myCursor = XCreateFontCursor(theDisplay, XC_exchange);
				XAllocNamedColor(theDisplay,colormap,"green",&closest[0],&exact[0]);
				XAllocNamedColor(theDisplay,colormap,"coral",&closest[1],&exact[1]);
				XRecolorCursor(theDisplay,myCursor,&closest[0],&closest[1]);
				XDefineCursor(theDisplay,can_xid,myCursor);
				}
			break;
		default:
			break;
	      } /* end of key switch */

	   } /* end of key press events */


	  /* Check for mouse pressed and not pressed and released. */
	  if(event->xany.type == ButtonPress)
	  {
	      /* If left mouse button is pushed then pick, erase or restore. */
	      if(event->xbutton.button == 1)
	      {
		x_loc = event->xbutton.x;
		y_loc = event->xbutton.y;
	
	again:

	            if(mode_pick == MODE_PICK)
			status = mbedit_action_mouse_pick(
				x_loc, y_loc,
				mxscale,myscale,
				mx_interval,my_interval,mplot_size,
				&nbuffer,&ngood,&icurrent,&mnplot);
		    else if (mode_pick == MODE_ERASE) 
			status = mbedit_action_mouse_erase(
				x_loc, y_loc,
				mxscale,myscale,
				mx_interval,my_interval,mplot_size,
				&nbuffer,&ngood,&icurrent,&mnplot);
		    else if (mode_pick == MODE_RESTORE) 
			status = mbedit_action_mouse_restore(
				x_loc, y_loc,
				mxscale,myscale,
				mx_interval,my_interval,mplot_size,
				&nbuffer,&ngood,&icurrent,&mnplot);
		if (status == 0) XBell(theDisplay,100);

			status = XQueryPointer(theDisplay,can_xid,
				&root_return,&child_return,&root_x_return,
		      		&root_y_return, &win_x, &win_y, &mask_return);

			x_loc = win_x;
			y_loc = win_y;

			/* If the button is still pressed then read the location */
			/* of the pointer and run the action mouse function again */
			if(mask_return == 256 && mode_pick != MODE_PICK)
			   goto again;


		} /* end of left button events */

		/* If middle mouse button is pushed then scroll in reverse. */
		if(event->xbutton.button == 2)
		{
			status = mbedit_action_step(-step,mxscale,myscale,
					mx_interval,my_interval,
					mplot_size,&nbuffer,
					&ngood,&icurrent,&mnplot);
			if (status == 0) XBell(theDisplay,100);
		} /* end of middle button events */

		/* If right mouse button is pushed then scroll forward. */
		if(event->xbutton.button == 3)
		{
			status = mbedit_action_step(step,mxscale,myscale,
					mx_interval,my_interval,
					mplot_size,&nbuffer,
					&ngood,&icurrent,&mnplot);
			if (status == 0) XBell(theDisplay,100);
		} /* end of right button events */	
	  } /* end of button pressed events */
	} /* end of inputs from window */
} /* end do_event function */

/********************************************************************/
/* User-defined action for `setting_mode'.                          */
/********************************************************************/
 
static void set_mode_pick(w, tag, list)
	Widget w;
	int *tag;
	XmListCallbackStruct *list;
{
	mode_pick = MODE_PICK;

	myCursor = XCreateFontCursor(theDisplay, XC_target);
	XAllocNamedColor(theDisplay,colormap,"red",&closest[0],&exact[0]);
	XAllocNamedColor(theDisplay,colormap,"coral",&closest[1],&exact[1]);
	XRecolorCursor(theDisplay,myCursor,&closest[0],&closest[1]);
	XDefineCursor(theDisplay,can_xid,myCursor);
}

/********************************************************************/
/* User-defined action for `setting_mode'.                          */
/********************************************************************/
 
static void set_mode_erase(w, tag, list)
	Widget w;
	int *tag;
	XmListCallbackStruct *list;
{
	mode_pick = MODE_ERASE;

	myCursor = XCreateFontCursor(theDisplay, XC_exchange);
	XAllocNamedColor(theDisplay,colormap,"red",&closest[0],&exact[0]);
	XAllocNamedColor(theDisplay,colormap,"coral",&closest[1],&exact[1]);
	XRecolorCursor(theDisplay,myCursor,&closest[0],&closest[1]);
	XDefineCursor(theDisplay,can_xid,myCursor);
}

/********************************************************************/
/* User-defined action for `setting_mode'.                          */
/********************************************************************/
 
static void set_mode_restore(w, tag, list)
	Widget w;
	int *tag;
	XmListCallbackStruct *list;
{
	mode_pick = MODE_RESTORE;

	myCursor = XCreateFontCursor(theDisplay, XC_exchange);
	XAllocNamedColor(theDisplay,colormap,"green",&closest[0],&exact[0]);
	XAllocNamedColor(theDisplay,colormap,"coral",&closest[1],&exact[1]);
	XRecolorCursor(theDisplay,myCursor,&closest[0],&closest[1]);
	XDefineCursor(theDisplay,can_xid,myCursor);
}

/********************************************************************/
/* Notify callback function for `apply_goto_button'.                */
/********************************************************************/
 
static void apply_goto_button(w, tag, list)
	Widget w;
	int *tag;
	XmListCallbackStruct *list;
{

	static char *time0_text;
	static char *time1_text;
	static char *time2_text;
	static char *time3_text;
	static char *time4_text;
	static char *time5_text;

	time0_text = XmTextGetString(widget_array[k_glf_year]);
	sscanf(time0_text, "%d", &ttime_i[0]);

	time1_text = XmTextGetString(widget_array[k_glf_month]);
	sscanf(time1_text, "%d", &ttime_i[1]);

	time2_text = XmTextGetString(widget_array[k_glf_day]);
	sscanf(time2_text, "%d", &ttime_i[2]);

	time3_text = XmTextGetString(widget_array[k_glf_hour]);
	sscanf(time3_text, "%d", &ttime_i[3]);

	time4_text = XmTextGetString(widget_array[k_glf_min]);
	sscanf(time4_text, "%d", &ttime_i[4]);

	time5_text = XmTextGetString(widget_array[k_glf_sec]);
	sscanf(time5_text, "%d", &ttime_i[5]);
	ttime_i[6] = 0;

	fprintf(stderr,"goto time: %d %d %d %d %d %d %d\n",
		ttime_i[0],ttime_i[1],ttime_i[2],ttime_i[3],
		ttime_i[4],ttime_i[5],ttime_i[6]);
	status = mbedit_action_goto(ttime_i,hold_size,buffer_size,
			mxscale,myscale,mx_interval,my_interval,mplot_size,
			&ndumped,&nloaded,&nbuffer,
			&ngood,&icurrent,&mnplot);
	if (status == 0) XBell(theDisplay,100);
	
	XtUnmanageChild(widget_array[k_goto_menu]);

}

/********************************************************************/
/* Notify callback function for `cancel_goto_button'.                */
/********************************************************************/
 
static void cancel_goto_button(w, tag, list)
	Widget w;
	int *tag;
	XmListCallbackStruct *list;
{
		XtUnmanageChild(widget_array[k_goto_menu]);
}

/********************************************************************/
/* END OF "mbedit_stubs.c".                                         */
/********************************************************************/

