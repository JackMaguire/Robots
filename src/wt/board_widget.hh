#pragma once

#include <Wt/WPaintDevice.h>
#include <Wt/WPaintedWidget.h>
#include <Wt/WPen.h>
#include <Wt/WPainter.h>
#include <Wt/WInteractWidget.h>
#include <Wt/WMessageBox.h>

#include "robots.hh"
#include "sidebar.hh"

#include <iostream>
#include <mutex>


class BoardWidget : public Wt::WPaintedWidget {//, Wt::WInteractWidget {
public:
  BoardWidget( ScoreWidget * sidebar ):
    sidebar_(sidebar)
  {
    setLayoutSizeAware( true );
    setSelectable( true );
    setCanReceiveFocus( true );
    //resize(200, 60);
    init_listeners();
  }

  void init_listeners(){
    //keyWentDown().connect( this, & BoardWidget::keyDown );
    keyPressed().connect( this, & BoardWidget::keyDown );
  }
  
  void paintEvent( Wt::WPaintDevice * paintDevice ) override {
    Wt::WPainter painter( paintDevice );
    Wt::WPen pen;
    pen.setWidth( 0.1 );
    painter.setPen( pen );

    int const grid_size = calc_grid_size();
    draw_background( painter, grid_size );
    draw_foreground( game_.board(), painter, grid_size );
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
    Wt::WColor const robot_color( 10, 10, 100 );
    Wt::WColor const human_color( 10, 100, 10 );
    Wt::WColor const fire_color( 200, 10, 10 );
    //Wt::WColor const ml_color( 80, 80, 10 );

    Wt::WBrush const robot_brush( robot_color );
    Wt::WBrush const human_brush( human_color );
    Wt::WBrush const fire_brush( fire_color );
    //Wt::WBrush const ml_brush( ml_color );

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
	painter.drawEllipse( i*grid_size, j*grid_size, grid_size, grid_size );
      }
    }
    
  }

  void keyDown( Wt::WKeyEvent const & e ){
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
    
    default:
      return;
    }
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
	  game_ = RobotsGame<>();
	  update();
	}
	this->removeChild( messageBox );
      }
    );
    messageBox->show();
  }


  void handle_move( int dx, int dy, bool teleport = false, bool wait = false ){
    bool game_over = false;
    if( wait ){
      //TODO
      //This is going to require some hacking.
      //Need to cascade INSIDE the painting function?
      //But maybe that still won't work
      game_over = game_.cascade( [=](){
	  std::cout << "update!" << std::endl;
	  this->update();
	} );
    } else if( teleport ){
      game_over = game_.teleport();
    } else {
      game_over = game_.move_human( dx, dy );      
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
  RobotsGame<> game_;
  int width_ = 0;
  int height_ = 0;

  std::mutex move_mutex_;

  ScoreWidget * sidebar_;
};
