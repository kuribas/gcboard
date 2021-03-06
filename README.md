GCBoard
=======

GCBoard is free software.  See the file COPYING for copying conditions.
See the file INSTALL for building and installation instructions.
The documentation is not written yet, sorry.  Please be patient.

GCBoard (Generic Chess Board) is an easy to use gameboard API for
Chess-style games (i.e. Draughts, Shogi, Xiangqi, Checkers, Chess, and
variants).  It implements most of the expected features (dragging,
highlighting, animating, flashing, ...), and handles them
transparently for the programmer.  Gcboard will support your game if
it has the following properties:

* The game is turn based (from 2 to 15 players).

* Pieces are put on one or more rectangular grids (typically one).

* Moves are made by moving a piece from it's square to another
	(either empty or containing an enemy piece).
      
An example of games not supported: go, othello, monopoly, backgammon.
    
gcboard is platform independent.  The core of the library is written
in (hopefully) portable C.  Supporting new platforms and toolkits
should be very easy: only create a wrapper around the independent
code.  Currently there is only a gtk+ widget.  A wxWindows version
will follow soon.  Any contributions to support other toolkits and
platforms are welkom! (i.e. MS Windows, Qt, Mac Os)

gcboard is especially optimized to be used with interpreted languages,
such as ruby, scheme, python and perl.  Most of the cpu-intensive
features are already implemented, so you can write your program in
your favorite language, instead of having to write it in C.  There are
already bindings for ruby, and I would gladly accept contributions for
other bindings.

The reason that I wrote this library is that other people can produce
beautiful games, without much effort.  Instead of having to write a
full interface each time from scratch, it is now possible to
concentrate on the game-specific parts.

Gcboard is released under the LGPL (See LICENCE).

INSTALL
-------

To compile the latest version, clone this repository:

    sudo apt-get install git
    git clone https://github.com/kuribas/gcboard

To compile the gtk library you'll need the gcc compiler,
autotools, libtool, and the gtk2 development libraries.  For the
ruby bindings you need additionaly the ruby and ruby-gnome2 development libraries.

For example on ubuntu/mint:

    sudo apt-get install autotools-dev libtool gcc libgtk2.0-dev` #ruby-gnome2-dev
    ./autogen.sh
    ./configure # --enable-ruby 
    make
    sudo make install

DEMO
----

There is a demo in the `ruby/demo` directory.  It shows a working
interface for the gnushogi engine.  To run this demo, first install the
ruby bindings, and make sure _gnushogi_ is somewhere in the path.
Then run the shogi.rb program from the `ruby/demo` directory.
