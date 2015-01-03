require 'gtk2'
require 'gtkcboard.so'

module Gtk
  class CBoard
    GridInfo = Struct.new(:x, :y, :columns, :rows,
			  :square_w, :square_h,
  			  :border_x, :border_y)
  end
end
