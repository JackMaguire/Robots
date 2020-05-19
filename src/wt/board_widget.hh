#pragma once

#include <Wt/WPaintDevice.h>
#include <Wt/WPaintedWidget.h>
#include <Wt/WPen.h>
#include <Wt/WPainter.h>

#include "robots.hh"

class BoardWidget : public Wt::WPaintedWidget, public OptionsHolder, public Refreshable {

  void paintEvent( Wt::WPaintDevice * paintDevice ) {
    Wt::WPainter painter( paintDevice );
    int const grid_size = 25; //TODO

    for( int i = 0; i < WIDTH; ++i ){
      for( int j = 0; j < HEIGHT; ++j ){
	
	painter.drawRect( sx, sy, grid_size, grid_size );
      }
    }
  }

private:
  RobotsGame game_;
}
