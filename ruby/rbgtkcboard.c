/*
  rbgtkcboard.c - ruby wrapper code for gtk_cboard
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

#include <ruby.h>
#include <rbgtk.h>
#include "../gtkcboard.h"

typedef struct
{
   GtkCBoardOverlay *overlay;
   GtkCBoard *board;
   int type;
   union {
      struct {
	 GdkPixmap *img;
	 GdkBitmap *mask;
      } pixmap;
      GdkPixbuf *pixbuf;
   };
} rb_GtkCBoardOverlay;

VALUE gCBoardOverlay, gCBoardPiece;

ID id_cpiece2rbpiece;

VALUE rb_gtk_cboard_overlay_new(int argc, VALUE *argv, VALUE class);
VALUE rb_gtk_cboard_piece_new(int argc, VALUE *argv, VALUE class);
void Init_gtkcboard(void);

static void
free_overlay(rb_GtkCBoardOverlay *overlay)
{
   switch(overlay->type)
   {
     case GTKCBOARD_PIXMAP:
	gdk_drawable_unref(overlay->pixmap.img);
	if(overlay->pixmap.mask)
	   gdk_drawable_unref(overlay->pixmap.mask);
	break;
     case GTKCBOARD_PIXBUF:
	g_object_unref(overlay->pixbuf);
	break;
     default:
	break;
   }
   free(overlay);
}

VALUE rb_gtk_cboard_overlay_new(int argc, VALUE *argv, VALUE class)
{
   rb_GtkCBoardOverlay *new_overlay;
   VALUE object;
   
   new_overlay = ALLOC(rb_GtkCBoardOverlay);
   memset(new_overlay, 0, sizeof(rb_GtkCBoardOverlay));

   object = Data_Wrap_Struct(class, 0, free_overlay, new_overlay);
   rb_obj_call_init(object, argc, argv);
   return object;
}

static VALUE
overlay_initialize(int argc, VALUE *argv, VALUE self)
{
   rb_GtkCBoardOverlay *overlay;
   VALUE pixmap, mask;
   
   Data_Get_Struct(self, rb_GtkCBoardOverlay, overlay);

   rb_scan_args(argc, argv, "11", &pixmap, &mask);

   overlay->board = NULL;
   overlay->overlay = NULL;
   
   if(RVAL2GTYPE(pixmap) == GDK_TYPE_PIXBUF)
   {
       if(mask != Qnil)
	  rb_raise(rb_eArgError, "mask given with pixbuf");
       overlay->type = GTKCBOARD_PIXBUF;
       overlay->pixbuf = RVAL2GOBJ(pixmap);
       g_object_ref(overlay->pixbuf);
   }
   else if(RVAL2GTYPE(pixmap) == GDK_TYPE_PIXMAP)
   {
       overlay->type = GTKCBOARD_PIXMAP;
       overlay->pixmap.img = RVAL2GOBJ(pixmap);
       gdk_drawable_ref(overlay->pixmap.img);
       if(mask == Qnil)
	  overlay->pixmap.mask = NULL;
       else if(RVAL2GTYPE(mask) == GDK_TYPE_PIXMAP) {
	   overlay->pixmap.mask = RVAL2GOBJ(mask);
	   gdk_drawable_ref(overlay->pixmap.mask);
       }else
	  rb_raise(rb_eArgError, "Mask at second argument is not a bitmap");
   }
   else
      rb_raise(rb_eArgError, "First argument is not a pixmap or a pixbuf");

   return Qnil;
}

static VALUE
overlay_dup(VALUE self)
{
   rb_GtkCBoardOverlay *new_overlay, *orig;
   VALUE object;

   Data_Get_Struct(self, rb_GtkCBoardOverlay, orig);

   new_overlay = ALLOC(rb_GtkCBoardOverlay);

   new_overlay->type = orig->type;
   switch(orig->type)
   {
     case GTKCBOARD_PIXMAP:
	new_overlay->pixmap.img = orig->pixmap.img;
	new_overlay->pixmap.mask = orig->pixmap.mask;
	gdk_drawable_ref(orig->pixmap.img);
	if(orig->pixmap.mask)
	   gdk_drawable_ref(orig->pixmap.mask);
	break;
     case GTKCBOARD_PIXBUF:
	new_overlay->pixbuf = orig->pixbuf;
	g_object_ref(orig->pixbuf);
	break;
     default:
	break;
   }
   
   new_overlay->board = NULL;
   new_overlay->overlay = NULL;
   
   object = Data_Wrap_Struct(CLASS_OF(self), 0, free_overlay, new_overlay);
   //rb_obj_call_init(object, 0, 0);
   return object;
}

static VALUE
overlay_attached(VALUE self)
{
   rb_GtkCBoardOverlay *ol;
   Data_Get_Struct(self, rb_GtkCBoardOverlay, ol);

   return (ol->board ? Qtrue : Qfalse);
}

static
void free_piece(GtkCBoardPiece *piece)
{
   switch(piece->type)
   {
     case GTKCBOARD_PIXBUF:
	if(piece->pixbuf)
	   g_object_unref(piece->pixbuf);
	break;
     case GTKCBOARD_PIXMAP:
	if(piece->pixmap.img)
	   gdk_drawable_unref(piece->pixmap.img);
	if(piece->pixmap.mask)
	   gdk_drawable_unref(piece->pixmap.mask);
	break;
     default:
	break;
   }

   free(piece);
}

VALUE rb_gtk_cboard_piece_new(int argc, VALUE *argv, VALUE class)
{
   GtkCBoardPiece *piece;
   VALUE tdata;
   
   piece = ALLOC(GtkCBoardPiece);
   memset(piece, 0, sizeof(GtkCBoardPiece));
   
   tdata = Data_Wrap_Struct(class, 0, free_piece, piece);
   rb_obj_call_init(tdata, argc, argv);

   return tdata;
}

static VALUE
piece_get_pixmap(VALUE self)
{
   GtkCBoardPiece *piece;

   Data_Get_Struct(self, GtkCBoardPiece, piece);
   switch(piece->type)
   {
     case GTKCBOARD_PIXMAP:
	return rb_ary_new3(2,
			   GOBJ2RVAL(piece->pixmap.img),
			   (piece->pixmap.mask ? GOBJ2RVAL(piece->pixmap.mask) : Qnil));
     case GTKCBOARD_PIXBUF:
	return GOBJ2RVAL(piece->pixbuf);
     default:
	break;
   }
   return Qnil;
}

static VALUE
piece_set_pixmap(VALUE self, VALUE pixarg)
{
   GtkCBoardPiece *piece, old;
   VALUE pixmap, mask;
   int w, h;

   Data_Get_Struct(self, GtkCBoardPiece, piece);
   old = *piece;

   if(TYPE(pixarg) == T_ARRAY)
   {
       if(RARRAY_LENINT(pixarg) != 2)
	  rb_raise(rb_eArgError, "number of elements in array must be 2: pixmap and mask");
       pixmap = rb_ary_entry(pixarg, 0);
       mask = rb_ary_entry(pixarg,1);
   }
   else {
       pixmap = pixarg;
       mask = Qnil;
   }
	
   if(RVAL2GTYPE(pixmap) == GDK_TYPE_PIXMAP)
   {
       if(mask == Qnil)
	  piece->pixmap.mask = NULL;
       else if(RVAL2GTYPE(mask) == GDK_TYPE_PIXMAP) {
	   piece->pixmap.mask = RVAL2GOBJ(mask);
	   gdk_drawable_ref(piece->pixmap.mask);
       } else
	  rb_raise(rb_eTypeError, "wrong type for mask: should be pixmap");

       piece->type = GTKCBOARD_PIXMAP;
       piece->pixmap.img = RVAL2GOBJ(pixmap);
       gdk_drawable_get_size(piece->pixmap.img, &w, &h);
       piece->width = w;
       piece->height = h;
       gdk_drawable_ref(piece->pixmap.img);
   }
   else if(RVAL2GTYPE(pixmap) == GDK_TYPE_PIXBUF)
   {
       if(mask != Qnil)
	  rb_raise(rb_eArgError, "a mask cannot be used together with a pixbuf");
       piece->type = GTKCBOARD_PIXBUF;
       piece->pixbuf = RVAL2GOBJ(pixmap);
       piece->width = gdk_pixbuf_get_width(piece->pixbuf);
       piece->height = gdk_pixbuf_get_height(piece->pixbuf);
       g_object_ref(piece->pixbuf);
   }
   else
      rb_raise(rb_eTypeError, "wrong type of argument: should be pixmap or pixbuf");
   
   switch(old.type)
   {
     case GTKCBOARD_PIXBUF:
	if(old.pixbuf)
	   g_object_unref(old.pixbuf);
	break;
     case GTKCBOARD_PIXMAP:
	if(old.pixmap.img)
	   gdk_drawable_unref(old.pixmap.img);
	if(old.pixmap.mask)
	   gdk_drawable_unref(old.pixmap.mask);
	break;
     default:
	break;
   }

   return pixarg;
}

static VALUE
piece_get_side(VALUE self)
{
   GtkCBoardPiece *piece;

   Data_Get_Struct(self, GtkCBoardPiece, piece);
   return INT2FIX(piece->side);
}

static VALUE
piece_set_side(VALUE self, VALUE side)
{
   GtkCBoardPiece *piece;

   Data_Get_Struct(self, GtkCBoardPiece, piece);
   piece->side = FIX2INT(side);

   return side;
}
   
static VALUE
piece_get_gravity(VALUE self)
{
   GtkCBoardPiece *piece;

   Data_Get_Struct(self, GtkCBoardPiece, piece);
   return INT2FIX(piece->gravity);
}

static VALUE
piece_set_gravity(VALUE self, VALUE gravity)
{
   GtkCBoardPiece *piece;

   Data_Get_Struct(self, GtkCBoardPiece, piece);
   piece->gravity = FIX2INT(gravity);

   return gravity;
}

static VALUE
piece_get_offset_x(VALUE self)
{
   GtkCBoardPiece *piece;

   Data_Get_Struct(self, GtkCBoardPiece, piece);
   return INT2FIX(piece->offset_x);
}

static VALUE
piece_set_offset_x(VALUE self, VALUE offset_x)
{
   GtkCBoardPiece *piece;

   Data_Get_Struct(self, GtkCBoardPiece, piece);
   piece->offset_x = FIX2INT(offset_x);

   return offset_x;
}

static VALUE
piece_get_offset_y(VALUE self)
{
   GtkCBoardPiece *piece;

   Data_Get_Struct(self, GtkCBoardPiece, piece);
   return INT2FIX(piece->offset_y);
}

static VALUE
piece_set_offset_y(VALUE self, VALUE offset_y)
{
   GtkCBoardPiece *piece;

   Data_Get_Struct(self, GtkCBoardPiece, piece);
   piece->offset_y = FIX2INT(offset_y);

   return offset_y;
}

static VALUE
rbgtkcboard_init(VALUE self, VALUE width, VALUE height, VALUE grids)
{
   int grid, n_grids;
   GtkCBoardGridInfo *ginfo;
   VALUE grid_arr;
   GtkWidget *widget;

   Check_Type(grids, T_ARRAY);
   n_grids = RARRAY_LENINT(grids);
   if(n_grids == 0)
      rb_raise(rb_eArgError, "gridinfo-array argument has no elements.");

   ginfo = ALLOCA_N(GtkCBoardGridInfo, n_grids);
   for(grid = 0; grid < n_grids; grid++)
   {
       grid_arr = rb_ary_entry(grids, grid);
       if(TYPE(grid_arr) != T_ARRAY)
	  grid_arr = rb_funcall(grid_arr, rb_intern("to_a"), 0);

       Check_Type(grid_arr, T_ARRAY);
       
       if(RARRAY_LENINT(grid_arr) != 8)
	  rb_raise(rb_eArgError,
		   "grid %i needs exactly 8 parameters, but provides %i.",
		   grid, RARRAY_LENINT(grid_arr));

       ginfo[grid].x =        FIX2UINT(rb_ary_entry(grid_arr, 0));
       ginfo[grid].y =        FIX2UINT(rb_ary_entry(grid_arr, 1));
       ginfo[grid].columns =  FIX2UINT(rb_ary_entry(grid_arr, 2));
       ginfo[grid].rows =     FIX2UINT(rb_ary_entry(grid_arr, 3));
       ginfo[grid].square_w = FIX2UINT(rb_ary_entry(grid_arr, 4));
       ginfo[grid].square_h = FIX2UINT(rb_ary_entry(grid_arr, 5));
       ginfo[grid].border_x = FIX2UINT(rb_ary_entry(grid_arr, 6));
       ginfo[grid].border_y = FIX2UINT(rb_ary_entry(grid_arr, 7));
   }

   widget = gtk_cboard_new(FIX2UINT(width), FIX2UINT(height), n_grids, ginfo);
   rb_ivar_set(self, id_cpiece2rbpiece, rb_hash_new());

   RBGTK_INITIALIZE(self, widget);
   return Qnil;
}

static VALUE
board_set_background(VALUE self, VALUE vpixmap)
{
   GdkPixmap *pixmap;

   if(NIL_P(vpixmap))
      pixmap = NULL;
   else if(RVAL2GTYPE(vpixmap) == GDK_TYPE_PIXMAP)
      pixmap = RVAL2GOBJ(vpixmap);
   else
      rb_raise(rb_eTypeError, "background must be a pixmap");
   
   gtk_cboard_set_background(GTK_CBOARD(RVAL2GOBJ(self)), pixmap);

   return vpixmap;
}

static VALUE
board_get_background(VALUE self)
{
   GdkPixmap *pixmap;

   pixmap = gtk_cboard_get_background(GTK_CBOARD(RVAL2GOBJ(self)));

   if(pixmap == NULL)
      return Qnil;
   else
      return GOBJ2RVAL(pixmap);
}

static VALUE
board_set_grid_highlight_normal(int argc, VALUE *argv, VALUE self)

{
   VALUE grid, pixmap, x, y, mask;

   rb_scan_args(argc, argv, "23", &grid, &pixmap, &mask, &x, &y);
   if(NIL_P(x)) x = INT2FIX(0);
   if(NIL_P(y)) y = INT2FIX(0);

   if(NIL_P(pixmap))
      gtk_cboard_set_grid_highlight_normal(GTK_CBOARD(RVAL2GOBJ(self)),
					   FIX2INT(grid), NULL, NULL, 0, 0);
   else if(RVAL2GTYPE(pixmap) == GDK_TYPE_PIXMAP)
   {
	if(!NIL_P(mask) &&
	   RVAL2GTYPE(mask) != GDK_TYPE_PIXMAP)
	   rb_raise(rb_eTypeError, "mask is not a bitmap");

	gtk_cboard_set_grid_highlight_normal(GTK_CBOARD(RVAL2GOBJ(self)),
					     FIX2INT(grid), RVAL2GOBJ(pixmap),
					     (NIL_P(mask) ? NULL : RVAL2GOBJ(mask)),
					     FIX2INT(x), FIX2INT(y));
   }
   else if(RVAL2GTYPE(pixmap) == GDK_TYPE_PIXBUF)
   {
       if(!NIL_P(mask))
	   rb_raise(rb_eArgError, "mask given with a pixbuf");

       gtk_cboard_set_grid_highlight_normal_pixbuf(GTK_CBOARD(RVAL2GOBJ(self)),
						   FIX2INT(grid), RVAL2GOBJ(pixmap),
						   FIX2INT(x), FIX2INT(y));
   }
   else
      rb_raise(rb_eTypeError, "argument 2 is not a pixbuf or pixmap");

   return self;
}
static VALUE
board_set_grid_highlight_premove(int argc, VALUE *argv, VALUE self)

{
   VALUE grid, pixmap, x, y, mask;

   rb_scan_args(argc, argv, "23", &grid, &pixmap, &mask, &x, &y);
   if(NIL_P(x)) x = INT2FIX(0);
   if(NIL_P(y)) y = INT2FIX(0);

   if(NIL_P(pixmap))
      gtk_cboard_set_grid_highlight_premove(GTK_CBOARD(RVAL2GOBJ(self)),
					    FIX2INT(grid), NULL, NULL, 0, 0);
   else if(RVAL2GTYPE(pixmap) == GDK_TYPE_PIXMAP)
   {
	if(!NIL_P(mask) &&
	   RVAL2GTYPE(mask) != GDK_TYPE_PIXMAP)
	   rb_raise(rb_eTypeError, "mask is not a bitmap");

	gtk_cboard_set_grid_highlight_premove(GTK_CBOARD(RVAL2GOBJ(self)),
					      FIX2INT(grid), RVAL2GOBJ(pixmap),
					     (NIL_P(mask) ? NULL : RVAL2GOBJ(mask)),
					     FIX2INT(x), FIX2INT(y));
   }
   else if(RVAL2GTYPE(pixmap) == GDK_TYPE_PIXBUF)
   {
       if(!NIL_P(mask))
	   rb_raise(rb_eArgError, "mask given with a pixbuf");

       gtk_cboard_set_grid_highlight_premove_pixbuf(GTK_CBOARD(RVAL2GOBJ(self)),
						    FIX2INT(grid), RVAL2GOBJ(pixmap),
						    FIX2INT(x), FIX2INT(y));
   }
   else
      rb_raise(rb_eTypeError, "argument 2 is not a pixbuf or pixmap");

   return self;
}


static VALUE
board_redraw(int argc, VALUE *argv, VALUE self)
{
   GtkCBoard *board = GTK_CBOARD(RVAL2GOBJ(self));
   VALUE x, y, w, h;

   switch(rb_scan_args(argc, argv, "04", &x, &y, &w, &h))
   {
     case 0:
	x = INT2FIX(0);
     case 1:
	y = INT2FIX(0);
     case 2:
	w = INT2FIX(board->width);
     case 3:
	h = INT2FIX(board->height);
     default:
	break;
   }
	
   gtk_cboard_redraw_area(board,
			  FIX2INT(x), FIX2INT(y),
			  FIX2UINT(w), FIX2UINT(h));
   return self;
}

static VALUE
board_resize(VALUE self, VALUE width, VALUE height)
{
   gtk_cboard_resize(GTK_CBOARD(RVAL2GOBJ(self)), FIX2UINT(width), FIX2UINT(height));

   return self;
}

static VALUE
board_resize_grid(VALUE self, VALUE grid, VALUE x, VALUE y,
		   VALUE sw, VALUE sh, VALUE borderX, VALUE borderY)
{
   gtk_cboard_resize_grid(GTK_CBOARD(RVAL2GOBJ(self)), FIX2INT(grid),
			  FIX2INT(x), FIX2INT(y),
			  FIX2INT(sw), FIX2INT(sh),
			  FIX2INT(borderX), FIX2INT(borderY));

   return self;
}

static VALUE
board_thaw(VALUE self)
{
   gtk_cboard_thaw(GTK_CBOARD(RVAL2GOBJ(self)));

   return self;
}

static VALUE
board_freeze(VALUE self)
{
   gtk_cboard_freeze(GTK_CBOARD(RVAL2GOBJ(self)));

   return self;
}

static VALUE
board_add_overlay(VALUE self, VALUE overlay, VALUE x, VALUE y, VALUE layer)
{
   rb_GtkCBoardOverlay *olay;
   GtkCBoard *board = GTK_CBOARD(RVAL2GOBJ(self));

   if(!rb_obj_is_kind_of(overlay, gCBoardOverlay))
      rb_raise(rb_eTypeError, "not a Gtk::CBoard::Overlay");

   Data_Get_Struct(overlay, rb_GtkCBoardOverlay, olay);
   
   if(olay->board)
      rb_raise(rb_eArgError, "Overlay is already attached");
   olay->board = board;
   
   olay->overlay = gtk_cboard_add_overlay(board,
					  olay->pixmap.img, olay->pixmap.mask,
					  FIX2INT(x), FIX2INT(y), FIX2INT(layer), FALSE);
					  
   return self;
}

static VALUE
board_add_overlay_now(VALUE self, VALUE overlay, VALUE x, VALUE y, VALUE layer)
{
   rb_GtkCBoardOverlay *olay;
   GtkCBoard *board = GTK_CBOARD(RVAL2GOBJ(self));

   if(!rb_obj_is_kind_of(overlay, gCBoardOverlay))
      rb_raise(rb_eTypeError, "not a Gtk::CBoard::Overlay");

   Data_Get_Struct(overlay, rb_GtkCBoardOverlay, olay);
   
   if(olay->board)
      rb_raise(rb_eArgError, "Overlay is already attached");
   olay->board = board;
   
   olay->overlay = gtk_cboard_add_overlay(board,
					  olay->pixmap.img, olay->pixmap.mask,
					  FIX2INT(x), FIX2INT(y), FIX2INT(layer), TRUE);

   return self;
}

static VALUE
board_add_overlay_piece(VALUE self, VALUE v_piece,
			VALUE x, VALUE y,
			VALUE grid, VALUE layer)
{
   rb_GtkCBoardOverlay *new_overlay;
   GtkCBoardPiece *piece;
   VALUE object;
   gint16 pix_x = -1, pix_y;

   if(!rb_obj_is_kind_of(v_piece, gCBoardPiece))
      rb_raise(rb_eTypeError, "not a Gtk::CBoard::Piece");
      
   Data_Get_Struct(v_piece, GtkCBoardPiece, piece);
   
   new_overlay = ALLOC(rb_GtkCBoardOverlay);
   new_overlay->board = NULL;
   new_overlay->type = piece->type;
   switch(piece->type)
   {
     case GTKCBOARD_PIXMAP:
	new_overlay->pixmap.img = piece->pixmap.img;
	gdk_drawable_ref(new_overlay->pixmap.img);
	if(! piece->pixmap.mask)
	   new_overlay->pixmap.mask = NULL;
	else {
	    new_overlay->pixmap.mask = piece->pixmap.mask;
	    gdk_drawable_ref(piece->pixmap.mask);
	}
	break;
     case GTKCBOARD_PIXBUF:
	new_overlay->pixbuf = piece->pixbuf;
	g_object_ref(piece->pixbuf);
	break;
     default:
	break;
   }

   gtk_cboard_calculate_offset_in_square(GTK_CBOARD(RVAL2GOBJ(self)),
					 &pix_x, &pix_y,
					 FIX2INT(x), FIX2INT(y), FIX2INT(grid),
					 piece->offset_x, piece->offset_y,
					 piece->width, piece->height, piece->gravity);
   object = Data_Wrap_Struct(gCBoardOverlay, 0, free_overlay, new_overlay);
   if(pix_x != -1)
      board_add_overlay(self, object, INT2FIX(pix_x), INT2FIX(pix_y), layer);
   return object;
}

static VALUE
board_add_overlay_piece_now(VALUE self, VALUE v_piece,
			    VALUE x, VALUE y,
			    VALUE grid, VALUE layer)
{
   rb_GtkCBoardOverlay *new_overlay;
   GtkCBoardPiece *piece;
   VALUE object;
   gint16 pix_x = -1, pix_y;

   if(!rb_obj_is_kind_of(v_piece, gCBoardPiece))
      rb_raise(rb_eTypeError, "not a Gtk::CBoard::Piece");
      
   Data_Get_Struct(v_piece, GtkCBoardPiece, piece);
   
   new_overlay = ALLOC(rb_GtkCBoardOverlay);
   new_overlay->board = NULL;
   new_overlay->type = piece->type;
   switch(piece->type)
   {
     case GTKCBOARD_PIXMAP:
	new_overlay->pixmap.img = piece->pixmap.img;
	gdk_drawable_ref(new_overlay->pixmap.img);
	if(! piece->pixmap.mask)
	   new_overlay->pixmap.mask = NULL;
	else {
	    new_overlay->pixmap.mask = piece->pixmap.mask;
	    gdk_drawable_ref(piece->pixmap.mask);
	}
	break;
     case GTKCBOARD_PIXBUF:
	new_overlay->pixbuf = piece->pixbuf;
	g_object_ref(piece->pixbuf);
	break;
     default:
	break;
   }

   gtk_cboard_calculate_offset_in_square(GTK_CBOARD(RVAL2GOBJ(self)),
					 &pix_x, &pix_y,
					 FIX2INT(x), FIX2INT(y), FIX2INT(grid),
					 piece->offset_x, piece->offset_y,
					 piece->width, piece->height, piece->gravity);
   object = Data_Wrap_Struct(gCBoardOverlay, 0, free_overlay, new_overlay);
   if(pix_x != -1)
      board_add_overlay_now(self, object, INT2FIX(pix_x), INT2FIX(pix_y), layer);
   return object;
}


static VALUE
board_calculate_offset_in_square(VALUE self,
				 VALUE x, VALUE y, VALUE grid,
				 VALUE offset_x, VALUE offset_y,
				 VALUE w, VALUE h, VALUE gravity)
{
   gint16 pix_x = -1, pix_y;

   gtk_cboard_calculate_offset_in_square(GTK_CBOARD(RVAL2GOBJ(self)),
					 &pix_x, &pix_y,
					 FIX2INT(x), FIX2INT(y), FIX2INT(grid),
					 FIX2INT(offset_x), FIX2INT(offset_y),
					 FIX2INT(w), FIX2INT(h), FIX2INT(gravity));
   if(pix_x != -1)
      return rb_ary_new3(2, INT2FIX(pix_x), INT2FIX(pix_y));
   else
      return Qnil;
}

static VALUE
board_remove_overlay(VALUE self, VALUE overlay)
{
   rb_GtkCBoardOverlay *olay;
   GtkCBoard *board = GTK_CBOARD(RVAL2GOBJ(self));
   
   Data_Get_Struct(overlay, rb_GtkCBoardOverlay, olay);

   if(olay->board != board)
      rb_raise(rb_eArgError, "Overlay is not attached to this board");
   olay->board = NULL;

   gtk_cboard_remove_overlay(board, olay->overlay);

   return self;
}

static VALUE
board_end_all_animation(VALUE self)
{
   GtkCBoard *board = GTK_CBOARD(RVAL2GOBJ(self));

   gtk_cboard_end_all_animation(board);
   return self;
}

static VALUE
board_enable_highlight(VALUE self, VALUE enable)
{
   if(enable != Qtrue && enable != Qfalse && enable != Qnil)
      rb_raise(rb_eTypeError, "enable_highlight takes a boolean argument");
   
   gtk_cboard_enable_highlight(GTK_CBOARD(RVAL2GOBJ(self)),
			       enable == Qtrue ? TRUE : FALSE);

   return enable;
}

static VALUE
board_enable_flashing(VALUE self, VALUE enable)
{
   if(enable != Qtrue && enable != Qfalse && enable != Qnil)
      rb_raise(rb_eTypeError, "enable_flashing takes a boolean argument");
   
   gtk_cboard_enable_flashing(GTK_CBOARD(RVAL2GOBJ(self)),
			      enable == Qtrue ? TRUE : FALSE);

   return enable;
}

static VALUE
board_enable_dragging(VALUE self, VALUE enable)
{
   if(enable != Qtrue && enable != Qfalse)
      rb_raise(rb_eTypeError, "enable_dragging takes a boolean argument");
   
   gtk_cboard_enable_dragging(GTK_CBOARD(RVAL2GOBJ(self)),
			      enable == Qtrue ? TRUE : FALSE);

   return enable;
}

static VALUE
board_enable_animated_move(VALUE self, VALUE enable)
{
   if(enable != Qtrue && enable != Qfalse && enable != Qnil)
      rb_raise(rb_eTypeError, "enable_animated_move takes a boolean argument");
   
   gtk_cboard_enable_animated_move(GTK_CBOARD(RVAL2GOBJ(self)),
				   enable == Qtrue ? TRUE : FALSE);

   return self;
}

static VALUE
board_enable_clicking_move(VALUE self, VALUE enable)
{
   if(enable != Qtrue && enable != Qfalse && enable != Qnil)
      rb_raise(rb_eTypeError, "enable_animated_move takes a boolean argument");
   
   gtk_cboard_enable_clicking_move(GTK_CBOARD(RVAL2GOBJ(self)),
				   enable == Qtrue ? TRUE : FALSE);

   return enable;
}

static VALUE
board_set_anim_speed(VALUE self, VALUE speed)
{
   gtk_cboard_set_anim_speed(GTK_CBOARD(RVAL2GOBJ(self)), INT2FIX(speed));

   return speed;
}

static VALUE
board_set_flash_speed(VALUE self, VALUE speed)
{
   gtk_cboard_set_flash_speed(GTK_CBOARD(RVAL2GOBJ(self)), INT2FIX(speed));

   return speed;
}

static VALUE
board_set_flash_count(VALUE self, VALUE count)
{
   gtk_cboard_set_flash_count(GTK_CBOARD(RVAL2GOBJ(self)), INT2FIX(count));

   return count;
}

static VALUE
board_set_promote_piece(VALUE self, VALUE v_piece)
{
   GtkCBoardPiece *piece;
   VALUE piece_list;

   if(v_piece == Qnil)
      piece = NULL;
   else {
       if(!rb_obj_is_kind_of(v_piece, gCBoardPiece))
	  rb_raise(rb_eTypeError, "not a Gtk::CBoard::Piece");
       
       Data_Get_Struct(v_piece, GtkCBoardPiece, piece);
   }

   gtk_cboard_set_promote_piece(GTK_CBOARD(RVAL2GOBJ(self)), piece);
   piece_list = rb_ivar_get(self, id_cpiece2rbpiece);
   rb_hash_aset(piece_list, INT2NUM((VALUE)piece), v_piece);

   return v_piece;
}

static VALUE
board_set_previous_piece(VALUE self, VALUE v_piece)
{
   GtkCBoardPiece *piece;
   VALUE piece_list;
   
   if(v_piece == Qnil)
      piece = NULL;
   else {
       if(!rb_obj_is_kind_of(v_piece, gCBoardPiece))
	  rb_raise(rb_eTypeError, "not a Gtk::CBoard::Piece");
   
       Data_Get_Struct(v_piece, GtkCBoardPiece, piece);
   }

   gtk_cboard_set_previous_piece(GTK_CBOARD(RVAL2GOBJ(self)), piece);
   piece_list = rb_ivar_get(self, id_cpiece2rbpiece);
   rb_hash_aset(piece_list, INT2NUM((VALUE)piece), v_piece);

   return v_piece;
}

static VALUE
board_set_mid_square(VALUE self, VALUE x, VALUE y, VALUE grid)
{
   gtk_cboard_set_mid_square(GTK_CBOARD(RVAL2GOBJ(self)),
			     FIX2INT(x),
			     FIX2INT(y),
			     FIX2INT(grid));
   return self;
}

static VALUE
board_move_piece(int argc, VALUE *argv, VALUE self)
{
   VALUE to_x, to_y, to_grid;
   VALUE from_x, from_y, from_grid, silent;
   
   if(rb_scan_args(argc, argv, "61",
		   &from_x, &from_y, &from_grid,
		   &to_x, &to_y, &to_grid,
		   &silent) == 6)
      silent = Qfalse;
   
   if(silent != Qtrue && silent != Qfalse && silent != Qnil)
      rb_raise(rb_eTypeError, "silent argument to move_piece needs to boolean");

   gtk_cboard_move_piece(GTK_CBOARD(RVAL2GOBJ(self)),
			 FIX2INT(from_x), FIX2INT(from_y), FIX2INT(from_grid),
			 FIX2INT(to_x), FIX2INT(to_y), FIX2INT(to_grid),
			 silent == Qtrue ? TRUE : FALSE);

   return self;
}

static VALUE
board_cancel_move(VALUE self)
{
   gtk_cboard_cancel_move(GTK_CBOARD(RVAL2GOBJ(self)));

   return self;
}

static VALUE
board_finish_move(VALUE self)
{
   gtk_cboard_finish_move(GTK_CBOARD(RVAL2GOBJ(self)));

   return self;
}

static VALUE
board_put_piece(int argc, VALUE *argv, VALUE self)
{
   VALUE vpiece, x, y, grid, silent;
   VALUE piece_list;
   GtkCBoardPiece *piece;

   rb_scan_args(argc, argv, "41", &vpiece, 
		&x, &y, &grid, &silent);

   if(vpiece == Qnil)
      piece = NULL;
   else
   {
       if(!rb_obj_is_kind_of(vpiece, gCBoardPiece))
	  rb_raise(rb_eTypeError, "not a Piece");

       Data_Get_Struct(vpiece, GtkCBoardPiece, piece);
   }

   if(silent != Qtrue && silent != Qfalse && silent != Qnil)
      rb_raise(rb_eTypeError, "silent argument of put_piece needs to be boolean.");

   gtk_cboard_put_piece(GTK_CBOARD(RVAL2GOBJ(self)), piece, FIX2INT(x), FIX2INT(y), FIX2INT(grid),
			silent == Qtrue ? TRUE : FALSE);

   piece_list = rb_ivar_get(self, id_cpiece2rbpiece);
   rb_hash_aset(piece_list, INT2NUM((VALUE)piece), vpiece);

   return self;
}

static VALUE
board_get_piece(VALUE self, VALUE x, VALUE y, VALUE grid)
{
   GtkCBoardPiece *piece;
   VALUE piece_list;

   piece = gtk_cboard_get_piece(GTK_CBOARD(RVAL2GOBJ(self)),
				FIX2INT(x), FIX2INT(y), FIX2INT(grid));

   if(piece)
   {
       piece_list = rb_ivar_get(self, id_cpiece2rbpiece);
       return rb_hash_aref(piece_list, INT2NUM((VALUE)piece));
   }
   else
      return Qnil;
}

static VALUE
board_delay(VALUE self, VALUE duration)
{
   gtk_cboard_delay(GTK_CBOARD(RVAL2GOBJ(self)),
		    FIX2INT(duration));

   return self;
}

static VALUE
board_block_piece(VALUE self, VALUE x, VALUE y, VALUE grid)
{
   gtk_cboard_block_piece(GTK_CBOARD(RVAL2GOBJ(self)),
			  FIX2INT(x),
			  FIX2INT(y),
			  FIX2INT(grid));

   return self;
}

static VALUE
board_unblock_piece(VALUE self, VALUE x, VALUE y, VALUE grid)
{
   gtk_cboard_unblock_piece(GTK_CBOARD(RVAL2GOBJ(self)),
			    FIX2INT(x),
			    FIX2INT(y),
			    FIX2INT(grid));

   return self;
}

static VALUE
board_block_side(VALUE self, VALUE side)
{
   gtk_cboard_block_side(GTK_CBOARD(RVAL2GOBJ(self)),
			 FIX2INT(side));

   return self;
}

static VALUE
board_unblock_side(VALUE self, VALUE side)
{
   gtk_cboard_unblock_side(GTK_CBOARD(RVAL2GOBJ(self)),
			   FIX2INT(side));

   return self;
}

static VALUE
board_set_active_side(VALUE self, VALUE side)
{
   gtk_cboard_set_active_side(GTK_CBOARD(RVAL2GOBJ(self)),
			      FIX2INT(side));

   return side;
}

static VALUE
board_set_player_side(VALUE self, VALUE side)
{
   gtk_cboard_set_player_side(GTK_CBOARD(RVAL2GOBJ(self)),
			      FIX2INT(side));

   return side;
}

void Init_gtkcboard(void)
{
   VALUE gCBoard = G_DEF_CLASS(GTK_TYPE_CBOARD, "CBoard", mGtk);

   gCBoardOverlay =   rb_define_class_under(gCBoard, "Overlay", rb_cObject);
   gCBoardPiece =     rb_define_class_under(gCBoard, "Piece", rb_cObject);

   id_cpiece2rbpiece = rb_intern("c_piece_to_ruby_piece_hash");

   /* constants */
   rb_define_const(gCBoard, "GRAVITY_CENTER",  INT2FIX(CBOARD_GRAVITY_CENTER));
   rb_define_const(gCBoard, "GRAVITY_NORTH",   INT2FIX(CBOARD_GRAVITY_NORTH));
   rb_define_const(gCBoard, "GRAVITY_SOUTH",   INT2FIX(CBOARD_GRAVITY_SOUTH));
   rb_define_const(gCBoard, "GRAVITY_EAST",    INT2FIX(CBOARD_GRAVITY_EAST));
   rb_define_const(gCBoard, "GRAVITY_WEST",    INT2FIX(CBOARD_GRAVITY_WEST));
   rb_define_const(gCBoard, "GRAVITY_NW",      INT2FIX(CBOARD_GRAVITY_NW));
   rb_define_const(gCBoard, "GRAVITY_NE",      INT2FIX(CBOARD_GRAVITY_NE));
   rb_define_const(gCBoard, "GRAVITY_SW",      INT2FIX(CBOARD_GRAVITY_SW));
   rb_define_const(gCBoard, "GRAVITY_SE",      INT2FIX(CBOARD_GRAVITY_SE));

   rb_define_const(gCBoard, "HIGHLIGHT_OFF",      INT2FIX(CBOARD_HIGHLIGHT_OFF));
   rb_define_const(gCBoard, "HIGHLIGHT_NORMAL",   INT2FIX(CBOARD_HIGHLIGHT_NORMAL));
   rb_define_const(gCBoard, "HIGHLIGHT_PREMOVE" , INT2FIX(CBOARD_HIGHLIGHT_PREMOVE));

   /* gCBoardOverlay methods*/
   rb_define_singleton_method(gCBoardOverlay, "new", rb_gtk_cboard_overlay_new, -1);
   rb_define_method(gCBoardOverlay, "initialize", overlay_initialize, -1);
   rb_define_method(gCBoardOverlay, "dup", overlay_dup, 0);
   rb_define_method(gCBoardOverlay, "attached", overlay_attached, 0);
   
   /* gCBoardPiece methods */
   rb_define_singleton_method(gCBoardPiece, "new", rb_gtk_cboard_piece_new, -1);
   rb_define_method(gCBoardPiece, "pixmap", piece_get_pixmap, 0);
   rb_define_method(gCBoardPiece, "pixmap=", piece_set_pixmap, 1);
   rb_define_method(gCBoardPiece, "side", piece_get_side, 0);
   rb_define_method(gCBoardPiece, "side=", piece_set_side, 1);
   rb_define_method(gCBoardPiece, "gravity", piece_get_gravity, 0);
   rb_define_method(gCBoardPiece, "gravity=", piece_set_gravity, 1);
   rb_define_method(gCBoardPiece, "offset_x", piece_get_offset_x, 0);
   rb_define_method(gCBoardPiece, "offset_x=", piece_set_offset_x, 1);
   rb_define_method(gCBoardPiece, "offset_y", piece_get_offset_y, 0);
   rb_define_method(gCBoardPiece, "offset_y=", piece_set_offset_y, 1);

   /*gCBoard methods */
   rb_define_method(gCBoard, "initialize", rbgtkcboard_init, 3);
   rb_define_method(gCBoard, "resize", board_resize, 2);
   rb_define_method(gCBoard, "resize_grid", board_resize_grid, 7);
   rb_define_method(gCBoard, "background=", board_set_background, 1);
   rb_define_method(gCBoard, "background", board_get_background, 0);
   rb_define_method(gCBoard, "set_grid_highlight_normal", board_set_grid_highlight_normal, -1);
   rb_define_method(gCBoard, "set_grid_highlight_premove", board_set_grid_highlight_premove, -1);
   rb_define_method(gCBoard, "redraw", board_redraw, 4);
   rb_define_method(gCBoard, "freeze", board_freeze, 0);
   rb_define_method(gCBoard, "thaw", board_thaw, 0);
   rb_define_method(gCBoard, "add_overlay", board_add_overlay, 4);
   rb_define_method(gCBoard, "add_overlay_now", board_add_overlay_now, 4);
   rb_define_method(gCBoard, "add_overlay_piece", board_add_overlay_piece, 5);
   rb_define_method(gCBoard, "add_overlay_piece_now", board_add_overlay_piece_now, 5);
   rb_define_method(gCBoard, "remove_overlay", board_remove_overlay, 1);
   rb_define_method(gCBoard, "calculate_offset_in_square", board_calculate_offset_in_square, 8);
   rb_define_method(gCBoard, "enable_highlight=", board_enable_highlight, 1);
   rb_define_method(gCBoard, "enable_flashing=", board_enable_flashing, 1);
   rb_define_method(gCBoard, "enable_dragging=", board_enable_dragging, 1);
   rb_define_method(gCBoard, "enable_animated_move=", board_enable_animated_move, 1);
   rb_define_method(gCBoard, "enable_clicking_move=", board_enable_clicking_move, 1);
   rb_define_method(gCBoard, "anim_speed=", board_set_anim_speed, 1);
   rb_define_method(gCBoard, "flash_speed=", board_set_flash_speed, 1);
   rb_define_method(gCBoard, "flash_count=", board_set_flash_count, 1);
   rb_define_method(gCBoard, "promote_piece=", board_set_promote_piece, 1);
   rb_define_method(gCBoard, "previous_piece=", board_set_previous_piece, 1);
   rb_define_method(gCBoard, "set_mid_square", board_set_mid_square, 3);
   rb_define_method(gCBoard, "move_piece", board_move_piece ,-1);
   rb_define_method(gCBoard, "cancel_move", board_cancel_move, 0);
   rb_define_method(gCBoard, "finish_move", board_finish_move, 0);
   rb_define_method(gCBoard, "put_piece", board_put_piece, -1);
   rb_define_method(gCBoard, "get_piece", board_get_piece, 3);
   rb_define_method(gCBoard, "end_all_animation", board_end_all_animation, 0);
   rb_define_method(gCBoard, "delay", board_delay, 1);
   rb_define_method(gCBoard, "block_piece", board_block_piece, 2);
   rb_define_method(gCBoard, "unblock_piece", board_unblock_piece, 2);
   rb_define_method(gCBoard, "player_side=", board_set_player_side, 1);
   rb_define_method(gCBoard, "active_side=", board_set_active_side, 1);
   rb_define_method(gCBoard, "block_side", board_block_side, 1);
   rb_define_method(gCBoard, "unblock_side", board_unblock_side, 1);
   
   /* Signals */
   rb_define_const(gCBoard, "SIGNAL_PRESS_SQUARE",     rb_str_new2("press_square"));
   rb_define_const(gCBoard, "SIGNAL_HIGHLIGHT_SQUARE", rb_str_new2("highlight_square"));
   rb_define_const(gCBoard, "SIGNAL_DRAW_BACKGROUND",  rb_str_new2("draw_background"));
   rb_define_const(gCBoard, "SIGNAL_DROP_PIECE",       rb_str_new2("drop_piece"));
}
