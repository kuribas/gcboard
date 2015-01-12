/*
  gtkcboard.h - Generic Chess Board
  code for gtk+ cboard-widget
  http://gcboard.sourceforge.net
  Copyright (C) 2004 Kristof Bastiaensen
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#define GTK_DISABLE_DEPRECATED 1

#include <gdk/gdk.h>
#include <gtk/gtk.h>

/******************************************
 *     typedefs used by gcb_common.h      *
 ******************************************/

typedef struct
{
   int type;
   union {
      GdkPixbuf *pixbuf;
      struct {
	 GdkPixmap *img;
	 GdkBitmap *mask;
      } pixmap;
   };
} GcbPixmap;


typedef struct
{
   GdkPixmap *img;
   GdkGC     *gc;
} GcbDrawable;

typedef struct
{
   guint id;
   void (* callback)(gpointer);
   int running;
} GcbTimer;


#include "gcb_common.h"
#include "gtkcboard.h"
#include "gtkcboard-marshal.h"


/******************************************
 *   inline functions for gcb_common.c    *
 ******************************************/

static
gint gtk_cboard_timer_callback(gpointer data)
{
   GcbBoard *board = (GcbBoard *)data;
   if(board->timer.running)
      (board->timer.callback)((gpointer)board);
      
   return board->timer.running;
}

static inline
void set_timer(void *data, GcbTimer *timer, short interval,
	       void (* func) (GcbBoard *), GcbBoard *board)
{
   timer->id = g_timeout_add(interval, gtk_cboard_timer_callback, (gpointer) board);
   timer->callback = (gpointer)func;
   timer->running = TRUE;
}

static inline
void stop_timer(void *data, GcbTimer *timer)
{
   timer->running = FALSE;
}

static inline
void copy_drawable(void *data, GcbDrawable *src, GcbDrawable *dest,
		   short srcX, short srcY, short destX, short destY,
		   short width, short height)
{
   gdk_draw_drawable(dest->img, dest->gc, src->img, srcX, srcY, destX, destY, width, height);
}
     
static inline
void copy_pixmap(void *data, GcbPixmap *pmap, GcbDrawable *dest,
		 short srcX, short srcY, short destX, short destY,
		 short width, short height)
{
   switch(pmap->type)
   {
     case GTKCBOARD_PIXMAP:
	gdk_gc_set_clip_origin(dest->gc, destX - srcX, destY - srcY);
	gdk_gc_set_clip_mask(dest->gc, pmap->pixmap.mask);
	gdk_draw_drawable(dest->img, dest->gc, pmap->pixmap.img, srcX, srcY,
			  destX, destY, width, height);
	gdk_gc_set_clip_mask(dest->gc, NULL);
	gdk_gc_set_clip_origin(dest->gc, 0, 0);
	break;
     case GTKCBOARD_PIXBUF:
	gdk_draw_pixbuf(dest->img, dest->gc, pmap->pixbuf, srcX, srcY,
			destX, destY, width, height,
			GDK_RGB_DITHER_NORMAL, 0, 0);
   }
}

static inline
void pixmap_ref(GcbPixmap *pixmap)
{
   switch(pixmap->type)
   {
     case GTKCBOARD_PIXBUF:
	g_object_ref(pixmap->pixbuf);
	break;
     case GTKCBOARD_PIXMAP:
	g_object_ref(pixmap->pixmap.img);
	if(pixmap->pixmap.mask)
	   g_object_ref(pixmap->pixmap.mask);
   }
}

static inline
void pixmap_unref(GcbPixmap *pixmap)
{
   switch(pixmap->type)
   {
     case GTKCBOARD_PIXBUF:
	g_object_unref(pixmap->pixbuf);
	break;
     case GTKCBOARD_PIXMAP:
	g_object_unref(pixmap->pixmap.img);
	if(pixmap->pixmap.mask)
	   g_object_unref(pixmap->pixmap.mask);
   }
}

static inline
void create_drawable(void *data, GcbDrawable *draw, short width, short height)
{
   GtkCBoard *cboard = (GtkCBoard *)data;

   draw->img = gdk_pixmap_new(GTK_WIDGET(cboard)->window, width, height, -1);
   draw->gc  = gdk_gc_new(draw->img);
}

static inline
void destroy_drawable(void *data, GcbDrawable *draw)
{
   g_object_unref( draw->img );
   gdk_gc_destroy( draw->gc );
}

#define RUNTIME_ERROR(...) { g_critical(__VA_ARGS__); return; }

/****************************************
 *        declare callbacks             *
 ****************************************/

static inline void gcb_highlight_square_cb(void *, short, short, short, short);
static inline void gcb_press_square_cb(void *, short, short, short, short);
static inline bool gcb_drop_piece_cb(void *, short, short, short, short, short, short);
static inline void gcb_refresh_area_cb(void *, short, short, short, short);
static inline void gcb_draw_background_cb(void *, short, short, short, short);

#define GCB_SCOPE static
#include "gcb_common.c"


/****************************************
 *      setup gtk data                  *
 ****************************************/


#define GET_BOARD(brd) ((GcbBoard *)(brd->board))
#define GCB_PIECE(piece) ((GcbPieceType *)(piece))

static void gtk_cboard_class_init     (GtkCBoardClass  *klass);
static void gtk_cboard_init           (GtkCBoard *cboard);
static void gtk_cboard_realize        (GtkWidget *widget);
static void gtk_cboard_destroy        (GtkObject *object);
static gint gtk_cboard_expose         (GtkWidget *widget,
				       GdkEventExpose *event);
static gint gtk_cboard_motion_notify  (GtkWidget *widget,
				       GdkEventMotion *event);
static void gtk_cboard_size_allocate  (GtkWidget *widget,
				       GtkAllocation *allocation);
static void gtk_cboard_size_request   (GtkWidget *widget,
				       GtkRequisition *requisition);
static gint gtk_cboard_button_press   (GtkWidget *widget,
				       GdkEventButton *event);
static gint gtk_cboard_button_release (GtkWidget *widget,
				       GdkEventButton *event);
static gint gtk_cboard_motion_notify  (GtkWidget *widget,
				       GdkEventMotion *event);
static void gtk_cboard_map            (GtkWidget *widget);
static void gtk_cboard_unmap          (GtkWidget *widget);

static GtkWidgetClass *parent_class = NULL;

GType
gtk_cboard_get_type (void)
{
   static GType cboard_type = 0;

   if (!cboard_type)
   {
       static const GTypeInfo cboard_info =
       {
	  sizeof (GtkCBoardClass),
	  NULL,
	  NULL,
	  (GClassInitFunc) gtk_cboard_class_init,
	  NULL,
	  NULL,
	  sizeof(GtkCBoard),
	  0,
	  (GInstanceInitFunc) gtk_cboard_init,
       };

       cboard_type = g_type_register_static (GTK_TYPE_WIDGET,
					     "GtkCBoard",
					     &cboard_info,
					     0);
   }

   return cboard_type;
}

enum {
   PRESS_SQUARE,
   HIGHLIGHT_SQUARE,
   DRAW_BACKGROUND,
   DROP_PIECE,
   INIT_BOARD,
   SIGNALS_SIZE
};

static gint gtk_cboard_signals[SIGNALS_SIZE] = {0, 0, 0, 0};

static void
gtk_cboard_class_init (GtkCBoardClass *class)
{
   GtkObjectClass *object_class;
   GtkWidgetClass *widget_class;
   
   object_class = (GtkObjectClass*) class;
   widget_class = (GtkWidgetClass*) class;

   parent_class = g_type_class_peek_parent(class);
   
   object_class->destroy = gtk_cboard_destroy;
   
   widget_class->realize =             gtk_cboard_realize;
   widget_class->expose_event =        gtk_cboard_expose;
   widget_class->size_allocate =       gtk_cboard_size_allocate;
   widget_class->size_request =        gtk_cboard_size_request;
   widget_class->button_press_event =  gtk_cboard_button_press;
   widget_class->button_release_event = gtk_cboard_button_release;
   widget_class->motion_notify_event = gtk_cboard_motion_notify;
   widget_class->map =                 gtk_cboard_map;
   widget_class->unmap =               gtk_cboard_unmap;

   gtk_cboard_signals[PRESS_SQUARE] = g_signal_new ("press_square", 
						    G_OBJECT_CLASS_TYPE(object_class),
						    G_SIGNAL_RUN_FIRST,
						    G_STRUCT_OFFSET (GtkCBoardClass, press_square),
						    NULL, NULL,
						    g_cclosure_user_marshal_NONE__INT_INT_INT_INT,
						    G_TYPE_NONE, 4,
						    G_TYPE_INT, G_TYPE_INT,
						    G_TYPE_INT, G_TYPE_INT);
   
   gtk_cboard_signals[HIGHLIGHT_SQUARE] = g_signal_new ("highlight_square", 
							G_OBJECT_CLASS_TYPE(object_class),
							G_SIGNAL_RUN_FIRST,
							G_STRUCT_OFFSET (GtkCBoardClass, highlight_square),
							NULL, NULL,
							g_cclosure_user_marshal_NONE__INT_INT_INT_INT,
							G_TYPE_NONE, 4,
							G_TYPE_INT, G_TYPE_INT,
							G_TYPE_INT, G_TYPE_INT);

   gtk_cboard_signals[DRAW_BACKGROUND] = g_signal_new ("draw_background",
						       G_OBJECT_CLASS_TYPE(object_class),
						       G_SIGNAL_RUN_FIRST,
						       G_STRUCT_OFFSET (GtkCBoardClass, draw_background),
						       NULL, NULL,
						       g_cclosure_user_marshal_NONE__INT_INT_INT_INT,
						       G_TYPE_NONE, 4,
						       G_TYPE_INT, G_TYPE_INT,
						       G_TYPE_INT, G_TYPE_INT);

   gtk_cboard_signals[DROP_PIECE] = g_signal_new ("drop_piece",
						  G_OBJECT_CLASS_TYPE(object_class),
						  G_SIGNAL_RUN_LAST,
						  G_STRUCT_OFFSET (GtkCBoardClass, drop_piece),
						  NULL, NULL,
						  g_cclosure_user_marshal_BOOLEAN__INT_INT_INT_INT_INT_INT,
						  G_TYPE_BOOLEAN, 6,
						  G_TYPE_INT, G_TYPE_INT,
						  G_TYPE_INT, G_TYPE_INT,
						  G_TYPE_INT, G_TYPE_INT);

   gtk_cboard_signals[INIT_BOARD] = g_signal_new ("init_board", 
						  G_OBJECT_CLASS_TYPE(object_class),
						  G_SIGNAL_RUN_FIRST,
						  G_STRUCT_OFFSET (GtkCBoardClass, init_board),
						  NULL, NULL,
						  g_cclosure_user_marshal_NONE__NONE,
						  G_TYPE_NONE, 0);

   class->press_square = NULL;
   class->highlight_square = NULL;
   class->drop_piece = NULL;
   class->draw_background = NULL;
   class->init_board = NULL;
}

static void
gtk_cboard_init (GtkCBoard *cboard)
{
   cboard->board = NULL;
   cboard->width = cboard->height = 0;
   cboard->canvas.img = NULL;
   cboard->canvas.gc = NULL;

   /* We don't need double buffering, because we already have
      a backing store */
   gtk_widget_set_double_buffered (GTK_WIDGET(cboard), FALSE);
}

GtkWidget *gtk_cboard_new (guint16   width,
			   guint16   height,
			   gint16    grids,
			   const GtkCBoardGridInfo *ginfo)
{
   GtkCBoard *cboard;
   gint i;

   cboard = (GtkCBoard *)g_object_new (GTK_TYPE_CBOARD, NULL);
   if(!cboard)
      return NULL;
   
   cboard->board = (gpointer)gcb_new_board(grids, cboard);
   if(!cboard)
      return NULL;
   
   for (i = 0; i < grids; i++)
      gcb_set_grid(GET_BOARD(cboard), i, ginfo[i].x, ginfo[i].y,
		   ginfo[i].columns, ginfo[i].rows, ginfo[i].square_w,
		   ginfo[i].square_h, ginfo[i].border_x, ginfo[i].border_y);

   cboard->width = width;
   cboard->height = height;

   return GTK_WIDGET(cboard);
}

static void
gtk_cboard_destroy (GtkObject *object)
{
   GtkCBoard *cboard;

   g_return_if_fail (object != NULL);
   g_return_if_fail(GTK_IS_CBOARD (object));
   
   cboard = GTK_CBOARD(object);
   
   if(GET_BOARD(cboard))
   {
       gcb_delete_board(GET_BOARD(cboard));
       cboard->board = NULL;
   
       if(cboard->canvas.gc)
	  gdk_gc_destroy(cboard->canvas.gc);

       g_object_unref(cboard->canvas.img);
   }

   if (GTK_OBJECT_CLASS (parent_class)->destroy)
      (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gtk_cboard_realize (GtkWidget *widget)
{
   GtkCBoard *cboard;
   GdkWindowAttr attributes;
   gint attributes_mask;
  
   g_return_if_fail (widget != NULL);
   g_return_if_fail (GTK_IS_CBOARD (widget));

   cboard = GTK_CBOARD (widget);
   gtk_widget_set_realized(widget, TRUE);

   attributes.window_type = GDK_WINDOW_CHILD;
   attributes.x = widget->allocation.x;
   attributes.y = widget->allocation.y;
   attributes.width = widget->allocation.width;
   attributes.height = widget->allocation.height;
   attributes.wclass = GDK_INPUT_OUTPUT;
   attributes.visual = gtk_widget_get_visual (widget);
   attributes.colormap = gtk_widget_get_colormap (widget);
   attributes.event_mask = gtk_widget_get_events (widget) |
     GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
     GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK;
   
   attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

   widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
   gdk_window_set_user_data (widget->window, cboard);

   widget->style = gtk_style_attach (widget->style, widget->window);
   gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

   g_signal_emit(GTK_OBJECT(cboard), gtk_cboard_signals[INIT_BOARD], 0);
}

static void
gtk_cboard_size_request (GtkWidget      *widget,
			 GtkRequisition *requisition)
{
   GtkCBoard *board;
   
   g_return_if_fail (widget != NULL);
   g_return_if_fail (GTK_IS_CBOARD (widget));

   board = (GtkCBoard *)widget;

   requisition->width = board->width;
   requisition->height = board->height;
}

static void
gtk_cboard_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
   //GtkCBoard *cboard;
   
   g_return_if_fail (widget != NULL);
   g_return_if_fail (GTK_IS_CBOARD (widget));
   g_return_if_fail (allocation != NULL);

   //cboard = (GtkCBoard *)widget;

   widget->allocation = *allocation;
   
   if (gtk_widget_get_realized (widget))
   {
       gdk_window_move_resize (widget->window,
			       allocation->x, allocation->y,
			       allocation->width, allocation->height);
   }
}

static gint
gtk_cboard_expose (GtkWidget *widget,
		   GdkEventExpose *event)
{
   GtkCBoard *cboard;

   g_return_val_if_fail (widget != NULL, FALSE);
   g_return_val_if_fail (GTK_IS_CBOARD (widget), FALSE);
   g_return_val_if_fail (event != NULL, FALSE);

   cboard = GTK_CBOARD (widget);

   gdk_draw_drawable(widget->window,
		     widget->style->fg_gc[widget->state], cboard->canvas.img,
		     event->area.x, event->area.y,
		     event->area.x, event->area.y,
		     event->area.width, event->area.height);
   
   return FALSE;
}

static gint
gtk_cboard_button_press (GtkWidget *widget,
			 GdkEventButton *event)
{
   GtkCBoard *board;

   board = GTK_CBOARD(widget);
   
   g_return_val_if_fail (widget != NULL, FALSE);
   g_return_val_if_fail (GTK_IS_CBOARD (widget), FALSE);
   g_return_val_if_fail (event != NULL, FALSE);
   g_return_val_if_fail (board != NULL, FALSE);

   gtk_grab_add (widget);
   gcb_on_press(GET_BOARD(board), event->x, event->y, event->button);

   return FALSE;
}   


static gint
gtk_cboard_button_release (GtkWidget *widget,
			   GdkEventButton *event)
{
   GtkCBoard *cboard;
   
   g_return_val_if_fail (widget, FALSE);
   g_return_val_if_fail (GTK_IS_CBOARD (widget), FALSE);
   g_return_val_if_fail (event, FALSE);

   cboard = GTK_CBOARD(widget);
   
   gtk_grab_remove(widget);
   gcb_on_release(cboard->board, event->x, event->y, event->button);
   
   return FALSE;
}   

static gint
gtk_cboard_motion_notify (GtkWidget *widget,
			 GdkEventMotion *event)
{
   GtkCBoard *cboard;

   g_return_val_if_fail (widget, FALSE);
   g_return_val_if_fail (GTK_IS_CBOARD (widget), FALSE);
   g_return_val_if_fail (event, FALSE);

   cboard = GTK_CBOARD(widget);
   gcb_on_mouse_move(GET_BOARD(cboard), event->x, event->y);
   
   return FALSE;
}   

static void
gtk_cboard_map (GtkWidget *widget)
{
   GtkCBoard *cboard;
   
   g_return_if_fail (widget);
   g_return_if_fail (GTK_IS_CBOARD (widget));
   
   cboard = (GtkCBoard *)widget;

   if(!gtk_widget_get_mapped (widget))
   {
       gtk_widget_set_mapped(widget, TRUE);
       cboard->canvas.img = gdk_pixmap_new(widget->window, cboard->width,
					   cboard->height, -1);
       cboard->canvas.gc = gdk_gc_new(cboard->canvas.img);
       gcb_set_canvas(GET_BOARD(cboard), *((GcbDrawable *)&cboard->canvas),
		      cboard->width, cboard->height);
       gdk_window_show (widget->window);
       gcb_thaw(cboard->board);       
   }
}

static void
gtk_cboard_unmap (GtkWidget *widget)
{
   GtkCBoard *cboard;
   
   g_return_if_fail (widget);
   g_return_if_fail (GTK_IS_CBOARD (widget));
   
   cboard = (GtkCBoard *)widget;

   if(gtk_widget_get_mapped(widget))
   {
       gtk_widget_set_mapped(widget, FALSE);
       gcb_freeze(GET_BOARD(cboard));
       g_object_unref(cboard->canvas.img);
       gdk_gc_destroy (cboard->canvas.gc);
       gdk_window_hide (widget->window);
       cboard->canvas.img = NULL;
       cboard->canvas.gc = NULL;
   }
}


/******************************************
 *      define gcb_common callbacks       *
 ******************************************/

static inline void gcb_highlight_square_cb(void *data, short x, short y,
					   short grid, short type)
{
   GtkCBoard *board = (GtkCBoard *)data;

   g_signal_emit(GTK_OBJECT(board), gtk_cboard_signals[HIGHLIGHT_SQUARE], 0,
		   (gint)x, (gint)y, (gint)grid, (gint)type);
}

static inline void gcb_draw_background_cb(void *data,
					  short x, short y,
					  short w, short h)
{
   GtkCBoard *board = (GtkCBoard *)data;

   g_signal_emit(GTK_OBJECT(board), gtk_cboard_signals[DRAW_BACKGROUND], 0,
		   (gint)x, (gint)y, (gint)w, (gint)h);
}

static inline void gcb_refresh_area_cb(void *data, short x, short y,
				       short w, short h)
{
   GtkCBoard *cboard = (GtkCBoard *)data;

   gdk_draw_drawable(GTK_WIDGET(cboard)->window,
		     cboard->canvas.gc, cboard->canvas.img,
		     x, y, x, y, w, h);
}
     
static inline void gcb_press_square_cb(void *data, short x,
				       short y, short grid, short button)
{
   GtkCBoard *board = (GtkCBoard *)data;

   g_signal_emit(GTK_OBJECT(board), gtk_cboard_signals[PRESS_SQUARE], 0,
		   (gint)x, (gint)y, (gint)grid, (gint)button);
}
     
static inline bool gcb_drop_piece_cb(void *data, short x, short y, short grid,
				     short to_x, short to_y, short to_grid)
{
   gboolean retval;
   GtkCBoard *board = (GtkCBoard *)data;

   g_signal_emit(GTK_OBJECT(board), gtk_cboard_signals[DROP_PIECE], 0,
		   (gint)x, (gint)y, (gint)grid,
		   (gint)to_x, (gint)to_y, (gint)to_grid,
		   &retval);

   return retval;
}

/***************************************
 *       interface functions           *
 ***************************************/

void gtk_cboard_resize_grid(GtkCBoard *board, gint16 grid, gint16 x, gint16 y,
			    gint16 sw, gint16 sh, gint16 borderX, gint16 borderY)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_resize_grid(GET_BOARD(board), grid, x, y, sw, sh, borderX, borderY);
}

void gtk_cboard_resize(GtkCBoard *board, gint16 width, gint16 height)
{
   GtkWidget *widget;
   
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   if(width == board->width &&
      height == board->height)
      return;

   widget = GTK_WIDGET(board);

   if(gtk_widget_get_mapped (widget))
   {
       g_object_unref(board->canvas.img);
       board->canvas.img = gdk_pixmap_new(widget->window, board->width,
					   board->height, -1);
       gcb_set_canvas(GET_BOARD(board), *((GcbDrawable *)&board->canvas),
		      width, height);
   }
   board->width = width;
   board->height = height;

   gtk_widget_queue_resize(widget);
}

void gtk_cboard_set_background(GtkCBoard *board, GdkPixmap *pixmap)
{
   GcbDrawable draw, *dptr;

   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   dptr = gcb_get_background(GET_BOARD(board));
   if(dptr)
      g_object_unref(dptr->img);

   if(pixmap == NULL)
       gcb_unset_background(GET_BOARD(board));
   else {
       draw.img = pixmap;
       g_object_ref(pixmap);
       draw.gc  = NULL;

       gcb_set_background(GET_BOARD(board), draw);
   }
}

GdkPixmap *gtk_cboard_get_background(GtkCBoard *board)
{
   GcbDrawable *dptr;

   g_return_val_if_fail(board != NULL, NULL);
   g_return_val_if_fail(GTK_IS_CBOARD(board), NULL);

   dptr = gcb_get_background(GET_BOARD(board));

   if(dptr)
      return dptr->img;
   else
      return NULL;
}

void gtk_cboard_set_grid_highlight_normal(GtkCBoard *board, gint16 grid, GdkPixmap *pixmap,
					  GdkBitmap *mask, gint16 x, gint16 y)
{
   GcbPixmap pm;
   gint w, h;
   
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));
   
   pm.type = GTKCBOARD_PIXMAP;
   pm.pixmap.img = pixmap;
   pm.pixmap.mask = mask;
   
   if(pixmap)
      gdk_drawable_get_size(pixmap, &w, &h);
   
   gcb_set_grid_highlight_pixmap(GET_BOARD(board), grid, pm, (pixmap != NULL),
				 x, y, w, h, CBOARD_HIGHLIGHT_NORMAL);
}

void gtk_cboard_set_grid_highlight_normal_pixbuf(GtkCBoard *board, gint16 grid, GdkPixbuf *pixbuf,
					  gint16 x, gint16 y)
{
   GcbPixmap pm;
   gint w, h;
   
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   pm.type = GTKCBOARD_PIXBUF;
   pm.pixbuf = pixbuf;
   if(pixbuf) {
       w = gdk_pixbuf_get_width(pixbuf);
       h = gdk_pixbuf_get_height(pixbuf);
   }

   gcb_set_grid_highlight_pixmap(GET_BOARD(board), grid, pm, (pixbuf != NULL),
				 x, y, w, h, CBOARD_HIGHLIGHT_NORMAL);
}

void gtk_cboard_set_grid_highlight_premove(GtkCBoard *board, gint16 grid, GdkPixmap *pixmap,
					   GdkBitmap *mask, gint16 x, gint16 y)
{
   GcbPixmap pm;
   gint w, h;
   
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));
   
   pm.type = GTKCBOARD_PIXMAP;
   pm.pixmap.img = pixmap;
   pm.pixmap.mask = mask;
   if(pixmap)
      gdk_drawable_get_size(pixmap, &w, &h);

   gcb_set_grid_highlight_pixmap(GET_BOARD(board), grid, pm, (pixmap != NULL),
				 x, y, w, h, CBOARD_HIGHLIGHT_PREMOVE);
}

void gtk_cboard_set_grid_highlight_premove_pixbuf(GtkCBoard *board, gint16 grid, GdkPixbuf *pixbuf,
						  guint16 x, guint16 y)
{
   GcbPixmap pm;
   gint w, h;
   
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));
   
   pm.type = GTKCBOARD_PIXBUF;
   pm.pixbuf = pixbuf;
   if(pixbuf) {
       w = gdk_pixbuf_get_width(pixbuf);
       h = gdk_pixbuf_get_height(pixbuf);
   }

   gcb_set_grid_highlight_pixmap(GET_BOARD(board), grid, pm, (pixbuf != NULL),
				 x, y, w, h, CBOARD_HIGHLIGHT_PREMOVE);
}

void gtk_cboard_redraw_area(GtkCBoard *board, gint16 x, gint16 y,
			    guint16 w, guint16 h)
{
   GcbRect area;
   
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));
   
   area.x = x;
   area.y = y;
   area.width = w;
   area.height = h;

   gcb_redraw_area(GET_BOARD(board), area);
}

void gtk_cboard_freeze(GtkCBoard *board)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_freeze(GET_BOARD(board));
}

void gtk_cboard_thaw(GtkCBoard *board)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_thaw(GET_BOARD(board));
}

GtkCBoardOverlay * gtk_cboard_add_overlay(GtkCBoard *board, GdkPixmap *pmap, GdkBitmap *mask,
					  gint16 x, gint16 y, gint16 layer, gboolean now)
{
   GcbPixmap pixmap;
   gint w, h;
   
   g_return_val_if_fail(board != NULL, NULL);
   g_return_val_if_fail(GTK_IS_CBOARD(board), NULL);
   g_return_val_if_fail(pmap != NULL, NULL);

   pixmap.type = GTKCBOARD_PIXMAP;
   pixmap.pixmap.img = pmap;
   pixmap.pixmap.mask = mask;
   if(pmap)
      gdk_drawable_get_size(pmap, &w, &h);

   return (GtkCBoardOverlay *)
     gcb_add_overlay(GET_BOARD(board), pixmap, x, y,
		     layer, w, h, now);
}

GtkCBoardOverlay * gtk_cboard_add_overlay_pixbuf(GtkCBoard *board, GdkPixbuf *pixbuf, 
						 gint16 x, gint16 y, gint16 layer,
						 gboolean now)
{
   GcbPixmap pixmap;
   gint w, h;
   
   g_return_val_if_fail(board != NULL, NULL);
   g_return_val_if_fail(GTK_IS_CBOARD(board), NULL);
   g_return_val_if_fail(pixbuf != NULL, NULL);

   pixmap.type = GTKCBOARD_PIXBUF;
   pixmap.pixbuf = pixbuf;
   if(pixbuf) {
       w = gdk_pixbuf_get_width(pixbuf);
       h = gdk_pixbuf_get_height(pixbuf);
   }

   return (GtkCBoardOverlay *)
     gcb_add_overlay(GET_BOARD(board), pixmap, x, y,
		     layer, w, h, now);
}

GtkCBoardOverlay * gtk_cboard_add_overlay_piece(GtkCBoard *board, GtkCBoardPiece *piece, gint16 x,
						gint16 y, gint16 grid, gint16 layer, gboolean now)
{
   gint16 offset_x, offset_y;
   GcbPixmap pixmap;

   g_return_val_if_fail(board != NULL, NULL);
   g_return_val_if_fail(GTK_IS_CBOARD(board), NULL);
   g_return_val_if_fail(piece != NULL, NULL);
   g_return_val_if_fail(gcb_check_valid_square(GET_BOARD(board), x, y, grid), NULL);
	
   gcb_calculate_offset_in_square(GET_BOARD(board),
				  &offset_x, &offset_y,
				  x, y, grid,
				  piece->offset_x, piece->offset_y,
				  piece->width, piece->height,
				  piece->gravity);

   pixmap = *(GcbPixmap*)(&piece->type);
   
   return (GtkCBoardOverlay *)gcb_add_overlay(GET_BOARD(board), pixmap, 
					      offset_x, offset_y, layer,
					      piece->width, piece->height, now);
}

void gtk_cboard_remove_overlay(GtkCBoard *board, GtkCBoardOverlay *overlay)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));
   g_return_if_fail(overlay != NULL);

   gcb_remove_overlay(GET_BOARD(board), (GcbOverlay *)overlay);
}

void gtk_cboard_calculate_offset_in_square(GtkCBoard *board,
					   gint16 *pix_x, gint16 *pix_y,
					   gint16 x, gint16 y, gint16 grid,
					   gint16 offset_x, gint16 offset_y,
					   gint16 w, gint16 h, gint16 gravity)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(gcb_check_valid_square(GET_BOARD(board), x, y, grid));

   gcb_calculate_offset_in_square(GET_BOARD(board),
				  pix_x, pix_y,
				  x, y, grid,
				  offset_x, offset_y,
				  w, h, gravity);
}

void gtk_cboard_enable_highlight(GtkCBoard *board, gboolean enable)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_enable_highlight(GET_BOARD(board), enable);
}

void gtk_cboard_enable_flashing(GtkCBoard *board, gboolean enable)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_enable_flashing(GET_BOARD(board), enable);
}

void gtk_cboard_enable_dragging(GtkCBoard *board, gboolean enable)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_enable_dragging(GET_BOARD(board), enable);
}

void gtk_cboard_enable_animated_move(GtkCBoard *board, gboolean enable)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_enable_animated_move(GET_BOARD(board), enable);
}

void gtk_cboard_enable_clicking_move(GtkCBoard *board, gboolean enable)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_enable_clicking_move(GET_BOARD(board), enable);
}


void gtk_cboard_set_anim_speed(GtkCBoard *board, gint16 speed)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_set_anim_speed(GET_BOARD(board), speed);
}

void gtk_cboard_set_flash_speed(GtkCBoard *board, gint16 speed)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_set_flash_speed(GET_BOARD(board), speed);
}

void gtk_cboard_set_flash_count(GtkCBoard *board, gint16 count)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_set_flash_count(GET_BOARD(board), count);
}

void gtk_cboard_set_promote_piece(GtkCBoard *board, GtkCBoardPiece *piece)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));
   
   gcb_set_promote_piece(GET_BOARD(board), GCB_PIECE(piece));
}

void gtk_cboard_set_previous_piece(GtkCBoard *board, GtkCBoardPiece *piece)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));
   
   gcb_set_previous_piece(GET_BOARD(board), GCB_PIECE(piece));
}

void gtk_cboard_set_mid_square(GtkCBoard *board, gint16 x, gint16 y, gint16 grid)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));
   g_return_if_fail(gcb_check_valid_square(GET_BOARD(board), x, y, grid));
   
   gcb_set_mid_square(GET_BOARD(board), x, y, grid);
}

void gtk_cboard_move_piece(GtkCBoard *board,
			   gint16 from_x, gint16 from_y, gint16 from_grid,
			   gint16 to_x, gint16 to_y, gint16 to_grid,
			   gboolean silent)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));
   g_return_if_fail(gcb_check_valid_square(GET_BOARD(board), to_x, to_y, to_grid));
   g_return_if_fail(gcb_check_valid_square(GET_BOARD(board), from_x, from_y, from_grid));
   g_return_if_fail(!(from_x == to_x && from_y == to_y && from_grid == to_grid));

   gcb_move_piece(GET_BOARD(board),
		  from_x, from_y, from_grid,
		  to_x, to_y, to_grid,
		  silent);
}

void gtk_cboard_cancel_move(GtkCBoard *board)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_cancel_move(GET_BOARD(board));
}

void gtk_cboard_finish_move(GtkCBoard *board)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_finish_move(GET_BOARD(board));
}

void gtk_cboard_put_piece(GtkCBoard *board, GtkCBoardPiece *piece,
			  gint16 x, gint16 y, gint16 grid, gboolean silent)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));
   g_return_if_fail(gcb_check_valid_square(GET_BOARD(board), x, y, grid));
   
   gcb_put_piece(GET_BOARD(board), GCB_PIECE(piece), x, y, grid, silent);
}

GtkCBoardPiece *gtk_cboard_get_piece(GtkCBoard *board, gint16 x,
				     gint16 y, gint16 grid)
{
   g_return_val_if_fail(board != NULL, NULL);
   g_return_val_if_fail(GTK_IS_CBOARD(board), NULL);
   g_return_val_if_fail(gcb_check_valid_square(GET_BOARD(board), x, y, grid), NULL);
   
   return (GtkCBoardPiece *)gcb_get_piece(GET_BOARD(board), x, y, grid);
}

void gtk_cboard_end_all_animation(GtkCBoard *board)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_end_all_animation(GET_BOARD(board));
}

void gtk_cboard_delay(GtkCBoard *board, gint16 delay)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_delay(GET_BOARD(board), delay);
}

void gtk_cboard_block_piece(GtkCBoard *board, gint16 x, gint16 y, gint16 grid)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));
   g_return_if_fail(gcb_check_valid_square(GET_BOARD(board), x, y, grid));

   gcb_block_piece(GET_BOARD(board), x, y, grid);
}
   
void gtk_cboard_unblock_piece(GtkCBoard *board, gint16 x, gint16 y, gint16 grid)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));
   g_return_if_fail(gcb_check_valid_square(GET_BOARD(board), x, y, grid));

   gcb_unblock_piece(GET_BOARD(board), x, y, grid);
}
   
void gtk_cboard_set_player_side(GtkCBoard *board, gint16 side)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));
   
   gcb_set_player_side(GET_BOARD(board), side);
}

void gtk_cboard_set_active_side(GtkCBoard *board, gint16 side)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));
   
   gcb_set_active_side(GET_BOARD(board), side);
}

void gtk_cboard_block_side(GtkCBoard *board, gint16 side)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_block_side(GET_BOARD(board), side);
}

void gtk_cboard_unblock_side(GtkCBoard *board, gint16 side)
{
   g_return_if_fail(board != NULL);
   g_return_if_fail(GTK_IS_CBOARD(board));

   gcb_unblock_side(GET_BOARD(board), side);
}
