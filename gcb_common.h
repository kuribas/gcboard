/* gcb_common.h  Generic Chess Boardg
   common routines

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


enum
{
   GCB_GRAVITY_CENTER = 0,

   GCB_GRAVITY_NORTH  = 1,
   GCB_GRAVITY_SOUTH  = 2,
   GCB_GRAVITY_EAST   = 4,
   GCB_GRAVITY_WEST   = 8,
   
   GCB_GRAVITY_NW     = (GCB_GRAVITY_NORTH | GCB_GRAVITY_WEST),
   GCB_GRAVITY_NE     = (GCB_GRAVITY_NORTH | GCB_GRAVITY_EAST),
   GCB_GRAVITY_SW     = (GCB_GRAVITY_SOUTH | GCB_GRAVITY_WEST),
   GCB_GRAVITY_SE     = (GCB_GRAVITY_SOUTH | GCB_GRAVITY_EAST)
};

enum
{
   HIGHLIGHT_OFF     = 0,
   HIGHLIGHT_NORMAL,
   HIGHLIGHT_PREMOVE,
   HIGHLIGHT_MAX = HIGHLIGHT_PREMOVE
};

#define MAX_FRAMES 10
#define NO_PIECE (GcbPiece *)NULL

typedef char bool;
typedef char int8;

typedef struct
{
   short x, y;
} GcbPoint;

typedef struct
{
   short x, y;
   unsigned short width, height;
} GcbRect;


/**! the GcbPieceType structure describes properties common to pieces
   of the same kind and side (i.e. black knight).  It also contains
   information about the placement of the piece in the square.
*/

typedef struct
{
   GcbPixmap pixmap;
   short side : 8;

   short gravity : 4;  /* the placing in the squares */
   GcbPoint offset; /* the offset of the pieces in the squares */
   unsigned short width, height;
} GcbPieceType;

//! the GcbPiece structure holds information about a piece being played

typedef struct
{
   GcbPieceType *type;

   short grid : 8;
   GcbPoint pos;

   bool blocked  : 1; /* piece cannot be dragged or clicked */
   bool animated   : 1; /* piece is being animated */
} GcbPiece;

typedef struct gcb_overlay_struct
{
   GcbPixmap pixmap;
   GcbRect bbox;
   short layer;
   bool immediate : 1;
   struct gcb_overlay_struct *next;
   struct gcb_overlay_struct *previous;
}GcbOverlay;

/** the grid is a rectangular grid of squares where the user can click
    or drag pieces.  It is possible to have more than one grid each
    board */

typedef struct
{
   GcbPixmap pmap;
   GcbRect pos;
} HighlightPixmap;

typedef struct
{
   GcbRect bbox;
   GcbPoint square_size;
   GcbPoint n_squares;
   GcbPoint border;
   GcbPiece **squares;
   int8 *hlight_types;
   int8 hlight_pix_used;
   GcbOverlay **hlight_overlays;
   HighlightPixmap *hlight_pix;
}Grid;

typedef struct
{
   GcbPiece *piece;
   GcbDrawable saved_bg;
   GcbRect bbox;
   bool hidden : 1;
   bool animating : 1;
}AnimatedPiece;

typedef struct
{
   AnimatedPiece anim;
   GcbPoint frames[MAX_FRAMES];
   int n_frames : 8;
   int curr_frame : 8;
   GcbPoint end_square;
   short end_grid;
}MovingPiece;

typedef struct
{
   AnimatedPiece anim;
   GcbPoint delta;
}DraggedPiece;

typedef struct anim_call_struct
{
   short func;
   struct anim_call_struct *next;
   void *data;
   short params[1];
} AnimCall;

/* an overlay is a pixmap that can be used to change the appearance of
   the board during the game in an easy way.  The overlays will be
   drawn on top of the board. The layer field makes it possible to
   place elements in different layers, such as above the pieces.
*/

typedef struct
{
   short x, y, grid;
   short finished;
}MoveInfo;

typedef struct
{
   void *data;

   bool animating : 1;
   bool dragging  : 1;
   bool moving    : 1;
   bool flashing  : 1;
   bool highlighting : 1;

   bool enable_highlight : 1;
   bool enable_clicking_move : 1;
   bool enable_flashing  : 1;
   bool enable_dragging  : 1;
   bool enable_animated_move : 1;
   bool enable_premove : 1;
   
   bool has_background_pixmap : 1;
   bool active_side_changed : 1;
   bool silent_move : 1;
   bool mouse1_down : 1;

   bool selected_piece_marked : 1;
   bool selected_piece_active : 1;
   int frozen;
   
   MovingPiece moving_piece;
   DraggedPiece dragged_piece;

   MoveInfo *moveinfo;
   MoveInfo mid_square;
  
   GcbPiece *selected_piece;
   GcbPoint start_drag;
   GcbPiece *promote_piece;
   GcbPiece *previous_piece;

   AnimCall *anim_queue, *anim_queue_end;
   short active_side : 8;
   short player_side : 8;

   GcbRect damage;
   GcbDrawable canvas;
   GcbDrawable background;
   
   GcbTimer timer;
   short anim_speed;
   short flash_speed;
   short flash_count;

   GcbOverlay *bg_overlay_first;
   GcbOverlay *piece_overlay_first;

   short n_grids;
   Grid grids[1];
} GcbBoard;

// Local Variables:
// mode:c
// c-file-style: "gnu"
// c-file-offsets: ((inclass . 3)(substatement-open . 0)(statement-block-intro . 4)(substatement . 3)(case-label . 2)(block-open . 0)(statement-case-intro . 3)(statement-case-open . 0)(defun-block-intro . 3))
// End:
