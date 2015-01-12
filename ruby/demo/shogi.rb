#!/usr/bin/env ruby

require 'gtk2'
require 'gtkcboard'

PIECE_DIR = "./pieces/"

BLACK = 0
WHITE = 1

MAINGRID = 2

PieceInfo = [
  # name short-name promotes?
  ["Lance",   "L",  true],
  ["Knight",  "N",  true],
  ["Silver",  "S",  true],
  ["Gold",    "G",  false],
  ["Bishop",  "B",  true],
  ["Rook",    "R",  true],
  ["Pawn",    "P",  true],
  ["King",    "K",  false]]

$ShogiPieces = Array.new(8)

InitSetup = [
  [0,   1,   2,   3,   7,   3,   2,   1,   0  ],
  [nil, 5,   nil, nil, nil, nil, nil, 4,   nil],
  [6,   6,   6,   6,   6,   6,   6,   6,   6  ],
  [nil, nil, nil, nil, nil, nil, nil, nil, nil],
  [nil, nil, nil, nil, nil, nil, nil, nil, nil]]

class ShogiPiece < Gtk::CBoard::Piece
  def initialize(index, window, promoted, side)
    @promoted = promoted
    @name = PieceInfo[index][0]
    @short_name = PieceInfo[index][1]
    @index = index
    
    self.offset_x = 0
    self.offset_y = 0
    self.side = side
    self.gravity = (side == BLACK ? Gtk::CBoard::GRAVITY_SOUTH : 
                          Gtk::CBoard::GRAVITY_NORTH)
    filename = PIECE_DIR + (promoted ? "P_" : "" ) + @name +
               (side == BLACK ? "_B" : "_W") + ".xpm"
    self.pixmap = Gdk::Pixmap.create_from_xpm(window, nil, filename)
  end

  attr_reader :name, :short_name, :promoted, :index
  attr_accessor :turn, :opposite
end

def Gtk.message_box(type, message, *options)
  dialog = Gtk::Dialog.new
  hbox = Gtk::HBox.new(false, 6)
  img = Gtk::Image.new(Gtk::Stock.const_get("DIALOG_" + type.to_s.upcase),
                       Gtk::IconSize::DIALOG)
  hbox.pack_start(img, false, false, 12)
  hbox.pack_start(Gtk::Label.new(message), true, true)

  dialog.vbox.pack_start(hbox, false, false)
  options.each_with_index do |title, index|
    title = Gtk::Stock.const_get(title) if title.is_a?(Symbol)
    dialog.add_button(title, index)
  end

  dialog.show_all
  ret = dialog.run
  dialog.destroy
  return ret
end

Move = Struct.new(:from, :to, :capture, :promote)

# this class handles the pieces you captured from the opponent
class Captures
  Info = Struct.new(:cnt, :nums, :rest)
  #cnt = number of this pieces captured
  #nums = overlays for displaying the number
  #rest = piece overlay for the underlying pieces

  NumWidths = [7, 4, 7, 8, 8, 8, 8, 7, 7, 8]
  
  def initialize(board, side, x, y)
    @board = board
    @side = side              # side that owns these captures
    @x = x                    # start offset x for the capture count
    @y = y                    # start offset y
    @list = Array.new(7) { Info.new(0, nil, nil) }
    # the capture count overlays:
    @num_overlays = Array.new(10) do |i|
      pixmap, mask =
        Gdk::Pixmap.create_from_xpm(@board.window, nil,
                                    PIECE_DIR + "num_" + i.to_s + ".xpm")
      Gtk::CBoard::Overlay.new(pixmap, mask)
    end
  end
  
  # display the number of captured pieces next to the piece
  def add_num(num, index)
    return if num < 2
    w = (num > 9 ? NumWidths[1] + 2 : 0)
    w += NumWidths[num % 10]
    x = @x - (w / 2)
    y = @y + index * 47
    
    overlays = []
    if num > 9
      overlay = @num_overlays[1].dup
      @board.add_overlay_now(overlay, x, y, 50)
      x += NumWidths[1] + 2
      overlays.push overlay
    end
    
    overlay = @num_overlays[num % 10].dup
    @board.add_overlay_now(overlay, x, y, 50)
    overlays.push overlay
    @list[index].nums = overlays
  end
  
  # add a piece to your captured pieces
  def add(piece)
    index = piece.index
    @list[index].cnt += 1
    count = @list[index].cnt
    
    if(count == 2)
      # add an extra overlay piece so that when dragging the piece, it
      # appears as if there are other pieces below it
      @list[index].rest = 
        @board.add_overlay_piece(piece, 0, index, @side, 40)
    end
    if(count > 2)
      @list[index].nums.each { |o|
	@board.remove_overlay(o) }
    end
    if(count > 1)
      add_num(count, index)
    end
    @board.put_piece(piece, 0, index, @side)
  end
  
  # remove a piece from your captured pieces
  def remove(piece)
    index = piece.index
    @list[index].cnt -= 1
    count = @list[index].cnt
    raise "removing unexisting captured piece" if(count < 0)

    if(count > 0)
      @board.put_piece(piece, 0, index, @side)
    else
      @board.put_piece(nil, 0, index, @side)
    end
    
    if(count == 1)
      @board.remove_overlay(@list[index].rest)
      @list[index].rest = nil
    end
    if(count >= 1)
      @list[index].nums.each { |o|
	@board.remove_overlay(o) }
      @list[index].nums = nil
    end
    if(count > 1)
      add_num(count, index)
    end
  end

  # remove all the captured pieces
  def clear
    for info in @list
      if(info.cnt > 1)
	info.nums.each { |o| @board.remove_overlay(o) }
      end
      info.nums = nil
      info.cnt = 0
      if(info.rest)
	@board.remove_overlay(info.rest)
	info.rest = nil
      end
      
    end
    7.times { |i| @board.put_piece(nil, 0, i, @side) }
  end
end


class ShogiBoard < Gtk::CBoard
  def initialize
    grids = [
      # x    y   cols rows square_w, square_h, border_x, border_y
      [561,  111, 1,   7,   47,      47,       1,        1],
      [14,   17,  1,   7,   47,      47,       1,        1],
      [107,  17,  9,   9,   47,      47,       1,        1]]

    width = 640;
    height = 458;
    super(width, height, grids)
    signal_connect("drop_piece") { |w, *a| drop_piece(*a) }
    signal_connect("init_board") { init_board }
    @promote = PROMOTE_ASK
  end

  PROMOTE_NEVER  = 0
  PROMOTE_ALWAYS = 1
  PROMOTE_ASK    = 2

  attr_accessor :promote

  def init_board
    # we need to do this initializations from here
    # because the window isn't created when calling initialize
    self.background =
         Gdk::Pixmap.create_from_xpm(self.window, nil,
                                     PIECE_DIR + "background.xpm")[0]
    hlight_pmap, hlight_mask =
          Gdk::Pixmap.create_from_xpm(self.window, nil,
                                      PIECE_DIR + "highlight.xpm")

    3.times { |grid| set_grid_highlight_normal(grid, hlight_pmap, hlight_mask) }

    @captures = Array.new(2)
    @captures[BLACK] = Captures.new(self, BLACK, 620, 132)
    @captures[WHITE] = Captures.new(self, WHITE, 73, 38)

    setup_pieces()
    reset()
  end
  private :init_board

  def setup_pieces()
    8.times do |i|
      promotes = PieceInfo[i][2]
      pair = Array.new(2)
      
      for side in BLACK..WHITE
	piece = ShogiPiece.new(i, window, false, side)
	if promotes
	  promoted = ShogiPiece.new(i, window, true, side)
	  piece.turn = promoted
	  promoted.turn = piece
	else
	  piece.turn = nil
	end
	pair[side] = piece
      end
      pair[WHITE].opposite = pair[BLACK]
      pair[BLACK].opposite = pair[WHITE]
      if promotes
	pair[BLACK].turn.opposite = pair[WHITE].turn
	pair[WHITE].turn.opposite = pair[BLACK].turn
      end
      $ShogiPieces[i] = pair
    end
  end

  def add_captured(piece)
    piece = piece.turn if piece.promoted
    @captures[piece.side].add(piece)
  end

  def remove_captured(piece)
    piece = piece.turn if piece.promoted
    @captures[piece.side].remove(piece)
  end

  def get_move(&block)
    @move_callback = block
  end

  def drop_piece(x, y, grid, to_x, to_y, to_grid)
    #move pieces only to the main board
    return false if(to_grid != MAINGRID)
    
    #if we are capturing save the piecetype of the captured piece
    old = get_piece(to_x, to_y, to_grid)
    piece = get_piece(x, y, grid)
    
    promote_to = nil
    move_done = false

    #check if we can promote, and set promote_to to the type of the promoted piece
    if((not piece.promoted) &&
          (piece.turn) &&                     #piece can promote
          (grid == MAINGRID) &&               #not dropping
          #moving from or into the last three rows
          ((piece.side == BLACK && (y <= 2 || to_y <= 2)) || #last three rows
              (piece.side == WHITE && (y >= 6 || to_y >= 6))))

      case @promote
      when PROMOTE_ALWAYS
	promote_to = piece.turn
      when PROMOTE_ASK
	cancel_move()      #avoid hanging piece
	move_done = true
	case Gtk.message_box(:question, "Promote piece?", "Don't promote", "Promote", :CANCEL)
	when 1
	  promote_to = piece.turn
	when 2
	  return
	end
      end
    end

    from = (grid == MAINGRID) ? [x, y] : piece.index
    move = Move.new(from, [to_x, to_y], old, promote_to ? true : false)

    #try the move
    if @move_callback.call(move)
      self.promote_piece = promote_to if promote_to
      if move_done
	move_piece(x, y, grid, to_x, to_y, to_grid, true)
      else
	finish_move
      end
      add_captured(old.opposite) if old
      remove_captured(piece) if grid != MAINGRID
      switch_side()
    end

    #cancel move if we haven't moved still
    return false
  end
  private :drop_piece

  def reverse_move(move)
    from, to, old, promote = move.to_a
    # we need to call end_all_animation to make sure
    # the positition is current when getting the piece
    end_all_animation()
    switch_side
    piece = get_piece(to[0], to[1], MAINGRID)
    self.promote_piece = piece.turn if promote

    if old
      remove_captured(old.opposite)
      self.previous_piece = old
    end
    
    if from.is_a? Integer
      move_piece(to[0], to[1], MAINGRID, 0, from, @side)
      add_captured($ShogiPieces[from][@side])
    else
      move_piece(to[0], to[1], MAINGRID, from[0], from[1], MAINGRID)
    end
  end

  def switch_side
    @side = (@side == BLACK ? WHITE : BLACK)
    self.active_side = @side
  end

  def reset
    freeze()
    @side = BLACK
    self.active_side = BLACK
    for y in 0..4
      for x in 0..8
	i = InitSetup[y][x]
	self.put_piece(i ? $ShogiPieces[i][WHITE] : nil,
                             x, y, MAINGRID)
      end
    end
    for y in 5..8
      for x in 0..8
	i = InitSetup[8 - y][8 - x]
	self.put_piece(i ? $ShogiPieces[i][BLACK] :
                                                   nil, x, y, MAINGRID)
      end
    end

    @captures.each { |cap| cap.clear }
    thaw()
  end

  def set_move(move)
    end_all_animation()
    from, to, old, promote = move.to_a

    if from.is_a? Integer     #dropping this piece
      piece = $ShogiPieces[from][@side]
    else
      piece = get_piece(from[0], from[1], MAINGRID)
    end

    #old value is unknown at this moment
    old = get_piece(to[0], to[1], MAINGRID)

    if(promote)
      self.promote_piece = piece.turn
    end

    if from.is_a? Integer
      move_piece(0, from, @side, to[0], to[1], MAINGRID)
    else
      move_piece(from[0], from[1], MAINGRID, to[0], to[1], MAINGRID)
    end
    add_captured(old.opposite) if old
    remove_captured($ShogiPieces[from][@side]) if from.is_a? Integer
    switch_side()

    move.capture = old
    return move;
  end
end

class GnuShogi
  def initialize
    @side = BLACK
    @thinking = false
    @discard = false
    @move_callback = nil
    @finished = false
  end

  attr_reader :finished

  def start
    @gnushogi = IO.popen("gnushogi", "w+")
    if(get() !~ /^GNU Shogi/)
      @gnushogi.close
      return false
    else
      send "force"
      # don't call return from the following proc,
      # or you will get a LocalJumpError!
      Gdk::Input.add(@gnushogi.to_i, Gdk::Input::Condition::READ) {
	text = get()
	send "force"
	@thinking = false
	if @discard
	  @discard = false
	else
	  parse text
	end
      }
      return true
    end
  end

  def send string
    @gnushogi.puts string
  end

  def get
    text = @gnushogi.gets
    text
  end
  
  def parse(text)
    if text =~ /mates/
      @finished = true
      @move_callback.call("finished")
      return
    end
    move = str2move(text.split(" ")[2])
    if move == nil
      raise "Unexpected error: incorrect move by gnushogi: " + text
    end
    @move_callback.call(move)
  end
  private :parse

  def stop
    cancel
    send "quit"
  end

  def go
    send "switch"
    @thinking = true
  end

  def reset
    cancel
    send "new"
    send "force"
    @finished = false
  end

  def force
    send "force" if @thinking
  end

  def cancel
    if @thinking
      @discard = true
      send "force"
      send "undo"
    end
  end
  
  def try_move(move)
    send move2str(move)
    
    if get() =~ /^Illegal move/
      return false
    end
    return true
  end

  def get_move(&proc)
    @move_callback = proc
  end

  def undo
    cancel
    send "undo"
  end

  def square2str(x, y)
    str = (8 - x + ?1.ord).chr
    str += (y + ?a.ord).chr
    return str
  end

  def str2square(str)
    return nil if str.length < 2
    x = ?9.ord - str[0].ord
    return nil if x > 8 or x < 0
    y = str[1].ord - ?a.ord
    return nil if y > 8 or y < 0
    return [x, y]
  end
  
  def move2str(move)
    if move.from.is_a? Integer
      string = PieceInfo[move.from][1] + "*"
    else
      string = square2str(move.from[0], move.from[1])
    end
    string += square2str(move.to[0], move.to[1])
    string += "+" if move.promote
    return string
  end

  NameToIndex = "LNSGBRPK"
  def str2move(str)
    move = Move.new
    return nil if str.length < 4
    if str[1] == ?*
      move.from = NameToIndex.index(str[0])
    else
      move.from = str2square(str[0..1])
    end
    
    move.to = str2square(str[2..3])
    move.promote = (str[4] == ?+)
    move.capture = nil

    return nil if move.from == nil or move.to == nil
    return move
  end
end

class ShogiGame
  def initialize(board, engine, statusbar)
    @board = board
    @engine = engine
    @movelist = []
    @last_move = 0
    @current_move = 0
    @side = BLACK
    @statusbar = statusbar

    @board.player_side = BLACK
    @players = [:board, :engine]
    @board.get_move  &self.method("board_move")
    @engine.get_move &self.method("engine_move")
  end

  def board_move(move)
    if @engine.try_move(move)
      switch_side(move)
      return true
    end
    return false
  end

  def engine_move(move)
    move_end() if @current_move != @last_move
    if move == "finished"
      @board.player_side = 2 # block the board
      show_move()
    else
      move = @board.set_move(move)
      switch_side(move)
    end
  end

  def switch_side(move)
    @side = (@side == BLACK ? WHITE : BLACK)
    if @engine.finished
      @board.player_side = 2
    elsif @players[@side] == :board
      @board.player_side = @side
    else
      @engine.go
    end
    @movelist.push move
    @last_move += 1
    @current_move += 1
    show_move()
  end

  def show_move
    if @current_move == 0
      str = ""
    else
      str = ((@current_move + 1) / 2).to_s + ". "
      str += "... " if @current_move % 2 == 0
      str += @engine.move2str(@movelist[@current_move - 1])
      if @current_move == @last_move && @engine.finished
	str += "(" + (@side == BLACK ? "White" : "Black")  + " Mates)"
      end
    end
    @statusbar.message str
  end

  def undo
    move_end() if @current_move != @last_move
    return if @last_move == 0
    return if @players[BLACK] == :engine && @players[WHITE] == :engine
    @engine.undo
    @side = (@side == BLACK ? WHITE : BLACK)
    @board.reverse_move(@movelist.pop)
    @last_move -= 1
    @current_move -= 1
    if @players[@side] == :engine
      undo
    else
      @board.player_side = @side
    end
  end

  def force
    @engine.force
  end

  def new_game
    @engine.reset
    @board.reset
    @engine.go if @players[BLACK] == :engine

    if @players[BLACK] == :board
      @board.player_side = BLACK
    elsif @players[WHITE] == :board
      @board.player_side = WHITE
    else
      @board.player_side = 2
    end

    @last_move = 0
    @movelist = []
    @current_move = 0
    @side = BLACK
    show_move()
  end

  def move_next
    return if @current_move == @last_move
    @board.set_move(@movelist[@current_move])
    @current_move += 1
    activate_board if @current_move == @last_move
    show_move()
  end

  def activate_board
    return if @engine.finished
    if @players[@side] == :board
      @board.player_side = @side
    else
      opposite = (@side == BLACK ? WHITE : BLACK)
      if @players[opposite] == :board
	@board.player_side = opposite
      end
    end
  end

  def move_previous
    return if @current_move == 0
    @board.player_side = 2 #deactivate board
    @current_move -= 1
    @board.reverse_move(@movelist[@current_move])
    show_move()
  end

  def move_beginning
    return if @current_move == 0
    if @current_move == 1
      move_previous()
    else
      @board.reset
      @current_move = 0
      @board.player_side = 2
      show_move()
    end
  end

  def move_end
    return if @current_move == @last_move
    if @current_move == @last_move - 1
      move_next
    else
      @board.freeze
      move_next while @current_move != @last_move
      @board.thaw
      show_move()
    end
  end

  def computer_black
    if @side == BLACK
      @engine.go if @players[BLACK] == :board
    else
      @engine.cancel if @players[WHITE] == :engine
    end
    @board.player_side = WHITE if @current_move == @last_move
    @players[WHITE] = :board
    @players[BLACK] = :engine
  end

  def computer_white
    if @side == WHITE
      @engine.go if @players[WHITE] == :board
    else
      @engine.cancel if @players[BLACK] == :engine
    end
    @board.player_side = BLACK if @current_move == @last_move
    @players[BLACK] = :board
    @players[WHITE] = :engine
  end

  def manual
    @engine.cancel if @players[@side] == :engine
    @players[BLACK] = @players[WHITE] = :board
    @board.player_side = @side
  end

  def computer_match
    @engine.go if @players[@side] == :board
    @players[BLACK] = @players[WHITE] = :engine
    @board.player_side = 2
  end
end

class MyStatusbar < Gtk::Statusbar
  def initialize
    super
    @id = get_context_id "Don't know the purpose of this"
  end
  def message(msg)
    pop @id if @message
    @message = msg
    push @id, msg if msg
  end
end    


def make_menu(info)
  menu = Gtk::MenuBar.new
  info.each{ |descr| create_submenu(menu, descr) }
  return menu
end

def create_submenu(parent, menuinfo)
  case menuinfo[0]
  when :menu
    item = Gtk::MenuItem.new(menuinfo[1])
    menu = Gtk::Menu.new()
    item.set_submenu(menu)
    menuinfo[2..-1].each{ |descr| create_submenu(menu, descr) }
  when :item
    item = Gtk::MenuItem.new(menuinfo[1])
    item.signal_connect("activate") { |w|
      menuinfo[2].call }
  when :separator
    item = Gtk::MenuItem.new()
  when :check
    item = Gtk::CheckMenuItem.new(menuinfo[1])
    item.active = true if menuinfo[3]
    item.signal_connect("toggled") { |w|
      menuinfo[2].call(w.active?) }
  when :radio
    radio = nil
    for info in menuinfo[1..-1]
      radio = radio ? Gtk::RadioMenuItem.new(radio, info[0]) :
              Gtk::RadioMenuItem.new(info[0])
      radio.active = true if info[2]
      radio.signal_connect("toggled", info[1]) { |w, f|
	f.call if w.active? }
      parent.append(radio)
    end
    return
  end
  parent.append(item)
end

gnushogi = GnuShogi.new
if (!gnushogi.start)
  Gtk.message_box(:error, "Couldn't start gnushogi.  This demo requires gnushogi to run.", :OK)
  exit
end

begin
  Gtk.init
  window = Gtk::Window.new(Gtk::Window::TOPLEVEL)
  board = ShogiBoard.new
  statusbar = MyStatusbar.new
  game = ShogiGame.new(board, gnushogi, statusbar)

  mainmenu = make_menu [
    [:menu, "Game",
     [:item,  "New", game.method(:new_game)],
     [:separator],
     [:radio,
      ["Computer Black", game.method(:computer_black)],
      ["Computer White", game.method(:computer_white), true],
      ["Manual",         game.method(:manual)],
      ["Computer Match", game.method(:computer_match)]],
     [:separator],
     [:item,  "Exit",     Gtk.method(:main_quit)]],
    [:menu, "Moves",
     [:item,  "Take Back",  game.method(:undo)],
     [:item,  "Force Move", game.method(:force)],
     [:separator],
     [:item,  "Next",       game.method(:move_next)],
     [:item,  "Previous",   game.method(:move_previous)],
     [:item,  "First",      game.method(:move_beginning)],
     [:item,  "Last",       game.method(:move_end)]],
    [:menu, "Options",
     [:check, "Move by Dragging", board.method(:enable_dragging=),      true],
     [:check, "Move by Clicking", board.method(:enable_clicking_move=), true],
     [:check, "Animate Moving",   board.method(:enable_animated_move=), true],
     [:check, "Highlight Last Move", board.method(:enable_highlight=),  true],
     [:check, "Flash Moves",      board.method(:enable_flashing=),      true]]]

  hbox = Gtk::HBox.new(false, 4)

  hbox.pack_start(Gtk::Label.new("Promote: "), false, true)
  radio = nil
  for a in [
      [Gtk::Stock::YES, ShogiBoard::PROMOTE_ALWAYS],
      [Gtk::Stock::NO,  ShogiBoard::PROMOTE_NEVER],
      [Gtk::Stock::DIALOG_QUESTION, ShogiBoard::PROMOTE_ASK]]
    radio = radio ? Gtk::RadioButton.new(radio, a[0]) :
            Gtk::RadioButton.new(a[0])
    radio.signal_connect("toggled", a[1]) { |w, v|
      board.promote = v if w.active? }
    hbox.pack_start(radio, false, true)
  end
  radio.active = true
  
  hbox.pack_start(statusbar, true, true)
  
  buttonbox = Gtk::HBox.new(true)
  [[Gtk::Stock::GOTO_FIRST, game.method(:move_beginning)],
   [Gtk::Stock::GO_BACK,  game.method(:move_previous) ],
   [Gtk::Stock::GO_FORWARD,  game.method(:move_next) ],
   [Gtk::Stock::GOTO_LAST, game.method(:move_end) ]].each do |a|
    img = Gtk::Image.new(a[0], Gtk::IconSize::MENU)
    button = Gtk::Button.new.add(img)
    button.signal_connect("clicked", a[1]) { |w, f| f.call }
    buttonbox.pack_start(button, false, true)
  end
  hbox.pack_start(buttonbox, false, true)
  
  vbox = Gtk::VBox.new
  vbox.pack_start(mainmenu)
  vbox.pack_start(board)
  vbox.pack_start(Gtk::Frame.new(nil).add(hbox))
  
  window.add(vbox).show_all
  window.signal_connect("destroy") { |w| 
    Gtk.main_quit }
  Gtk.main
ensure
  gnushogi.stop
end
