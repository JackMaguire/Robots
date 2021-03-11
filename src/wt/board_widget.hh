#pragma once

#include <Wt/WPaintDevice.h>
#include <Wt/WPaintedWidget.h>
#include <Wt/WPen.h>
#include <Wt/WPainter.h>
#include <Wt/WInteractWidget.h>
#include <Wt/WMessageBox.h>
#include <Wt/WApplication.h>

#include "robots.hh"
#include "sidebar.hh"

#include <iostream>
#include <mutex>


class BoardWidget : public Wt::WPaintedWidget {//, Wt::WInteractWidget {
public:
  BoardWidget(
    ScoreWidget * sidebar,
    Wt::WApplication * app
  ):
    game_(),
    sidebar_( sidebar ),
    app_( app )
  {
    setLayoutSizeAware( true );
    setSelectable( true );
    setCanReceiveFocus( true );
    //resize(200, 60);
    init_listeners();

    cached_board_ = game_.board();
  }

  void init_listeners(){
    //keyWentDown().connect( this, & BoardWidget::keyDown );
    keyPressed().connect( this, & BoardWidget::keyDown );
  }

  void paintEvent( Wt::WPaintDevice * paintDevice ) override {
    Wt::WPainter painter( paintDevice );

    int const grid_size = calc_grid_size();
    draw_background( painter, grid_size );

    if( display_cached_board_ ) {
      draw_foreground( cached_board_, painter, grid_size );
    } else {
      draw_foreground( game_.board(), painter, grid_size );
    }
  }

  void layoutSizeChanged(int width, int height) override {
    Wt::WPaintedWidget::layoutSizeChanged( width, height );
    width_ = width;
    height_ = height;
    update();
  }

  int calc_grid_size(){
    int const w = width_ / WIDTH;
    int const h = height_ / HEIGHT;
    return ( w < h ? w : h );
  }

protected:
  void draw_background( Wt::WPainter & painter, int const grid_size ){

    Wt::WPen pen;
    pen.setWidth( 0.1 );
    painter.setPen( pen );

    Wt::WColor c1( 220, 220, 220 );
    Wt::WColor c2( 200, 200, 200 );

    bool use_c1 = false;

    for( int i = 0; i < WIDTH; ++i ){
      for( int j = 0; j < HEIGHT; ++j ){
	use_c1 = !use_c1;
	if( use_c1 ) painter.setBrush(Wt::WBrush(c1));
	else         painter.setBrush(Wt::WBrush(c2));
	painter.drawRect( i*grid_size, j*grid_size, grid_size, grid_size );
      }
      use_c1 = !use_c1;
    }

  }

  void draw_foreground( Board const & board, Wt::WPainter & painter, int const grid_size ){
  
    Wt::WColor const human_color = ( safe_mode_ ? Wt::WColor( 10, 200, 10 ) : Wt::WColor( 10, 100, 10 ) );
    //if ( safe_mode_ ) Wt::WColor const human_color( 10, 150, 10 );
    //else Wt::WColor const human_color( 10, 10, 150 );

    Wt::WColor const robot_color( 10, 10, 100 );
    Wt::WColor const fire_color( 200, 10, 10 );
    //Wt::WColor const ml_color( 80, 80, 10 );
    //Wt::WColor const safety_color( 120, 200, 120 );
    Wt::WColor const safety_color( 0, 0, 0 );

    Wt::WBrush const robot_brush( robot_color );
    Wt::WBrush const human_brush( human_color );
    Wt::WBrush const fire_brush( fire_color );
    //Wt::WBrush const ml_brush( ml_color );
    Wt::WBrush safety_brush( safety_color );
    safety_brush.setStyle( Wt::BrushStyle::None );
    
    if( show_safe_moves_ ){ //safety

      Wt::WPen pen;
      pen.setWidth( 1.0 );
      painter.setPen( pen );

      painter.setBrush( safety_brush );
      auto const human_p = board.human_position();
      for( int dx = -1; dx <= 1; ++dx ){
	if( human_p.x + dx >= WIDTH ) continue;
	for( int dy = -1; dy <= 1; ++dy ){
	  if( human_p.y + dy >= HEIGHT ) continue;
	  if( board.move_is_cascade_safe( dx, dy ) ){
	    int const i = human_p.x + dx;
	    int const j = (HEIGHT-1) - (human_p.y + dy);
	    painter.drawEllipse( i*grid_size, j*grid_size, grid_size, grid_size );
	    //painter.drawChord( i*grid_size, j*grid_size, grid_size, grid_size, 0, 2880*2 );
	  }
	}
      }
    }

    { // members

      Wt::WPen pen;
      pen.setWidth( 0.1 );
      painter.setPen( pen );

      Position p;
      for( int i = 0; i < WIDTH; ++i ){
	p.x = i;
	for( int j = 0; j < HEIGHT; ++j ){
	  p.y = (HEIGHT-1) - j;
	  switch( board.cell( p ) ){
	  case( Occupant::EMPTY ): continue;
	  case( Occupant::ROBOT ): painter.setBrush( robot_brush ); break;
	  case( Occupant::HUMAN ): painter.setBrush( human_brush ); break;
	  case( Occupant::FIRE ):  painter.setBrush(  fire_brush ); break;
	  }
	  painter.drawEllipse( i*grid_size +1, j*grid_size +1, grid_size -2, grid_size -2 );
	}
      }
    }
  }

  void keyDown( Wt::WKeyEvent const & e ){
    if( ignore_keys_ ) return;

    ignore_keys_ = true;

    //std::cout << "KEY " << e.charCode() << std::endl;
    std::lock_guard<std::mutex> guard( move_mutex_ );

    switch( e.charCode() ){
    case( 'q' ):
    case( 'Q' ):
      handle_move( -1,  1 );
    break;

    case( 'w' ):
    case( 'W' ):
      handle_move(  0,  1 );
    break;

    case( 'e' ):
    case( 'E' ):
      handle_move(  1,  1 );
    break;


    case( 'a' ):
    case( 'A' ):
      handle_move( -1,  0 );
    break;

    case( 's' ):
    case( 'S' ):
      handle_move(  0,  0 );
    break;

    case( 'd' ):
    case( 'D' ):
      handle_move(  1,  0 );
    break;


    case( 'z' ):
    case( 'Z' ):
      handle_move( -1, -1 );
    break;

    case( 'x' ):
    case( 'X' ):
      handle_move(  0, -1 );
    break;

    case( 'c' ):
    case( 'C' ):
      handle_move(  1, -1 );
    break;


    case( 't' ):
    case( 'T' ):
      handle_move( -1, -1, true );
    break;

    case( ' ' ):
      handle_move( -1, -1, false, true );
      break;

    case( '?' ):
      toggle_cached_board();
      break;


      //MODES
    case( '1' ):
      safe_mode_ = !safe_mode_;
      this->update();
      break;

    case( '2' ):
      show_safe_moves_ = !show_safe_moves_;
      this->update();	
      break;

    default:
      break;
    }

    ignore_keys_ = false;
  }

  void toggle_cached_board(){
    display_cached_board_ = ! display_cached_board_;
    update();
  }

  void display_endgame( std::string const & text ){
    Wt::WMessageBox * const messageBox = addChild(
      Wt::cpp14::make_unique< Wt::WMessageBox >(
	text,
	"<p>"+text+". Play Again?</p>",
	Wt::Icon::Warning, Wt::StandardButton::Yes //| Wt::StandardButton::No
      )
    );
    messageBox->buttonClicked().connect(
      [=] {
	if( messageBox->buttonResult() == Wt::StandardButton::Yes ) {
	  game_ = decltype(game_)();
	  update();
	}
	this->removeChild( messageBox );
      }
    );
    messageBox->show();
  }

  bool handle_wait(){
    //TODO look into WTimer: https://www.webtoolkit.eu/wt/doc/reference/html/classWt_1_1WTimer.html
    //This has some good ideas too: https://redmine.webtoolkit.eu/boards/2/topics/14880?r=14884#message-14884

    //TODO
    //This is going to require some hacking.
    //Need to cascade INSIDE the painting function?
    //But maybe that still won't work
    bool const game_over = game_.cascade( [=](){
	std::cout << "update!" << std::endl;
	//std::this_thread::sleep_for(std::chrono::milliseconds(250));
	this->update();
	app_->processEvents();
      } );
    return game_over;
  }

  void handle_move( int dx, int dy, bool teleport = false, bool wait = false ){
    if( display_cached_board_ == true ){
      display_cached_board_ = false;
      update();
      return;//Don't make them make a mistake
    }

    cached_board_ = game_.board();

    bool game_over = false;
    if( wait ){
      game_over = handle_wait();
    } else if( teleport ){
      game_over = game_.teleport();
    } else {
      if( safe_mode_ ){
	if( game_.board().move_is_safe( dx, dy ) ){
	  game_over = game_.move_human( dx, dy );
	}
      } else {
	game_over = game_.move_human( dx, dy );
      }
    }
    update();

    if( game_over ){
      switch( game_.latest_result() ){
      case( MoveResult::YOU_LOSE ):
	display_endgame( "You Lose" );
	break;
      case( MoveResult::YOU_WIN_GAME ):
	display_endgame( "You Beat The Game!" );
	break;
      default:
	//TODO - error?
	break;
      }
    }

    sidebar_->update( game_ );
  }

private:
  RobotsGame< NullVisualizer, true, 100 > game_;
  int width_ = 0;
  int height_ = 0;

  std::mutex move_mutex_;

  ScoreWidget * sidebar_;
  Wt::WApplication * app_;

  Board cached_board_;
  bool display_cached_board_;

  bool safe_mode_ = false;
  bool show_safe_moves_ = true;

  bool ignore_keys_ = false;
};
