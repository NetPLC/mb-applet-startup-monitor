/*
 * mb-applet-startup-monitor - A tray app that provides feedback
 *                             on application startup.
 *
 * Copyright 2004, Openedhand Ltd. By Matthew Allum <mallum@o-hand.com>
 *
 * Based very roughly on GPE's startup monitor. 
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define SN_API_NOT_YET_FROZEN 
#include <libsn/sn.h>

#include <libmb/mb.h>

#define TIMEOUT		   20
#define POLLTIME	   10
#define HOURGLASS_N_FRAMES 8 

typedef struct LaunchList LaunchList;

struct LaunchList
{
  char       *id;
  time_t      when;
  LaunchList *next;
};

/* 
 * Lazy boy globals :/
 */
static MBTrayApp     *app = NULL;
static LaunchList    *launch_list = NULL;
static MBPixbuf      *pb;
static MBPixbufImage *img_hourglass[HOURGLASS_N_FRAMES] = { 0,0,0,0,0,0,0,0 };static  MBPixbufImage *img_hourglass_scaled[HOURGLASS_N_FRAMES] = { 0,0,0,0,0,0,0,0 };
static Bool           hourglass_shown;
static int            hourglass_cur_frame_n = 0;
static char          *ThemeName = NULL;

static SnDisplay        *display;
static SnMonitorContext *context;

static void
show_hourglass (void)
{
  mb_tray_app_request_offset (app, -1);
  mb_tray_app_set_session (app, False); 
  mb_tray_app_unhide (app);
  hourglass_shown = TRUE;
}

static void
hide_hourglass (void)
{
  mb_tray_app_hide (app);
  XFlush(mb_tray_app_xdisplay(app));
  XSelectInput (mb_tray_app_xdisplay(app), mb_tray_app_xrootwin(app), 
		PropertyChangeMask | StructureNotifyMask);
  hourglass_shown = FALSE;
}

static void
monitor_event_func (SnMonitorEvent *event,
                    void            *user_data)
{
  SnMonitorContext *context;
  SnStartupSequence *sequence;
  const char *id;
  time_t t;
  
  context = sn_monitor_event_get_context (event);
  sequence = sn_monitor_event_get_startup_sequence (event);
  id = sn_startup_sequence_get_id (sequence);

  switch (sn_monitor_event_get_type (event))
    {
    case SN_MONITOR_EVENT_INITIATED:
      {
	
	LaunchList *item = launch_list;

	if (item == NULL)
	  {
	    launch_list = item = malloc(sizeof(LaunchList));
	  }
	else
	  {
	    while (item->next != NULL)
	      item = item->next;
	    item->next = malloc(sizeof(LaunchList));
	    item = item->next;
	  }

	item->next = NULL;
	item->id   = strdup (id);
	t = time (NULL);
	item->when = t + TIMEOUT;

	if (! hourglass_shown)
	  show_hourglass ();
      }
      break;

    case SN_MONITOR_EVENT_COMPLETED:
    case SN_MONITOR_EVENT_CANCELED:
      {
	LaunchList *item = launch_list, *last_item = NULL;

	while (item != NULL)
	  {
	    if (!strcmp(item->id, id))
	      {
		if (last_item == NULL)
		  launch_list = item->next;
		else
		  last_item->next = item->next;

		free(item->id);
		free(item);

		break;
	      }
	    last_item = item;
	    item = item->next;
	  }

	if (launch_list == NULL && hourglass_shown)
	  hide_hourglass ();
      }
      break;
    default:
      break; 			/* Nothing */
    }
}

void
paint_callback ( MBTrayApp *app, Drawable drw )
{
  MBPixbufImage *img_backing = NULL;

  if (!hourglass_shown) return;
  
  img_backing = mb_tray_app_get_background (app, pb);

  /* CurrentVolLevel */
  mb_pixbuf_img_composite(pb, img_backing, 
			  img_hourglass_scaled[hourglass_cur_frame_n], 
			  0, 0);

  mb_pixbuf_img_render_to_drawable(pb, img_backing, drw, 0, 0);

  mb_pixbuf_img_free( pb, img_backing );
}

void
resize_callback ( MBTrayApp *app, int w, int h )
{
  int i;

  if (!hourglass_shown) return;

  for (i=0; i<HOURGLASS_N_FRAMES; i++)
    {
      if (img_hourglass_scaled[i] != NULL) 
	mb_pixbuf_img_free(pb, img_hourglass_scaled[i]);
      img_hourglass_scaled[i] = mb_pixbuf_img_scale(pb, 
						    img_hourglass[i], w, h);
    }
}

void
button_callback (MBTrayApp *app, int x, int y, Bool is_released )
{
  ; 				/* Nothing yet */
}

void
load_icons(void)
{
  int   i;
  char *icon_path;
  char  icon_name[128] = { 0 };
  
  for (i=0; i<HOURGLASS_N_FRAMES; i++)
    {
      if (img_hourglass[i] != NULL) 
	mb_pixbuf_img_free(pb, img_hourglass[i]);

      sprintf(icon_name, "hourglass-%i.png", i+1);

      icon_path = mb_dot_desktop_icon_get_full_path (ThemeName, 
						     32, 
						     icon_name);
      
      if (icon_path == NULL 
	  || !(img_hourglass[i] = mb_pixbuf_img_new_from_file(pb, icon_path)))
	{
	  fprintf(stderr, 
		  "mb-applet-startup-monitor: failed to load icon '%s'\n", 
		  icon_name );
	  exit(1);
	}
      free(icon_path);
    }
}

void 
theme_callback (MBTrayApp *app, char *theme_name)
{
  /* TODO
  if (!theme_name) return;
  if (ThemeName) free(ThemeName);
  ThemeName = strdup(theme_name);
  load_icons(); 	
  resize_callback (app, mb_tray_app_width(app), mb_tray_app_width(app) );
  */
}

void
timeout_callback ( MBTrayApp *app )
{
  LaunchList *item = launch_list;
  LaunchList *last_item = NULL;
  time_t t;

  if (!hourglass_shown) return;

  t = time (NULL);

  /* timeouts */
  while (item)
    {
      if ((item->when - t) <= 0)
	{
	  if (last_item == NULL)
	    launch_list = item->next;
	  else
	    last_item->next = item->next;
	  
	  free(item->id);
	  free(item);
	  
	  break;
	}
      last_item = item;
      item = item->next;
    }
  
  if (launch_list == NULL && hourglass_shown)
    {
      hide_hourglass ();
      return;
    }

  hourglass_cur_frame_n++;
  if (hourglass_cur_frame_n == 8)
    hourglass_cur_frame_n = 0;

  mb_tray_app_repaint (app);
}

void 
xevent_callback( MBTrayApp *mb_tray_app, XEvent *event ) 
{
  sn_display_process_event (display, event);
}

int
main (int argc, char **argv)
{
  struct timeval tv;

  app = mb_tray_app_new ( "Startup Monitor",
			  resize_callback,
			  paint_callback,
			  &argc,
			  &argv );  

  /* if (app == NULL) usage(); */

  
  pb = mb_pixbuf_new(mb_tray_app_xdisplay(app), 
		     mb_tray_app_xscreen(app));

  load_icons();

  XSelectInput (mb_tray_app_xdisplay(app), mb_tray_app_xrootwin(app), 
		PropertyChangeMask | StructureNotifyMask);

  mb_tray_app_set_xevent_callback (app, xevent_callback); 

  display = sn_display_new (mb_tray_app_xdisplay(app), NULL, NULL);

  context = sn_monitor_context_new (display, 
				    mb_tray_app_xscreen(app),
                                    monitor_event_func,
                                    NULL, NULL);  
  
  mb_tray_app_set_button_callback (app, button_callback );

  memset(&tv,0,sizeof(struct timeval));
  tv.tv_sec = 0;
  tv.tv_usec = 100000;

  mb_tray_app_set_timeout_callback (app, timeout_callback, &tv); 

  /* Add timeout call for failed launch apps */

  mb_tray_app_request_offset (app, -1);

  /* Dont save us in panel session */
  mb_tray_app_set_session (app, False); 

  mb_tray_app_hide (app);

  mb_tray_app_main (app);

  return 0;
}
