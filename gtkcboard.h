/*
  gtkcboard.h - Generic Chess Board
  declarations for gtk+ cboard-widget
  http://gcboard.sourceforge.net
  Copyright (C) 2015 Kristof Bastiaensen
  
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

#ifndef __GTKCBOARD_H__
#define __GTKCBOARD_H__

#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#define GTK_TYPE_CBOARD         (gtk_cboard_get_type())
#define GTK_CBOARD(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_CBOARD, GtkCBoard))
#define GTK_IS_CBOARD(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_CBOARD))
#define GTK_CBOARD_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), G_TYPE_CBOARD, GtkBoardClass))

typedef struct _GtkCBoard          GtkCBoard;
typedef struct _GtkCBoardClass     GtkCBoardClass;
typedef struct _GtkCBoardOverlay   GtkCBoardOverlay;
typedef struct _GtkCBoardPiece     GtkCBoardPiece;
typedef struct _GtkCBoardGridInfo  GtkCBoardGridInfo;

enum
{
   CBOARD_GRAVITY_CENTER = 0,

   CBOARD_GRAVITY_NORTH  = 1,
   CBOARD_GRAVITY_SOUTH  = 2,
   CBOARD_GRAVITY_EAST   = 4,
   CBOARD_GRAVITY_WEST   = 8,
   
   CBOARD_GRAVITY_NW     = (CBOARD_GRAVITY_NORTH | CBOARD_GRAVITY_WEST),
   CBOARD_GRAVITY_NE     = (CBOARD_GRAVITY_NORTH | CBOARD_GRAVITY_EAST),
   CBOARD_GRAVITY_SW     = (CBOARD_GRAVITY_SOUTH | CBOARD_GRAVITY_WEST),
   CBOARD_GRAVITY_SE     = (CBOARD_GRAVITY_SOUTH | CBOARD_GRAVITY_EAST)
};

enum
{
   CBOARD_HIGHLIGHT_OFF     = 0,
   CBOARD_HIGHLIGHT_NORMAL,
   CBOARD_HIGHLIGHT_PREMOVE
};

enum
{
   GTKCBOARD_PIXBUF = 0,
   GTKCBOARD_PIXMAP
};


#define CBOARD_NO_PIECE (GtkCBoardPiece *)NULL

struct _GtkCBoardPiece
{
   int type;
   union {
      GdkPixbuf * pixbuf;
      struct {
	 GdkPixmap *img;
	 GdkBitmap *mask;
      } pixmap;
   };
   guint8 side;
   gint gravity : 4;
   gint16 offset_x;
   gint16 offset_y;
   guint16 width;
   guint16 height;
};

/*
struct _GtkCBoardOverlay
{  gint dummy;  };
*/

struct _GtkCBoard
{
   GtkWidget widget;
   
   struct {
      GdkPixmap *img;
      GdkGC *gc;
   } canvas;
   guint16 width, height;
   gpointer board;
};

struct _GtkCBoardClass
{
   GtkWidgetClass parent_class;

   void (* press_square)     (GtkCBoard *,
			      gint x,
			      gint y,
			      gint grid,
			      gint button);
   void (* highlight_square) (GtkCBoard *,
			      gint x,
			      gint y,
			      gint grid,
			      gint type);
   void (* draw_background)  (GtkCBoard *,
			      gint x,
			      gint y,
			      gint width,
			      gint height);
   gboolean (* drop_piece)   (GtkCBoard *,
			      gint,
			      gint,
			      gint,
			      gint,
			      gint,
			      gint);

   void (* init_board)       (GtkCBoard *);
};

struct _GtkCBoardGridInfo
{
   guint16 x, y;
   guint16 columns, rows;
   guint16 square_w, square_h;
   guint16 border_x, border_y;
};

GType gtk_cboard_get_type (void);
GtkWidget *gtk_cboard_new      (guint16   width, guint16   height, gint16    grids, const GtkCBoardGridInfo *ginfo);
void gtk_cboard_resize_grid(GtkCBoard *board, gint16 grid, gint16 x, gint16 y,
			    gint16 sw, gint16 sh, gint16 borderX, gint16 borderY);
void gtk_cboard_resize(GtkCBoard *board, gint16 width, gint16 height);
void gtk_cboard_set_background(GtkCBoard *board, GdkPixmap *pixmap);
GdkPixmap *gtk_cboard_get_background(GtkCBoard *board);
void gtk_cboard_set_grid_highlight_normal(GtkCBoard *board, gint16 grid, GdkPixmap *pixmap,
					  GdkBitmap *mask, gint16 x, gint16 y);
void gtk_cboard_set_grid_highlight_normal_pixbuf(GtkCBoard *board, gint16 grid, GdkPixbuf *pixmap,
						 gint16 x, gint16 y);
void gtk_cboard_set_grid_highlight_premove(GtkCBoard *board, gint16 grid, GdkPixmap *pixmap,
					   GdkBitmap *mask, gint16 x, gint16 y);
void gtk_cboard_set_grid_highlight_premove_pixbuf(GtkCBoard *board, gint16 grid, GdkPixbuf *pixbuf,
						  guint16 x, guint16 y);
void gtk_cboard_redraw_area(GtkCBoard *board, gint16 x, gint16 y, guint16 w, guint16 h);
void gtk_cboard_freeze(GtkCBoard *board);
void gtk_cboard_thaw(GtkCBoard *board);
GtkCBoardOverlay *gtk_cboard_add_overlay(GtkCBoard *board, GdkPixmap *pmap, GdkBitmap *mask, gint16 x, gint16 y, gint16 layer, gboolean now);
GtkCBoardOverlay *gtk_cboard_add_overlay_pixbuf(GtkCBoard *board, GdkPixbuf *pbuf, gint16 x, gint16 y, gint16 layer, gboolean now);
GtkCBoardOverlay * gtk_cboard_add_overlay_from_piece(GtkCBoard *board, GtkCBoardPiece *piece, gint16 x,
						     gint16 y, gint16 grid, gint16 layer, gboolean now);
void gtk_cboard_calculate_offset_in_square(GtkCBoard *board,
					   gint16 *pix_x, gint16 *pix_y,
					   gint16 x, gint16 y, gint16 grid,
					   gint16 offset_x, gint16 offset_y,
					   gint16 w, gint16 h, gint16 gravity);
void gtk_cboard_remove_overlay(GtkCBoard *board, GtkCBoardOverlay *overlay);
void gtk_cboard_enable_highlight(GtkCBoard *board, gboolean enable);
void gtk_cboard_enable_flashing(GtkCBoard *board, gboolean enable);
void gtk_cboard_enable_dragging(GtkCBoard *board, gboolean enable);
void gtk_cboard_enable_animated_move(GtkCBoard *board, gboolean enable);
void gtk_cboard_enable_clicking_move(GtkCBoard *board, gboolean enable);
void gtk_cboard_set_anim_speed(GtkCBoard *board, gint16 speed);
void gtk_cboard_set_flash_speed(GtkCBoard *board, gint16 speed);
void gtk_cboard_set_flash_count(GtkCBoard *board, gint16 count);
void gtk_cboard_set_promote_piece(GtkCBoard *board, GtkCBoardPiece *piece);
void gtk_cboard_set_previous_piece(GtkCBoard *board, GtkCBoardPiece *piece);
void gtk_cboard_set_mid_square(GtkCBoard *board, gint16 x, gint16 y, gint16 grid);
void gtk_cboard_end_all_animation(GtkCBoard *board);
void gtk_cboard_move_piece(GtkCBoard *board,
			   gint16 from_x, gint16 from_y, gint16 from_grid,
			   gint16 to_x, gint16 to_y, gint16 to_grid,
			   gboolean silent);
void gtk_cboard_cancel_move(GtkCBoard *board);
void gtk_cboard_finish_move(GtkCBoard *board);
void gtk_cboard_put_piece(GtkCBoard *board, GtkCBoardPiece *piece, gint16 x, gint16 y, gint16 grid, gboolean silent);
GtkCBoardPiece *gtk_cboard_get_piece(GtkCBoard *board, gint16 x, gint16 y, gint16 grid);
void gtk_cboard_delay(GtkCBoard *board, gint16 delay);
void gtk_cboard_block_piece(GtkCBoard *board, gint16 x, gint16 y, gint16 grid);
void gtk_cboard_unblock_piece(GtkCBoard *board, gint16 x, gint16 y, gint16 grid);
void gtk_cboard_set_player_side(GtkCBoard *board, gint16 side);
void gtk_cboard_set_active_side(GtkCBoard *board, gint16 side);
void gtk_cboard_block_side(GtkCBoard *board, gint16 side);
void gtk_cboard_unblock_side(GtkCBoard *board, gint16 side);

#ifdef __cplusplus
}
#endif

#endif /* __GTKCBOARD_H__ */
