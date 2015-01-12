/*
  gcb_common.c - code common for all platforms
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

#include <malloc.h>
#include <stdarg.h>
#include <string.h>
#include "config.h"

#ifndef HAVE_MALLOC
#error don't say your compiler doesn't have malloc!
#endif

#ifndef HAVE_MEMSET
#error don't say your compiler doesn't have memset!
#endif

#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif
#define abs(x) ((x) < 0 ? -(x) : x)
#define max(x, y) (x > y ? x : y)
#define min(x, y) (x < y ? x : y)

enum
{
   ANIM_MOVE_PIECE,
   ANIM_SET_PIECE,
   ANIM_DELAY,
   ANIM_ADD_OVERLAY,
   ANIM_REMOVE_OVERLAY,
   ANIM_SET_PROMOTE_PIECE,
   ANIM_SET_PREVIOUS_PIECE
};

#define HIGHLIGHT_LAYER  50
#define PIECE_LAYER      100

static    bool gcb_pixel_to_square(GcbBoard *board, short x, short y, short *sx,
				   short *sy, short *grid);
static    void gcb_redraw_damaged_area(GcbBoard *board);
GCB_SCOPE void gcb_redraw_area(GcbBoard *board, GcbRect area);
GCB_SCOPE GcbBoard *gcb_new_board(short grids, void *data);
GCB_SCOPE void gcb_delete_board(GcbBoard *board);
static    void gcb_set_grid(GcbBoard *board, short grid, short x, short y, short columns,
			    short rows, short sw, short sh, short borderX, short borderY);
GCB_SCOPE void gcb_resize_grid(GcbBoard *board, short grid, short x, short y,
					 short sw, short sh, short borderX, short borderY);
static    void gcb_end_all_animation(GcbBoard *board);
GCB_SCOPE void gcb_freeze(GcbBoard *board);
GCB_SCOPE void gcb_thaw(GcbBoard *board);
GCB_SCOPE GcbOverlay *gcb_add_overlay(GcbBoard *board, GcbPixmap pmap, short x, short y,
				      short layer, short width, short height, bool now);
static    void gcb_do_add_overlay(GcbBoard *board, GcbOverlay *newnode);
GCB_SCOPE void gcb_remove_overlay(GcbBoard *board, GcbOverlay *overlayw);
static    void gcb_do_remove_overlay(GcbBoard *board, GcbOverlay *overlay);
static    void gcb_anim_piece_begin(GcbBoard *board, AnimatedPiece *apiece);
static    void gcb_update_dragged_piece(GcbBoard *board);
static    void gcb_anim_piece_clear(GcbBoard *board, AnimatedPiece *apiece, bool moving);
static    void gcb_anim_piece_draw(GcbBoard *board, AnimatedPiece *apiece, bool moving);
static    void gcb_anim_piece_end(GcbBoard *board, AnimatedPiece *piece);
static    void gcb_reset_if_new_move(GcbBoard *board);
static    void gcb_do_set_promote_piece(GcbBoard *board, GcbPiece *piece);
GCB_SCOPE void gcb_set_promote_piece(GcbBoard *board, GcbPieceType *piece);
GCB_SCOPE void gcb_move_piece(GcbBoard *board,
			      short from_x, short from_y, short grid,
			      short to_x, short to_y, short to_grid,
			      bool silent);
static    void gcb_begin_move_piece(GcbBoard *board, GcbPiece *piece,
				    short mid_x, short mid_y, short mid_grid,
				    short to_x, short to_y, short to_grid,
				    bool silent);
static    void gcb_move_next_frame(GcbBoard *board);
static    void gcb_stop_moving(GcbBoard *board);
static    void gcb_begin_flash_piece(GcbBoard *board, GcbPiece *piece);
static    void gcb_do_flash_frame(GcbBoard *board);
static    void gcb_stop_flashing(GcbBoard *board);
static    void gcb_start_dragging(GcbBoard *board, short x, short y);
static    void gcb_drag_piece(GcbBoard *board, short x, short y);
static    void gcb_finish_dragging(GcbBoard *board);
static    void gcb_cancel_dragging(GcbBoard *board);
GCB_SCOPE void gcb_cancel_move(GcbBoard *board);
GCB_SCOPE void gcb_finish_move(GcbBoard *board);
static    void gcb_on_press(GcbBoard *board, short x, short y, short button);
static    void gcb_on_release(GcbBoard *board, short x, short y, short button);
static    void gcb_on_mouse_move(GcbBoard *board, int x, int y);
static    void gcb_do_set_piece(GcbBoard *board, GcbPiece *piece, short x, short y, short grid, bool silent, bool moving);
GCB_SCOPE void gcb_put_piece(GcbBoard *board, GcbPieceType *piece, short x, short y, short grid, bool silent);
GCB_SCOPE GcbPieceType *gcb_get_piece(GcbBoard *board, short x, short y, short grid);
static    void gcb_anim_queue_add(GcbBoard *board, short func, void *data, ...);
static    void gcb_anim_queue_call_next(GcbBoard *board);
GCB_SCOPE void gcb_delay(GcbBoard *board, short delay);
static    void gcb_end_delay(GcbBoard *board);
static    void gcb_do_delay(GcbBoard *board, short delay);


static inline
bool point_inside_rect(GcbRect r, short x, short y)
{
   return (x >= r.x && x < r.x + r.width &&
	   y >= r.y && y < r.y + r.height);
}

static
bool rect_intersect(GcbRect r1, GcbRect r2, GcbRect *sect, GcbPoint *offset)
{
   if( (r1.x + r1.width <= r2.x)  ||
       (r2.x + r2.width <= r1.x)  ||
       (r1.y + r1.height <= r2.y) ||
       (r2.y + r2.height <= r1.y))
      return FALSE;

   if(sect)
   {
       sect->x = max(r1.x, r2.x);
       sect->y = max(r1.y, r2.y);
       sect->width = min(r1.x + r1.width,
			 r2.x + r2.width) - max(r1.x, r2.x);
       sect->height = min(r1.y + r1.height,
			  r2.y + r2.height) - max(r1.y, r2.y);

       // offset of intersection within r1
       offset->x = max(r2.x - r1.x, 0);
       offset->y = max(r2.y - r1.y, 0);
   }
   return TRUE;
}

static inline
short gcb_index_from_coord(GcbBoard *board, short x, short y, short grid)
{
   return y * board->grids[grid].n_squares.x + x;
}

static inline
void gcb_coord_from_index(GcbBoard *board, short *x, short *y, short grid, short index)
{
   short cx, cy;

   cy = index / board->grids[grid].n_squares.x;
   cx = index % board->grids[grid].n_squares.x;

   *x = cx; *y = cy;
}

static inline
GcbPiece *gcb_get_piece_in_grid(GcbBoard *board, short x, short y, short grid)
{
   return board->grids[grid].
     squares[gcb_index_from_coord(board, x, y, grid)];
}

static inline
void gcb_set_piece_in_grid(GcbBoard *board, short x, short y, short grid, GcbPiece *piece)
{
   board->grids[grid].
     squares[gcb_index_from_coord(board, x, y, grid)] = piece;
}

GCB_SCOPE
void gcb_calculate_offset_in_square(GcbBoard *board, short *pix_x, short *pix_y,
				    short xpos, short ypos, short grid,
				    short offset_x, short offset_y,
				    short w, short h, short gravity)
{
   short x, y;
   
   // calculate y
   if(gravity & GCB_GRAVITY_NORTH)
   {
       y = board->grids[grid].bbox.y +
	 board->grids[grid].square_size.y * ypos +
	 board->grids[grid].border.y;
   }
   else if(gravity & GCB_GRAVITY_SOUTH)
   {
       // first calculate offset of bottom border
       y = board->grids[grid].bbox.y +
	 board->grids[grid].square_size.y * (ypos + 1);

       y -= h; // substract height
   }
   else // center gravity
   {
       y = board->grids[grid].bbox.y +
	 board->grids[grid].square_size.y * (ypos) +
	 (board->grids[grid].square_size.y +
	  board->grids[grid].border.y - h) / 2;
   }
   y += offset_y;
	
   // calculate x
   if(gravity & GCB_GRAVITY_WEST)
   {
       x = board->grids[grid].bbox.x +
	 board->grids[grid].square_size.x * (xpos) +
	 board->grids[grid].border.x;
   }
   else if(gravity & GCB_GRAVITY_EAST)
   {
       // first calculate offset of right border
       x = board->grids[grid].bbox.x +
	 board->grids[grid].square_size.x * (xpos + 1);

       x -= w; // substract width
   }
   else // center gravity
   {
       x = board->grids[grid].bbox.x +
	 board->grids[grid].square_size.x * (xpos) +
	 (board->grids[grid].square_size.x +
	  board->grids[grid].border.x - w) / 2;
   }
   x += offset_x;

   *pix_x = x;
   *pix_y = y;
}

inline static
void gcb_calculate_piece_type_offset(GcbBoard *board, GcbPieceType *type,
				       short *pix_x, short *pix_y, short xpos,
				       short ypos, short grid)
{
   gcb_calculate_offset_in_square(board,
				  pix_x, pix_y,
				  xpos, ypos, grid,
				  type->offset.x,
				  type->offset.y,
				  type->width,
				  type->height,
				  type->gravity);
}

//! calculate pixel coordinate of the piece at it's current square
static inline
void gcb_calculate_piece_offset(GcbBoard *board, GcbPiece *piece, short *pix_x, short *pix_y)
{
   gcb_calculate_piece_type_offset(board, piece->type, pix_x, pix_y,
				   piece->pos.x, piece->pos.y,
				   piece->grid);
}

static inline
GcbPiece *gcb_new_piece(GcbPieceType *type)
{
   GcbPiece *newPiece;

   newPiece = (GcbPiece *)malloc(sizeof(GcbPiece));

   if(!newPiece)return NO_PIECE;
       
   newPiece->type = type;
   newPiece->animated = FALSE;
   newPiece->blocked = FALSE;
   
   return newPiece;
}

static inline
bool gcb_check_valid_square(GcbBoard *board, short x, short y, short grid)
{
   return (grid < board->n_grids &&
	   x < board->grids[grid].n_squares.x &&
	   y < board->grids[grid].n_squares.y);
}

/* return the square number and the grid of the specified pixel */
static
bool gcb_pixel_to_square(GcbBoard *board, short x, short y, short *sx, short *sy, short *grid)
{
   short gridnum;
   Grid *pGrid;
   short sqx, sqy;

   for(gridnum = 0; gridnum < board->n_grids; gridnum++)
   {
       pGrid = &(board->grids[gridnum]);
       if(point_inside_rect(pGrid->bbox, x, y))
       {   // inside this grid
	   sqx = (x - pGrid->bbox.x) / pGrid->square_size.x;
	   sqy = (y - pGrid->bbox.y) / pGrid->square_size.y;
	   // if the point is on a border
	   if( ((x - pGrid->bbox.x) % pGrid->square_size.x) < pGrid->border.x ||
	       ((y - pGrid->bbox.y) % pGrid->square_size.y) < pGrid->border.y)
	      return 0;

	   *sx = sqx;
	   *sy = sqy;
	   *grid = gridnum;
	   return TRUE;
       }
   }
   return FALSE;
}

static
void gcb_add_damage(GcbBoard *board, short x, short y, short width, short height)
{
   short x2, y2;
   
   if(board->damage.width == 0)
   {
       board->damage.x = x;
       board->damage.y = y;
       board->damage.width = width;
       board->damage.height = height;
       return;
   }

   x2 = board->damage.x + board->damage.width;
   y2 = board->damage.y + board->damage.height;
   
   board->damage.x = min(board->damage.x, x);
   board->damage.y = min(board->damage.y, y);
   board->damage.width = max(x2, x + width) - board->damage.x;
   board->damage.height = max(y2, y + height) - board->damage.y;
}

static inline
void gcb_add_damage_rect(GcbBoard *board, GcbRect damage)
{
   gcb_add_damage(board, damage.x, damage.y, damage.width, damage.height);
}

static
void gcb_draw_piece_at_square_clipped(GcbBoard *board, short x, short y, short grid)
{
   GcbPiece  *piece;
   GcbRect   bbox, sect;
   GcbPoint  offset;

   piece = gcb_get_piece_in_grid(board, x, y, grid);
  
   if(!piece || piece->animated)
      return;

   gcb_calculate_piece_offset(board, piece, &(bbox.x), &(bbox.y));
   bbox.width = piece->type->width;
   bbox.height = piece->type->height;
   if(rect_intersect(bbox, board->damage, &sect, &offset))
      copy_pixmap(board->data,
		  &piece->type->pixmap, &board->canvas,
		  offset.x, offset.y,
		  sect.x, sect.y,
		  sect.width, sect.height);
}

static inline
void gcb_refresh_damaged_area(GcbBoard *board)
{
   if(board->frozen)
      return;
   
   if(board->damage.width)
      gcb_refresh_area_cb(board->data, board->damage.x, board->damage.y,
			  board->damage.width, board->damage.height);
   board->damage.width = 0;
}

static
void gcb_redraw_damaged_area(GcbBoard *board)
{
   GcbOverlay     *overlay;
   GcbPiece       *piece;
   AnimatedPiece *anim;
   GcbRect        sect;
   GcbPoint       offset, start, end;
   short          x, y;
   short          gridnum;
   Grid           *pGrid;
  
   if(!board->damage.width)
      return;

   if(board->frozen)
      return;

   //draw the background
   if(board->has_background_pixmap)
      copy_drawable(board->data, &board->background, &board->canvas,
		    board->damage.x, board->damage.y,
		    board->damage.x, board->damage.y,
		    board->damage.width, board->damage.height);
   else
      gcb_draw_background_cb(board->data,
			     board->damage.x, board->damage.y,
			     board->damage.width, board->damage.height);

   //draw the background overlay pixmaps
   for(overlay = board->bg_overlay_first; overlay; overlay = overlay->next)
   {
       if(rect_intersect(overlay->bbox, board->damage, &sect, &offset))
	  copy_pixmap(board->data,
		      &overlay->pixmap, &board->canvas,
		      offset.x, offset.y,
		      sect.x, sect.y,
		      sect.width, sect.height);
   }

   //draw the pieces
   for(gridnum = 0; gridnum < board->n_grids; gridnum++)
   {
       pGrid = &(board->grids[gridnum]);

       if(rect_intersect(pGrid->bbox, board->damage, &sect, &offset))
       {
	   start.x = (sect.x - pGrid->bbox.x) / pGrid->square_size.x;
	   start.y = (sect.y - pGrid->bbox.y) / pGrid->square_size.y;
	   end.x = (sect.x + sect.width - 1 - pGrid->bbox.x) / pGrid->square_size.x;
	   end.y = (sect.y + sect.height - 1 - pGrid->bbox.y) / pGrid->square_size.y;

	   //draw upper row
	   y = start.y;
	   for(x = start.x; x <= end.x; x++)
	      gcb_draw_piece_at_square_clipped(board, x, y, gridnum);

	   //draw middle rows
	   for(++y; y < end.y; y++)
	   {
	       x = start.x;
	       gcb_draw_piece_at_square_clipped(board, x, y, gridnum);
	       for(x++; x < end.x; x++)
	       {
		   piece = gcb_get_piece_in_grid(board, x, y, gridnum);
		   if(piece && !piece->animated)
		   {
		       gcb_calculate_piece_offset(board, piece, &(offset.x), &(offset.y));
		       copy_pixmap(board->data,
				   &piece->type->pixmap, &board->canvas,
				   0, 0, offset.x, offset.y,
				   piece->type->width, piece->type->height);
		   }
	       }
	     
	       if(x == end.x)
		  gcb_draw_piece_at_square_clipped(board, x, y, gridnum);
	   }

	   //draw bottom row
	   if(y == end.y)
	      for(x = start.x; x <= end.x; x++)
		 gcb_draw_piece_at_square_clipped(board, x, y, gridnum);
       }
   }
	  
   //draw the piece overlay pixmaps
   for(overlay = board->piece_overlay_first; overlay; overlay = overlay->next)
   {
       if(rect_intersect(overlay->bbox, board->damage, &sect, &offset))
	  copy_pixmap(board->data,
		      &overlay->pixmap, &board->canvas,
		      offset.x, offset.y,
		      sect.x, sect.y,
		      sect.width, sect.height);
   }

   //draw the moving piece
   anim = &(board->moving_piece.anim);
   if(anim->animating &&
      rect_intersect(anim->bbox, board->damage,
		     &sect, &offset))
   {
       //save the background
       copy_drawable(board->data,
		     &board->canvas, &anim->saved_bg,
		     sect.x, sect.y,
		     offset.x, offset.y,
		     sect.width, sect.height);
     
       if(!anim->hidden)
	  //draw the pixmap
	  copy_pixmap(board->data,
		      &anim->piece->type->pixmap,
		      &board->canvas, offset.x, offset.y,
		      sect.x, sect.y,
		      sect.width, sect.height);
   }

   //draw the dragged piece
   anim = &(board->dragged_piece.anim);
   if(anim->animating &&
      rect_intersect(anim->bbox, board->damage,
		     &sect, &offset))
   {
       //save the background
       copy_drawable(board->data,
		     &board->canvas, &anim->saved_bg,
		     sect.x, sect.y,
		     offset.x, offset.y,
		     sect.width, sect.height);
     
       if(!anim->hidden)
	  //draw the pixmap
	  copy_pixmap(board->data,
		      &anim->piece->type->pixmap,
		      &board->canvas, offset.x, offset.y,
		      sect.x, sect.y,
		      sect.width, sect.height);
   }
  
   gcb_refresh_damaged_area(board);
}


static inline
void gcb_refresh_area(GcbBoard *board, GcbRect area)
{
   if(board->damage.width)
      gcb_redraw_damaged_area(board);

   gcb_add_damage_rect(board, area);
   gcb_refresh_damaged_area(board);
}

GCB_SCOPE inline
void gcb_redraw_area(GcbBoard *board, GcbRect area)
{
   gcb_add_damage_rect(board, area);
   gcb_redraw_damaged_area(board);
}

//! Create a new board.
/*!
  \param grids the number of grids
  \param canvas the offscreen canvas where all drawing will go to
  \param data A custom data structure that will be used for callbacks
  to the toolkit specific version (i.e. to hold a pointer to a class)
  \return the new board structure
*/
GCB_SCOPE GcbBoard *
gcb_new_board(short grids, void *data)
{
   GcbBoard *newboard;
   int size;

   size = sizeof(GcbBoard) + (grids - 1) * sizeof(Grid);
   newboard = (GcbBoard *)malloc(size);
   memset(newboard, 0, size);

   newboard->frozen = TRUE;
   newboard->enable_highlight = TRUE;
   newboard->enable_dragging = TRUE;
   newboard->enable_flashing = TRUE;
   newboard->enable_animated_move = TRUE;
   newboard->enable_clicking_move = TRUE;
   
   newboard->anim_speed = 10;
   newboard->flash_speed = 120;
   newboard->flash_count = 3;

   newboard->mid_square.x = -1;

   if(data)
      newboard->data = data;
   else
      newboard->data = newboard;
  
   newboard->n_grids = grids;

   return newboard;
}

static
void free_overlay_list(GcbOverlay *list)
{
   GcbOverlay *tmpOver = list;
  
   while(tmpOver)
   {
       list = list->next;
       pixmap_unref(&tmpOver->pixmap);
       free(tmpOver);
       tmpOver = list;
   }
}

GCB_SCOPE 
void gcb_delete_board(GcbBoard *board)
{
   int g, i;
   int size;
   GcbPiece *piece;
   Grid *pGrid;

   gcb_freeze(board);
   free_overlay_list(board->bg_overlay_first);
   free_overlay_list(board->piece_overlay_first);

   for(g = 0; g < board->n_grids; g++) {
       pGrid = &(board->grids[g]);
       size = pGrid->n_squares.x * pGrid->n_squares.y;
       
       for(i = 0; i < size; i++) {
	   piece = pGrid->squares[i];
	   if(piece)
	      free(piece);
       }
       
       free(pGrid->squares);
       free(pGrid->hlight_types);
       if(pGrid->hlight_overlays)
	  free(pGrid->hlight_overlays);
       if(pGrid->hlight_pix)
	  free(pGrid->hlight_pix);
   }

   if(board->promote_piece)
      free(board->promote_piece);

   if(board->previous_piece)
      free(board->previous_piece);

   free(board);
}

//! initialize the grid
/*!
  This function must be called after creating the board.
  There has to be at least one grid.
  \param board the current board
  \param grid  the number of the grid to be initialized
  \param x     the x coordinate of the topleft corner
  \param y     the y coordinate of the topleft corner
  \param columns the number of squares horizontaly
  \param rows  the number of squares verticaly
  \param sw    the width of a square (including one border)
  \param sh    the height of a square (including one border)
  \param borderX the width of the border between squares
  \param borderY the height of the border between squares
*/
static
void gcb_set_grid(GcbBoard *board, short grid, short x, short y, short columns, short rows,
		  short sw, short sh, short borderX, short borderY)
{
   Grid *pGrid;
   int size;

   if(grid >= board->n_grids)return;
   pGrid = &(board->grids[grid]);

   pGrid->bbox.x = x;
   pGrid->bbox.y = y;
   pGrid->bbox.width = columns * sw;
   pGrid->bbox.height = rows * sh;
   pGrid->square_size.x = sw;
   pGrid->square_size.y = sh;
   pGrid->n_squares.x = columns;
   pGrid->n_squares.y = rows;
   pGrid->border.x = borderX;
   pGrid->border.y = borderY;
   
   size = sizeof(GcbPiece*) * rows * columns;
   pGrid->squares = (GcbPiece **)malloc(size);
   memset(pGrid->squares, 0, size);

   size = rows * columns * sizeof(int8);
   pGrid->hlight_types = (int8 *)malloc(size);
   memset(pGrid->hlight_types, 0, size);
}

GCB_SCOPE 
void gcb_resize_grid(GcbBoard *board, short grid, short x, short y,
		     short sw, short sh, short borderX, short borderY)
{
   Grid *pGrid;
   
   if(!board->frozen)
      RUNTIME_ERROR("board should be frozen when resizing the grid");

   if(grid >= board->n_grids || grid < 0)
      RUNTIME_ERROR("when resizing the grid: grid out of range");
      
   pGrid = &(board->grids[grid]);

   pGrid->bbox.x = x;
   pGrid->bbox.y = y;
   pGrid->bbox.width = pGrid->n_squares.x * sw;
   pGrid->bbox.height = pGrid->n_squares.y * sh;
   pGrid->square_size.x = sw;
   pGrid->square_size.y = sh;
   pGrid->border.x = borderX;
   pGrid->border.y = borderY;
}

static inline
bool gcb_grid_has_highlight_pixmap(GcbBoard *board, short grid, short type)
{
   return board->grids[grid].hlight_pix_used & (1 << (type - 1));
}

GCB_SCOPE
void gcb_set_grid_highlight_pixmap(GcbBoard *board, short grid, GcbPixmap pmap, bool set,
				   short x, short y, short w, short h, short type)
{
   Grid *pGrid;
   int size;
   
   if(grid >= board->n_grids && grid < 0)
      RUNTIME_ERROR("when setting grid highlight_pixmap: grid out of range");

   pGrid = &board->grids[grid];

   if(gcb_grid_has_highlight_pixmap(board, grid, type))
      pixmap_unref(&pGrid->hlight_pix[type].pmap);
   
   type--;
   if(!set)
      // unset bit
      pGrid->hlight_pix_used &= ~(1 << type);
   else
   {
       if(pGrid->hlight_pix == NULL)
       {
	   pGrid->hlight_pix = (HighlightPixmap *)malloc(sizeof(HighlightPixmap) * HIGHLIGHT_MAX);

	   size = sizeof(GcbOverlay *) * pGrid->n_squares.x * pGrid->n_squares.y;
	   pGrid->hlight_overlays = (GcbOverlay **)malloc(size);
	   memset(pGrid->hlight_overlays, 0, size);
       }
       // set bit
       pGrid->hlight_pix_used |= (1 << type);
       
       pGrid->hlight_pix[type].pmap = pmap;
       pixmap_ref(&pmap);
       pGrid->hlight_pix[type].pos.x = x;
       pGrid->hlight_pix[type].pos.y = y;
       pGrid->hlight_pix[type].pos.width = w;
       pGrid->hlight_pix[type].pos.height = h;
   }
}

//!replace the canvas (to resize)
/*! We can only replace if the board
  is frozen.
*/

GCB_SCOPE 
void gcb_set_canvas(GcbBoard *board, GcbDrawable canvas, short width, short height)
{
   if(!board->frozen)
      RUNTIME_ERROR("can only set canvas on frozen board");

   board->canvas = canvas;
   gcb_add_damage(board, 0, 0, width, height);
}

GCB_SCOPE inline
void gcb_set_background(GcbBoard *board, GcbDrawable pixmap)
{
   board->has_background_pixmap = TRUE;
   board->background = pixmap;
}

GCB_SCOPE inline
GcbDrawable *gcb_get_background(GcbBoard *board)
{
   if(board->has_background_pixmap)
      return &board->background;
   else
      return NULL;
}

GCB_SCOPE inline
void gcb_unset_background(GcbBoard *board)
{
   board->has_background_pixmap = FALSE;
}   
//! disable all drawing to the screen
/*! When frozen no drawing operations will occur.
  It is safe to change any structures (for example
  to resize the board) as long as their integrity
  is guarded, and the board is redrawn after.
*/

static inline
bool gcb_get_highlight(GcbBoard *board, short x, short y, short grid)
{
   return (board->grids[grid].
	   hlight_types[gcb_index_from_coord(board, x, y, grid)]);
}

static
void gcb_highlight_square(GcbBoard *board, short x, short y, short grid, short type)
{
   short old;
   short index;
   HighlightPixmap *hp;
   Grid *pGrid;
   GcbPoint pos;
   
   old = gcb_get_highlight(board, x, y, grid);

   if(type == old)
      return;
   
   if(type)
      board->highlighting = TRUE;

   pGrid = &board->grids[grid];
   index = gcb_index_from_coord(board, x, y, grid);
   
   pGrid->hlight_types[index] = type;

   if(pGrid->hlight_overlays && pGrid->hlight_overlays[index])
   {
       //remove old highlight
       gcb_remove_overlay(board, pGrid->hlight_overlays[index]);
       pGrid->hlight_overlays[index] = NULL;
       old = HIGHLIGHT_OFF;
   }
   
   if(type && gcb_grid_has_highlight_pixmap(board, grid, type))
   {
       if(old)
	  gcb_highlight_square_cb(board->data, x, y, grid, HIGHLIGHT_OFF);

       hp = &pGrid->hlight_pix[type - 1];
       pos.x = pGrid->bbox.x + pGrid->square_size.x * x + hp->pos.x;
       pos.y = pGrid->bbox.y + pGrid->square_size.y * y + hp->pos.y;
       pGrid->hlight_overlays[index]=
	 gcb_add_overlay(board, hp->pmap, pos.x, pos.y, HIGHLIGHT_LAYER,
			 hp->pos.width, hp->pos.height, TRUE);
   }
   else if(type != old)
      gcb_highlight_square_cb(board->data, x, y, grid, type);
}

static
void gcb_unhighlight_all(GcbBoard *board)
{
   short x, y, grid;
   short size, i;
   Grid *pGrid;
  
   if(!board->highlighting)
      return;
  
   board->highlighting = FALSE;
  
   for(grid = 0; grid < board->n_grids; grid++)
   {
       pGrid = &(board->grids[grid]);
       size = pGrid->n_squares.x * pGrid->n_squares.y;
     
       for(i = 0; i < size; i++)
	  if(pGrid->hlight_types[i])
	  {
	      gcb_coord_from_index(board, &x, &y, grid, i);
	      gcb_highlight_square(board, x, y, grid, HIGHLIGHT_OFF);
	  }
   }
}

GCB_SCOPE
void gcb_end_all_animation(GcbBoard *board)
{
   if(board->animating)
   {
       board->frozen++;
       
       if(board->moving)
		  gcb_stop_moving(board);
       else if(board->flashing)
		  gcb_stop_flashing(board);
       else
       {
		   stop_timer(board->data, &board->timer);
		   gcb_anim_queue_call_next(board);
       }
       
       if(--board->frozen == 0)
		  gcb_redraw_damaged_area(board);
   }
}

GCB_SCOPE 
void gcb_freeze(GcbBoard *board)
{
   board->frozen++;
   if(board->frozen > 1)
      return;
  
   if(board->dragging)
      gcb_cancel_dragging(board);

   gcb_end_all_animation(board);
}

//! enable drawing to the screen
GCB_SCOPE 
void gcb_thaw(GcbBoard *board)
{
   if(!board->frozen)
      return;

   board->frozen--;
   if(! board->frozen)
      gcb_redraw_damaged_area(board);
}


//! Add an overlay pixmap.
/*! The pixmap will be inserted
  in the list so that the list is sorted according to
  the layer.  Layers under 100 come in the bg_overlay
  list, the others in the piece_overlay list.
*/
GCB_SCOPE 
GcbOverlay *gcb_add_overlay(GcbBoard *board, GcbPixmap pmap, short x, short y,
			    short layer, short width, short height, bool now)
{
   GcbOverlay *overlay;

   overlay = (GcbOverlay *)malloc(sizeof(GcbOverlay));
   overlay->pixmap = pmap;
   overlay->layer = layer;
   overlay->bbox.x = x;
   overlay->bbox.y = y;
   overlay->bbox.width = width;
   overlay->bbox.height = height;
   overlay->previous = NULL;
   overlay->immediate = now;

   pixmap_ref(&pmap);
  
   if(board->animating && ! now)
      gcb_anim_queue_add(board, ANIM_ADD_OVERLAY, overlay);
   else
      gcb_do_add_overlay(board, overlay);
   
   return overlay;
}

static
void gcb_do_add_overlay(GcbBoard *board, GcbOverlay *newnode)
{
   GcbOverlay *node, **list;

   if(newnode->layer < PIECE_LAYER)
      list = &(board->bg_overlay_first);
   else
      list = &(board->piece_overlay_first);

   node = *list;

   if(node == NULL)
   {   // empty list
       newnode->previous = NULL;
       newnode->next = NULL;
       *list = newnode;
       goto update;
   }

   while(node->layer < newnode->layer)
   {
       if(node->next)
		  node = node->next;
       else
       {   // insert at the end
		   node->next = newnode;
		   newnode->previous = node;
		   newnode->next = NULL;
		   goto update;
       }
   }

   // insert before node
   if(!node->previous)
      *list = newnode;   /* first node */
   else
      node->previous->next = newnode;
   
   newnode->previous = node->previous;

   node->previous = newnode;
   newnode->next = node;

 update:
   /* update the board */
   gcb_redraw_area(board, newnode->bbox);
   if(! newnode->immediate)
      gcb_anim_queue_call_next(board);
}  

GCB_SCOPE 
void gcb_remove_overlay(GcbBoard *board, GcbOverlay *overlay)
{
   if(board->animating && ! overlay->immediate)
      gcb_anim_queue_add(board, ANIM_REMOVE_OVERLAY, overlay);
   else
      gcb_do_remove_overlay(board, overlay);
}

//!remove an overlay pixmap
static
void gcb_do_remove_overlay(GcbBoard *board, GcbOverlay *overlay)
{
   bool now;

   if(!overlay->previous)
   { /* first node */
       if(overlay->layer < PIECE_LAYER)
		  board->bg_overlay_first = overlay->next;
       else
		  board->piece_overlay_first = overlay->next;
   }
   else
      overlay->previous->next = overlay->next;

   if(overlay->next)
      overlay->next->previous = overlay->previous;

   /* update the board */

   gcb_redraw_area(board, overlay->bbox);
   now = overlay->immediate;

   pixmap_unref(&overlay->pixmap);
   free(overlay);
   if(! now)
      gcb_anim_queue_call_next(board);
}

GCB_SCOPE inline
void gcb_enable_highlight(GcbBoard *board, bool enable)
{
   board->enable_highlight = enable;
}

GCB_SCOPE inline
void gcb_enable_clicking_move(GcbBoard *board, bool enable)
{
   board->enable_clicking_move = enable;
}

GCB_SCOPE inline
void gcb_enable_flashing(GcbBoard *board, bool enable)
{
   board->enable_flashing = enable;
}

GCB_SCOPE inline
void gcb_enable_dragging(GcbBoard *board, bool enable)
{
   board->enable_dragging = enable;
}

GCB_SCOPE inline
void gcb_enable_animated_move(GcbBoard *board, bool enable)
{
   board->enable_animated_move = enable;
}

GCB_SCOPE inline
void gcb_set_anim_speed(GcbBoard *board, short speed)
{
   board->anim_speed = speed;
}

GCB_SCOPE inline
void gcb_set_flash_speed(GcbBoard *board, short speed)
{
   board->flash_speed = speed;
}

GCB_SCOPE inline
void gcb_set_flash_count(GcbBoard *board, short count)
{
   board->flash_count = count;
}

/* begin animating the piece
   note: the caller needs to call update after this function,
   otherwise the screen will be messed up.
*/

static
void gcb_anim_piece_begin(GcbBoard *board, AnimatedPiece *apiece)
{
   short width, height;

   width = apiece->bbox.width;
   height = apiece->bbox.height;

   create_drawable(board->data, &apiece->saved_bg, width, height);
   apiece->animating = TRUE;
   apiece->hidden = FALSE;
   apiece->piece->animated = TRUE;
}

static
void gcb_update_dragged_piece(GcbBoard *board)
{
   GcbRect sect;
   GcbPoint offset;
   AnimatedPiece *dpiece = &board->dragged_piece.anim;
  
   if(rect_intersect(dpiece->bbox, board->moving_piece.anim.bbox, &sect, &offset))
   {
       // save background from upper piece
       copy_drawable(board->data,
		     &board->canvas, &dpiece->saved_bg,
		     sect.x, sect.y,
		     offset.x, offset.y, sect.width,
		     sect.height);
       // redraw upper piece
       if(!dpiece->hidden)
	  copy_pixmap(board->data,
		      &dpiece->piece->type->pixmap, &board->canvas,
		      offset.x, offset.y,
		      sect.x, sect.y,
		      sect.width, sect.height);
   }
}

static
void gcb_anim_piece_clear(GcbBoard *board, AnimatedPiece *apiece, bool moving)
{
   /* erase piece
      it shouldn't have moved
   */
   copy_drawable(board->data,
		 &apiece->saved_bg, &board->canvas,
		 0, 0,
		 apiece->bbox.x, apiece->bbox.y,
		 apiece->bbox.width, apiece->bbox.height);
   apiece->hidden = TRUE;

   if(moving && board->dragging)
      gcb_update_dragged_piece(board);
}

static
void gcb_anim_piece_draw(GcbBoard *board, AnimatedPiece *apiece, bool moving)
{
   GcbRect sect;
   GcbPoint offset;
   
   if(moving && board->dragging && ! board->dragged_piece.anim.hidden &&
      rect_intersect(board->dragged_piece.anim.bbox, apiece->bbox, &sect, &offset))
   {
       // copy background from dragged piece
       copy_drawable(board->data,
		     &board->dragged_piece.anim.saved_bg, &board->canvas,
		     offset.x, offset.y,
		     sect.x, sect.y,
		     sect.width, sect.height);
   }
   // save background
   copy_drawable(board->data,
		 &board->canvas, &apiece->saved_bg,
		 apiece->bbox.x, apiece->bbox.y,
		 0, 0,
		 apiece->bbox.width, apiece->bbox.height);
  
   //draw piece
   copy_pixmap(board->data,
	       &apiece->piece->type->pixmap, &board->canvas,
	       0, 0,
	       apiece->bbox.x, apiece->bbox.y,
	       apiece->bbox.width, apiece->bbox.height);

   apiece->hidden = FALSE;

   if(moving && board->dragging)
      gcb_update_dragged_piece(board);
}

/* end animating the piece
   note: this function doesn't update the board, so that is
   the responsability from the caller
*/
static inline
void gcb_anim_piece_end(GcbBoard *board, AnimatedPiece *anim)
{
   destroy_drawable(board->data, &anim->saved_bg);
   anim->animating = FALSE;
   anim->hidden = TRUE;
}

static
void gcb_reset_if_new_move(GcbBoard *board)
{
   if(! board->active_side_changed)
      return;

   board->active_side_changed = FALSE;
   gcb_end_all_animation(board);
   gcb_unhighlight_all(board);
}

static
void gcb_do_set_promote_piece(GcbBoard *board, GcbPiece *piece)
{
   if(board->promote_piece)
      free(board->promote_piece);

   board->promote_piece = piece;
   gcb_anim_queue_call_next(board);
}

GCB_SCOPE void
gcb_set_promote_piece(GcbBoard *board, GcbPieceType *type)
{
   GcbPiece *new = (type ? gcb_new_piece(type) : NULL);

   if(board->animating)
      gcb_anim_queue_add(board, ANIM_SET_PROMOTE_PIECE, new);
   else
      gcb_do_set_promote_piece(board, new);
}

static
void gcb_do_set_previous_piece(GcbBoard *board, GcbPiece *piece)
{
   if(board->previous_piece)
      free(board->previous_piece);

   board->previous_piece = piece;
   gcb_anim_queue_call_next(board);
}

GCB_SCOPE void
gcb_set_previous_piece(GcbBoard *board, GcbPieceType *type)
{
   GcbPiece *new = (type ? gcb_new_piece(type) : NULL);

   if(board->animating)
      gcb_anim_queue_add(board, ANIM_SET_PREVIOUS_PIECE, new);
   else
      gcb_do_set_previous_piece(board, new);
}

GCB_SCOPE void
gcb_set_mid_square(GcbBoard *board, short x, short y, short grid)
{
   board->mid_square.x = x;
   board->mid_square.y = y;
   board->mid_square.grid = grid;
}

//!move the piece
GCB_SCOPE 
void gcb_move_piece(GcbBoard *board,
		    short from_x, short from_y, short from_grid,
		    short to_x, short to_y, short to_grid,
		    bool silent)
{
   short mid_x, mid_y, mid_grid;
   
   mid_x = board->mid_square.x;
   mid_y = board->mid_square.y;
   mid_grid = board->mid_square.grid;

   if(board->mid_square.x != -1)
      board->mid_square.x = -1;

   gcb_reset_if_new_move(board);   
   if(board->animating)
   {
       /* already animating.  Add this function to the queue, so that it
	  can be called later */

       gcb_anim_queue_add(board, ANIM_MOVE_PIECE, NULL,
			  from_x, from_y, from_grid,
			  mid_x, mid_y, mid_grid,
			  to_x, to_y, to_grid,
			  silent);
   }
   else
   {
       GcbPiece *piece = gcb_get_piece_in_grid(board, from_x, from_y, from_grid);
       if(!piece)
	  RUNTIME_ERROR("No piece on square <%i, %i, %i>.", from_x, from_y, from_grid);

       if(board->enable_animated_move && ! board->frozen)
	  gcb_begin_move_piece(board, piece,
			       mid_x, mid_y, mid_grid,
			       to_x, to_y, to_grid,
			       silent);
       else
	  gcb_do_set_piece(board, piece, to_x, to_y,
			   to_grid, silent, TRUE);
   }
}

//!Calculate the frames in the animation
/*
  The algorithm I, ehm... borrowed from Xboard.  It generates a slow
  out slow in effect by doubling the distance between
  the frames, and halving them again.  In this way the
  attention gets drawn on where the piece comes from,
  and where it goes to.
*/

static
void gcb_calculate_frames(short from_x, short from_y, short mid_x, short mid_y, short to_x,
				 short to_y, GcbPoint *frames, short n_frames)
{
   short factor, fract, i;

   // n_frames should be even
   factor = n_frames / 2 - 1;
   fract = factor;

   for(i = 0; i <= factor; i++)
   {
       frames[i].x = from_x + ((mid_x - from_x) >> fract);
       //         == from_x + ((mid_x - from_x) / (2 ^ fract));
       frames[i].y = from_y + ((mid_y - from_y) >> fract);
       fract--;
   }

   fract = 1;
   for(; i < n_frames - 1; i++)
   {
       frames[i].x = to_x - ((to_x - mid_x) >> fract);
       frames[i].y = to_y - ((to_y - mid_y) >> fract);
       fract++;
   }

   frames[i].x = to_x;
   frames[i].y = to_y;
}

/* begin moving the piece in an animated way.
 */
static 
void gcb_begin_move_piece(GcbBoard *board, GcbPiece *piece,
			  short mid_x, short mid_y, short mid_grid,
			  short to_x, short to_y, short to_grid,
			  bool silent)
{
   short width, height;
   short from_x, from_y;
   MovingPiece *mp;

   board->moving = TRUE;

   width = piece->type->width;
   height = piece->type->height;

   mp = &(board->moving_piece);

   mp->anim.piece = piece;
   mp->end_square.x = to_x;
   mp->end_square.y = to_y;
   mp->end_grid = to_grid;
  
   // use less frames with short moves
   if(piece->grid == to_grid && mid_x == -1 &&
      (abs(to_x - from_x) + abs(to_y - from_y) <= 2))
      mp->n_frames = MAX_FRAMES - 2;
   else
      mp->n_frames = MAX_FRAMES;

   // calculate pixel positions of the piece
   gcb_calculate_piece_type_offset(board, piece->type, &from_x, &from_y, piece->pos.x,
				   piece->pos.y, piece->grid);
   gcb_calculate_piece_type_offset(board, piece->type, &to_x, &to_y, to_x, to_y, to_grid);
   if(mid_x == -1){
       // no midpoint
       mid_x = (from_x + to_x) / 2;
       mid_y = (from_y + to_y) / 2;
   }
   else
      gcb_calculate_piece_type_offset(board, piece->type, &mid_x, &mid_y, mid_x, mid_y, mid_grid);

   gcb_calculate_frames(from_x, from_y,
			mid_x, mid_y, to_x, to_y,
			mp->frames,
			mp->n_frames);

   /* Set frame to first frame and update board.
      This will also save the background for the
      animated piece.
   */
   mp->curr_frame = 0;
   mp->anim.bbox.x = mp->frames[0].x;
   mp->anim.bbox.y = mp->frames[0].y;
   mp->anim.bbox.width = width;
   mp->anim.bbox.height = height;

   gcb_anim_piece_begin(board, &mp->anim);
   board->silent_move = silent;
   board->animating = TRUE;
  
   if(board->previous_piece)
   {
       GcbRect previous;

       board->previous_piece->grid = piece->grid;
       board->previous_piece->pos = piece->pos;

       previous.width = board->previous_piece->type->width;
       previous.height = board->previous_piece->type->height;
       gcb_calculate_piece_offset(board, board->previous_piece,
				  &previous.x, &previous.y);
       gcb_add_damage_rect(board, previous);

       gcb_set_piece_in_grid(board, piece->pos.x, piece->pos.y, piece->grid, board->previous_piece);
       board->previous_piece = NULL;
   } else
      gcb_set_piece_in_grid(board, piece->pos.x, piece->pos.y, piece->grid, NULL);
   

   gcb_add_damage(board, from_x, from_y, width, height);
   gcb_add_damage_rect(board, mp->anim.bbox);
   gcb_redraw_damaged_area(board);

   set_timer(board->data, &board->timer, board->anim_speed, gcb_move_next_frame, board);
}

static
void gcb_move_next_frame(GcbBoard *board)
{
   MovingPiece *mp;
   GcbRect oldbbox;
  
   mp = &(board->moving_piece);
   mp->curr_frame++;
  
   oldbbox = mp->anim.bbox;
   gcb_anim_piece_clear(board, &mp->anim, TRUE);

   mp->anim.bbox.x = mp->frames[mp->curr_frame].x;
   mp->anim.bbox.y = mp->frames[mp->curr_frame].y;
       
   gcb_anim_piece_draw(board, &mp->anim, TRUE);
			
   gcb_refresh_area(board, oldbbox);
   gcb_refresh_area(board, mp->anim.bbox);
   
   if(mp->curr_frame == mp->n_frames - 1)
   {   // reached destination square
       stop_timer(board->data, &board->timer);
       gcb_anim_piece_end(board, &mp->anim);
       gcb_do_set_piece(board, mp->anim.piece, mp->end_square.x,
			mp->end_square.y,
			mp->end_grid, board->silent_move,
			TRUE);
       board->moving = FALSE;
   }
}

static
void gcb_stop_moving(GcbBoard *board)
{
   // let's cheat this one. We will pretend we are at the last frame
   // and call next frame
   board->moving_piece.curr_frame =
     board->moving_piece.n_frames - 2;

   gcb_move_next_frame(board);
}

/* begin flashing the piece.  This also uses
   the animated_piece struct.
   That shouldn't be a problem, since we cannot
   be flashing and animating at the same time.
*/

static
void gcb_begin_flash_piece(GcbBoard *board, GcbPiece *piece)
{
   MovingPiece *fp;

   if(board->frozen)
   {
       gcb_anim_queue_call_next(board);
       return;
   }

   fp = &(board->moving_piece);

   fp->anim.piece = piece;
   fp->n_frames = board->flash_count * 2;
   fp->curr_frame = 0;
   
   gcb_calculate_piece_offset(board, piece, &(fp->anim.bbox.x), &(fp->anim.bbox.y));
   fp->anim.bbox.width = piece->type->width;
   fp->anim.bbox.height = piece->type->height;
   gcb_anim_piece_begin(board, &fp->anim);
   fp->anim.hidden = TRUE;
  
   gcb_redraw_area(board, fp->anim.bbox);
   set_timer(board->data, &board->timer, board->flash_speed, gcb_do_flash_frame, board);
   board->animating = TRUE;
   board->flashing = TRUE;
}

static
void gcb_do_flash_frame(GcbBoard *board)
{
   MovingPiece *fp;

   fp = &board->moving_piece;

   fp->curr_frame++;

   if(fp->curr_frame % 2)
      gcb_anim_piece_draw(board, &fp->anim, TRUE);
   else
      gcb_anim_piece_clear(board, &fp->anim, TRUE);

   if(fp->curr_frame + 1 == fp->n_frames)
      /* the last frame */
      gcb_stop_flashing(board);
   else
      gcb_refresh_area(board, fp->anim.bbox);
}

static
void gcb_stop_flashing(GcbBoard *board)
{
   stop_timer(board->data, &board->timer);
   gcb_anim_piece_end(board, &(board->moving_piece.anim));
   board->moving_piece.anim.piece->animated = FALSE;
   gcb_redraw_area(board, board->moving_piece.anim.bbox);
   board->flashing = FALSE;
   gcb_anim_queue_call_next(board);
}
   
static
void gcb_start_dragging(GcbBoard *board, short x, short y)
{
   GcbPiece *piece;
   DraggedPiece *dp;
   short width, height;
   short from_x, from_y;

   dp = &board->dragged_piece;
   piece = dp->anim.piece = board->selected_piece;
   
   width = piece->type->width;
   height = piece->type->height;

   dp->delta.x = width / 2;
   dp->delta.y = height / 2;
   dp->anim.bbox.x = x - dp->delta.x;
   dp->anim.bbox.y = y - dp->delta.y;
   dp->anim.bbox.width = width;
   dp->anim.bbox.height = height;

   gcb_calculate_piece_offset(board, dp->anim.piece, &from_x, &from_y);
   gcb_anim_piece_begin(board, &dp->anim);
   gcb_add_damage(board, from_x, from_y, width, height);
   gcb_add_damage_rect(board, dp->anim.bbox);
   gcb_redraw_damaged_area(board);

   board->dragging = TRUE;
}

static
void gcb_drag_piece(GcbBoard *board, short x, short y)
{
   DraggedPiece *dp = &(board->dragged_piece);

   gcb_add_damage_rect(board, dp->anim.bbox);
  
   gcb_anim_piece_clear(board, &dp->anim, FALSE);
   dp->anim.bbox.x = x - dp->delta.x;
   dp->anim.bbox.y = y - dp->delta.y;
   gcb_anim_piece_draw(board, &dp->anim, FALSE);

   gcb_add_damage_rect(board, dp->anim.bbox);
   gcb_refresh_damaged_area(board);
}

static
void gcb_finish_dragging(GcbBoard *board)
{
   GcbPiece *piece;

   gcb_anim_piece_clear(board, &board->dragged_piece.anim, FALSE);
   gcb_add_damage_rect(board, board->dragged_piece.anim.bbox);
   
   piece = board->dragged_piece.anim.piece;
   gcb_anim_piece_end(board, &board->dragged_piece.anim);
   board->selected_piece = NO_PIECE;
  
   piece->animated = FALSE;
   gcb_do_set_piece(board, piece, board->moveinfo->x,
		    board->moveinfo->y, board->moveinfo->grid,
		    TRUE, TRUE);
  
   /* gcb_refresh_damaged_area(board); called by gcb_do_set_piece */
   board->dragging = FALSE;
   board->selected_piece = NO_PIECE;
}

static
void gcb_cancel_dragging(GcbBoard *board)
{
   GcbPiece *piece;
   GcbRect bbox;
  
   piece = board->dragged_piece.anim.piece;
   gcb_anim_piece_end(board, &board->dragged_piece.anim);
   
   gcb_calculate_piece_offset(board, piece, &(bbox.x), &(bbox.y));
   bbox.width = piece->type->width;
   bbox.height = piece->type->height;
   piece->animated = FALSE;
   
   board->dragging = FALSE;
   if(!board->selected_piece_marked)
      board->selected_piece = NO_PIECE;
   
   gcb_redraw_area(board, bbox);
   gcb_redraw_area(board, board->dragged_piece.anim.bbox);
}

GCB_SCOPE 
void gcb_cancel_move(GcbBoard *board)
{
   if(!board->moveinfo || board->moveinfo->finished)
   {
       /* call error */
       return;
   }
   board->moveinfo->finished = TRUE;

   if(board->dragging)
      gcb_cancel_dragging(board);

   if(!board->selected_piece_marked)
      board->selected_piece = NO_PIECE;
}

GCB_SCOPE 
void gcb_finish_move(GcbBoard *board)
{
   if(!board->moveinfo || board->moveinfo->finished)
       return;

   board->moveinfo->finished = TRUE;

   if(board->dragging)
      gcb_finish_dragging(board);
   else
      gcb_move_piece(board,
		     board->selected_piece->pos.x,
		     board->selected_piece->pos.y,
		     board->selected_piece->grid,
		     board->moveinfo->x,
		     board->moveinfo->y, 
		     board->moveinfo->grid,
		     TRUE);

   board->mid_square.x = -1;
   board->selected_piece = NO_PIECE;
   gcb_unhighlight_all(board);
}

static 
void gcb_on_press(GcbBoard *board, short x, short y, short button)
{
   bool retval;
   GcbPiece *piece;
   MoveInfo minfo;

   if(board->frozen)
      return;

   if(!gcb_pixel_to_square(board, x, y, &(minfo.x), &(minfo.y), &(minfo.grid)))
      return;
   minfo.finished = FALSE;

   gcb_press_square_cb(board->data, minfo.x, minfo.y, minfo.grid, button);
  
   if(button != 1)
      return;

   board->mouse1_down = TRUE;
   piece = gcb_get_piece_in_grid(board, minfo.x, minfo.y, minfo.grid);
   
   if(!piece || piece->type->side != board->player_side)
   {  //empty or opponent piece

       if(board->selected_piece && board->enable_clicking_move)
       { //destination square
	
	   if(board->active_side != board->player_side)
	   { //not players turn: here comes code for premove
	       gcb_unhighlight_all(board);
	       board->selected_piece = NO_PIECE;
	   }
	   else
	   {
	       if(board->animating)
	       {
		   gcb_end_all_animation(board);
		   if(!board->selected_piece)
		      return;
	       }

	       board->moveinfo = &(minfo);
	       retval = gcb_drop_piece_cb(board->data,
					  board->selected_piece->pos.x,
					  board->selected_piece->pos.y,
					  board->selected_piece->grid,
					  minfo.x, minfo.y, minfo.grid);
	       if(!minfo.finished)
	       {
		   if(retval)
		      /* gcb_swedish_move */	     
		      gcb_finish_move(board);
		   else
		      gcb_cancel_move(board);
	       }
	       board->moveinfo = NULL;
	       board->active_side_changed = FALSE;
	   }
	   return;
       }
       else // no piece selected
	  gcb_unhighlight_all(board);
   }
   else //own piece
   {
       if(piece == board->selected_piece)
       {
	  gcb_highlight_square(board, minfo.x, minfo.y, minfo.grid, HIGHLIGHT_OFF);
	  board->selected_piece_marked = FALSE;
       }
       else
       {
	   if(piece->blocked) {
	       board->selected_piece = NO_PIECE;
	       gcb_unhighlight_all(board);
	   }
	   else
	   {
	       gcb_unhighlight_all(board);
	       if(board->enable_clicking_move) {
		   gcb_highlight_square(board, minfo.x, minfo.y, minfo.grid, HIGHLIGHT_NORMAL);
		   board->selected_piece_marked = TRUE;
	       }else
		  board->selected_piece_marked = FALSE;
	       board->selected_piece = piece;
	   }
       }
       board->selected_piece_active = TRUE;
       board->start_drag.x = x;
       board->start_drag.y = y;
   }
}

static 
void gcb_on_release(GcbBoard *board, short x, short y, short button)
{
   MoveInfo minfo;
   bool on_square;
   
   if(board->frozen)
      return;
  
   on_square = gcb_pixel_to_square(board, x, y, &(minfo.x), &(minfo.y), &(minfo.grid));
   minfo.finished = FALSE;
   
   if(button != 1)
      return;

   board->mouse1_down = FALSE;
   board->selected_piece_active = FALSE;
   
   if(board->dragging)
   {
       GcbPiece *piece;
       short retval;
  
       if(!on_square)
       {
	   gcb_cancel_dragging(board);
	   return;
       }
       piece = gcb_get_piece_in_grid(board, minfo.x, minfo.y, minfo.grid);
       if(!piece || piece->type->side != board->active_side)
       {
	   //destination square
	   if(board->active_side != board->player_side)
	   {
	       //here comes premove code
	       board->selected_piece = NO_PIECE;
	       gcb_cancel_dragging(board);
	   }
	   else
	   {
	       if(board->animating)
	       {
		  gcb_end_all_animation(board);
		  if(!board->dragging)
		     return;
	       }

	       board->moveinfo = &minfo;
	       retval = gcb_drop_piece_cb(board->data,
					  board->selected_piece->pos.x,
					  board->selected_piece->pos.y,
					  board->selected_piece->grid,
					  minfo.x, minfo.y, minfo.grid);
	       if(!minfo.finished)
	       {
		   if(retval)
		      // gcb_swedish_move
		      gcb_finish_move(board);
		   else
		      gcb_cancel_move(board);
	       }
	       board->moveinfo = NULL;
	       board->active_side_changed = FALSE;
	   }
	   return;
       }
       //else fall through;
   }
  
   if(!board->selected_piece)
      return;
  
   /*   if(!on_square ||
      minfo.x != board->selected_piece->pos.x ||
      minfo.y != board->selected_piece->pos.y ||
      minfo.grid != board->selected_piece->grid)
   {
       //mouse not released in the same square
       board->selected_piece = NO_PIECE;
       gcb_unhighlight_all(board);
   }
   
   else
   {
   */
   if(!board->selected_piece_marked)
      board->selected_piece = NULL;

   if(board->dragging)
      gcb_cancel_dragging(board);
}

void gcb_on_mouse_move(GcbBoard *board, int x, int y)
{
   if(board->frozen)
      return;

   if(!board->mouse1_down)
      return;

   if(board->dragging)
      gcb_drag_piece(board, x, y);
   else
   {  //check if we should start dragging
       if(board->selected_piece &&
	  board->selected_piece_active &&
	  board->enable_dragging &&
	  (abs(x - board->start_drag.x) +
	   abs(y - board->start_drag.y) >= 3))
	  gcb_start_dragging(board, x, y);
   }
}

/* this function actually sets the piece.  It should not be called directly,
   but from gcb_set_piece, or from the queue */
   
static
void gcb_do_set_piece(GcbBoard *board, GcbPiece *piece, short x,
		      short y, short grid, bool silent, bool moving)
{
   GcbPiece *old;
   GcbPoint offset;
   GcbRect from_bbox;
   short from_x, from_y, from_grid;
   
   old = gcb_get_piece_in_grid(board, x, y, grid);
   
   if(piece)
   {
       if(moving)
       {
	   from_x = piece->pos.x;
	   from_y = piece->pos.y;
	   from_grid = piece->grid;

	   if(board->dragging &&
	      board->dragged_piece.anim.piece == old)
	      gcb_cancel_dragging(board);
	   
	   //redraw original square
	   //already done when animating
	   if(!piece->animated)
	   {
	       gcb_calculate_piece_type_offset(board, piece->type, &from_bbox.x, &from_bbox.y,
					       from_x, from_y, from_grid);
	       from_bbox.width = piece->type->width;
	       from_bbox.height = piece->type->height;
	       
	       gcb_add_damage_rect(board, from_bbox);

	       if(board->previous_piece)
	       {
		   from_bbox.width = board->previous_piece->type->width;
		   from_bbox.height = board->previous_piece->type->height;
		   gcb_calculate_piece_type_offset(board, board->previous_piece->type,
						   &from_bbox.x, &from_bbox.y,
						   from_x, from_y, from_grid);
		   gcb_add_damage_rect(board, from_bbox);
	       
		   gcb_set_piece_in_grid(board, from_x, from_y, from_grid, board->previous_piece);
		   board->previous_piece->grid = from_grid;
		   board->previous_piece->pos.x = from_x;
		   board->previous_piece->pos.y = from_y;
		   board->previous_piece = NULL;
	       } else
		  gcb_set_piece_in_grid(board, from_x, from_y, from_grid, NULL);
	   }

	   if(! board->frozen)
	      gcb_highlight_square(board, from_x, from_y, from_grid,
				   (!silent && board->enable_highlight) ?
				   HIGHLIGHT_NORMAL :
				   HIGHLIGHT_OFF);

	   gcb_redraw_damaged_area(board);

	   // do we need to promote?
	   if(board->promote_piece)
	   {
	       free(piece);
	       piece = board->promote_piece;
	       piece->animated = FALSE; 
	       board->promote_piece = NO_PIECE;
	   }
       }

       piece->grid = grid;
       piece->pos.x = x;
       piece->pos.y = y;

       //update new position
       gcb_calculate_piece_offset(board, piece, &offset.x, &offset.y);
       gcb_add_damage(board, offset.x, offset.y,
		      piece->type->width, piece->type->height);

       piece->animated = FALSE;
   }
   
   gcb_set_piece_in_grid(board, x, y, grid, piece);
   
   if(old)
   {
       gcb_calculate_piece_offset(board, old, &offset.x, &offset.y);
       gcb_add_damage(board, offset.x, offset.y,
		      old->type->width, old->type->height);
       if(old == board->selected_piece)
	  board->selected_piece = NO_PIECE;

       free(old);
   }

   if(! board->frozen)
      gcb_highlight_square(board, x, y, grid,
			   (!silent && board->enable_highlight) ?
			   HIGHLIGHT_NORMAL :
			   HIGHLIGHT_OFF);
   
   gcb_redraw_damaged_area(board);

   if(piece && !silent && board->enable_flashing)
      gcb_begin_flash_piece(board, piece);
   else
      gcb_anim_queue_call_next(board);
}

GCB_SCOPE 
void gcb_put_piece(GcbBoard *board, GcbPieceType *type, short x,
			short y, short grid, bool silent)
{
   GcbPiece *piece;
   
   piece = (type ? gcb_new_piece(type) : NULL);
   
   gcb_reset_if_new_move(board);
   if(board->animating)
      /* already animating.  Add this function to the queue, so that it
	 can be called later */
      gcb_anim_queue_add(board, ANIM_SET_PIECE, piece, x, y, grid, silent);
   else
      gcb_do_set_piece(board, piece, x, y, grid, silent, FALSE);
}

GCB_SCOPE inline
GcbPieceType *gcb_get_piece(GcbBoard *board, short x, short y, short grid)
{
   GcbPiece *piece = gcb_get_piece_in_grid(board, x, y, grid);
   return (piece ? piece->type : NULL);
}

static
void gcb_anim_queue_add(GcbBoard *board, short func, void *data, ...)
{
   AnimCall *new_call;
   va_list arglist;

   short i;
   static short anim_params[] = {10, 4, 1, 0, 0, 0, 0};
   short args = anim_params[func];
   
   va_start(arglist, data);

   new_call = (AnimCall *)malloc(sizeof(AnimCall) + sizeof (short) * (args - 1));

   new_call->func = func;
   new_call->data = data;
   for(i = 0; i < args; i++)
      new_call->params[i] = va_arg(arglist, int);
   new_call->next = NULL;
  
   if(board->anim_queue == NULL)
      board->anim_queue = new_call;
   else
      board->anim_queue_end->next = new_call;

   board->anim_queue_end = new_call;
   board->animating = TRUE;

   va_end(arglist);
}

static
void gcb_anim_queue_call_next(GcbBoard *board)
{
   AnimCall *anim;
   GcbPiece *piece;

   if(board->anim_queue == NULL)
   {
       board->animating = FALSE;
       return;
   }

   /* get next animation func from queue*/
   anim = board->anim_queue;
   board->anim_queue = board->anim_queue->next;
   if(board->anim_queue == NULL)
      board->anim_queue_end = NULL;

   switch(anim->func)
   {
     case ANIM_MOVE_PIECE:
	piece = gcb_get_piece_in_grid(board,
				      anim->params[0], anim->params[1], anim->params[2]);
	if(!piece)
	   RUNTIME_ERROR("No piece on square <%i, %i, %i>.",
			 anim->params[0], anim->params[1], anim->params[2]);

	if(board->frozen || !board->enable_animated_move)
	   gcb_do_set_piece(board, piece,
			    anim->params[6], anim->params[7],
			    anim->params[8], anim->params[9], TRUE);
	else
	   gcb_begin_move_piece(board, piece,
				anim->params[3], anim->params[4],
				anim->params[5], anim->params[6],
				anim->params[7], anim->params[8],
				anim->params[9]);
	break;
     case ANIM_SET_PIECE:
	gcb_do_set_piece(board, (GcbPiece *)anim->data,
			 anim->params[0], anim->params[1],
			 anim->params[2], anim->params[3], FALSE);
	break;
     case ANIM_DELAY:
	if(!board->frozen)
	   gcb_do_delay(board, anim->params[0]);
	else
	   gcb_anim_queue_call_next(board);
	break;
     case ANIM_ADD_OVERLAY:
	gcb_do_add_overlay(board, (GcbOverlay *)anim->data);
	break;
     case ANIM_REMOVE_OVERLAY:
	gcb_do_remove_overlay(board, (GcbOverlay *)anim->data);
	break;
     case ANIM_SET_PROMOTE_PIECE:
	gcb_do_set_promote_piece(board, (GcbPiece *)anim->data);
	break;
     case ANIM_SET_PREVIOUS_PIECE:
	gcb_do_set_previous_piece(board, (GcbPiece *)anim->data);
	break;
   }
   free(anim);
   return;
}

GCB_SCOPE
void gcb_delay(GcbBoard *board, short delay)
{
   if(board->animating)
      gcb_anim_queue_add(board, ANIM_DELAY, NULL, delay, 0, 0, 0, 0, 0);
   else
      gcb_do_delay(board, delay);
}

static
void gcb_end_delay(GcbBoard *board)
{
   stop_timer(board->data, &board->timer);
   gcb_anim_queue_call_next(board);
}

static
void gcb_do_delay(GcbBoard *board, short delay)
{
   set_timer(board->data, &board->timer, delay, gcb_end_delay, board);
}

GCB_SCOPE inline
void gcb_block_piece(GcbBoard *board, short x, short y, short grid)
{
   GcbPiece *piece = gcb_get_piece_in_grid(board, x, y, grid);

   if(!piece)
      RUNTIME_ERROR("No piece to block at <%i, %i, %i>.", x, y, grid);

   piece->blocked = TRUE;
}

GCB_SCOPE inline
void gcb_unblock_piece(GcbBoard *board, short x, short y, short grid)
{
   GcbPiece *piece = gcb_get_piece_in_grid(board, x, y, grid);

   if(!piece)
      RUNTIME_ERROR("No piece to unblock at <%i, %i, %i>.", x, y, grid);

   piece->blocked = FALSE;
}

GCB_SCOPE inline
void gcb_set_active_side(GcbBoard *board, short side)
{
   if(board->active_side != side)
   {
       board->active_side = side;
       board->active_side_changed = TRUE;
   }
}

GCB_SCOPE inline
void gcb_set_player_side(GcbBoard *board, short side)
{
   board->player_side = side;
}

GCB_SCOPE
void gcb_block_side(GcbBoard *board, short side)
{
   short iGrid, n_squares, i;

   for(iGrid = 0; iGrid < board->n_grids; iGrid++)
   {
       n_squares = board->grids[iGrid].n_squares.x *
	 board->grids[iGrid].n_squares.y;
      
       for(i = 0; i < n_squares; i++)
	  if(board->grids[iGrid].squares[i] &&
	     board->grids[iGrid].squares[i]->type->side == side)
	     board->grids[iGrid].squares[i]->blocked = FALSE;
   }
}

GCB_SCOPE
void gcb_unblock_side(GcbBoard *board, short side)
{
   short iGrid, n_squares, i;

   for(iGrid = 0; iGrid < board->n_grids; iGrid++)
   {
       n_squares = board->grids[iGrid].n_squares.x *
	 board->grids[iGrid].n_squares.y;
     
       for(i = 0; i < n_squares; i++)
	  if(board->grids[iGrid].squares[i] &&
	     board->grids[iGrid].squares[i]->type->side == side)
	     board->grids[iGrid].squares[i]->blocked = TRUE;
   }
}

// Local Variables:
// mode:c
// c-file-style: "gnu"
// c-file-offsets: ((inclass . 3)(substatement-open . 0)(statement-block-intro . 4)(substatement . 3)(case-label . 2)(block-open . 0)(statement-case-intro . 3)(statement-case-open . 0)(defun-block-intro . 3))
// End:
