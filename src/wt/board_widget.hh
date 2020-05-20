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
    pen.setWidth( 0.2 );
    painter.setPen( pen );

    int const grid_size = calc_grid_size();
    draw_background( painter, grid_size );
    draw_foreground( painter, grid_size );
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

  void draw_foreground( Wt::WPainter & painter, int const grid_size ){
    Wt::WColor robot_color( 10, 10, 100 );
    Wt::WColor human_color( 10, 100, 10 );
    Wt::WColor fire_color( 200, 10, 10 );
    Wt::WColor ml_color( 80, 80, 10 );
  }
  
private:
  RobotsGame<> game_;
  int width_ = 0;
  int height_ = 0;
};
