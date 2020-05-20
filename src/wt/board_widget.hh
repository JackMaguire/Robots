#pragma once

#include <Wt/WPaintDevice.h>
#include <Wt/WPaintedWidget.h>
#include <Wt/WPen.h>
#include <Wt/WPainter.h>

#include "robots.hh"

//#include <iostream>

class BoardWidget : public Wt::WPaintedWidget {
public:
  BoardWidget(){
    setLayoutSizeAware( true );
    setSelectable( false );
    //resize(200, 60);
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
  
private:
  RobotsGame<> game_;
  int width_ = 0;
  int height_ = 0;
};
