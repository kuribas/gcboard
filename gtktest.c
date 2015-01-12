#include <gtk/gtk.h>
#include "gtkcboard.h"
#include <ctype.h>
#include <stdio.h>

#include "xpm_images.inc"

GtkCBoardPiece whale_types[8][2];
GdkPixmap *whale_board = NULL;
GdkPixbuf *highlight = NULL;
//GdkBitmap *highlight_mask = NULL;
GdkPixbuf *whale = NULL;
//GdkBitbuf *whale_mask = NULL;

#define BLACK 0
#define WHITE 1

GtkCBoardGridInfo grid_info = 
  { 11, 11, 6, 6, 60, 60, 2, 2};

char **whale_xpms[8][2] =
  { { Blue_whale_sea_B_xpm,     Blue_whale_sea_W_xpm },
    { Dolphin_sea_B_xpm,        Dolphin_sea_W_xpm },
    { Grey_whale_sea_B_xpm,     Grey_whale_sea_W_xpm },
    { Humpback_whale_sea_B_xpm, Humpback_whale_sea_W_xpm },
    { Killer_whale_sea_B_xpm,   Killer_whale_sea_W_xpm },
    { Narwhal_sea_B_xpm,        Narwhal_sea_W_xpm },
    { Porpoise_sea_B_xpm,       Porpoise_sea_W_xpm },
    { White_Whale_sea_B_xpm,    White_Whale_sea_W_xpm } };

gint8 board_setup[] =
  { 0, 5, 6, 7, 2, 3,
    1, 1, 1, 1, 1, 1,
    9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9,
    1, 1, 1, 1, 1, 1,
    3, 2, 7, 6, 5, 0};
    
void setup_whale_types(GdkWindow *window)
{
   gint i, c;
   GtkCBoardPiece *piece;

   for(i = 0; i < 8; i++)
   {
      for(c = 0; c < 2; c++)
      {
	  piece = &whale_types[i][c];
	  piece->type = GTKCBOARD_PIXBUF;
	  piece->pixbuf = gdk_pixbuf_new_from_xpm_data(whale_xpms[i][c]);
	  piece->width = gdk_pixbuf_get_width(piece->pixbuf);
	  piece->height = gdk_pixbuf_get_height(piece->pixbuf);
	  piece->side = c;
	  piece->gravity = (c ? CBOARD_GRAVITY_NORTH : CBOARD_GRAVITY_SOUTH);
	  piece->offset_x = 0;
	  piece->offset_y = 0;
      }
   }
   whale_board = gdk_pixmap_create_from_xpm_d(window, NULL, NULL, Whalebord_xpm);
   highlight = gdk_pixbuf_new_from_xpm_data(highlight_xpm);
   whale = gdk_pixbuf_new_from_xpm_data(whale_xpm);
}

void setup_board(GtkCBoard *board, gpointer data)
{
   int x, y, i, p;
   GtkCBoardPiece *piece;
   setup_whale_types(GTK_WIDGET(board)->window);
   
   gtk_cboard_set_background(board, whale_board);
   gtk_cboard_set_grid_highlight_normal_pixbuf(board, 0, highlight, 0, 0);

   i = 0;
   for (y = 0; y < 6; y++)
      for(x = 0; x < 6; x++)
      {
	 p = board_setup[i];
	 if(p < 8) {
	     piece = &whale_types[p][y < 3 ? WHITE : BLACK];
	     gtk_cboard_put_piece(board, piece, x, y, 0, 0);
	 }
	 i++;
      }
}

gboolean drop_piece (GtkCBoard *board, gint fx, gint fy, gint fgrid, gint x, gint y, gint grid)
{
   gint16 mx, my;
   static gint side = WHITE;
   static GtkCBoardOverlay *overlay = NULL;

   if(y == (side == BLACK ? 0 : 5))
      gtk_cboard_set_promote_piece(board, &whale_types[4][side]);
   
   gtk_cboard_finish_move(board);
   do
   {
       mx = rand() % 6;
       my = rand() % 6;
   } while(mx == x && my == y);
   g_message("moving from %i, %i, %i to %i, %i, %i", x, y, grid, mx, my, 0);
   gtk_cboard_move_piece(board, x, y, grid, mx, my, 0, FALSE);
   if(! overlay)
      overlay = gtk_cboard_add_overlay_pixbuf(board, whale, rand() % 200,
				       rand() % 200, rand() % 200, FALSE);
   else
   {
      gtk_cboard_remove_overlay(board, overlay);
      overlay = NULL;
   }
   gtk_cboard_delay(board, 200);
   do
   {
       fx = rand() % 6;
       fy = rand() % 6;
   }while(mx == fx && my == fy);
   g_message("moving from %i, %i, %i to %i, %i, %i", mx, my, 0, fx, fy, 0);
   //gtk_cboard_move_piece(board, mx, my, 0, fx, fy, 0, FALSE);
   side = ! side;
   gtk_cboard_set_active_side(board, side);
   gtk_cboard_set_player_side(board, side);
   //gtk_cboard_set_active_side(board, side);
   return TRUE;   
}

int main(int argc, char **argv)
{
   GtkWindow *mainWin;
   GtkCBoard *board;
   GtkVBox *vbox;
   
   gtk_init(&argc, &argv);

   mainWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   board = gtk_cboard_new(383, 384, 1, &grid_info);
   vbox = gtk_vbox_new(0, FALSE);
   gtk_cboard_enable_animated_move(board, FALSE);

   gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(board), 0, 0, 0);
   gtk_container_add(GTK_CONTAINER(mainWin), GTK_WIDGET(vbox));
   g_signal_connect(GTK_OBJECT(board), "init_board",
		    GTK_SIGNAL_FUNC(setup_board), NULL);
   g_signal_connect(GTK_OBJECT(board), "drop_piece",
		    GTK_SIGNAL_FUNC(drop_piece), NULL);
    /*gtk_signal_connect(GTK_OBJECT(entry), "highlight_square", NULL, NULL,
      GTK_SIGNAL_FUNC(highlight_square_handler), NULL, FALSE, 1); */
   gtk_widget_show_all(GTK_WIDGET(mainWin));
   // gtk_cboard_thaw(board);
    
   gtk_main();

   gtk_exit(0);
}
    
