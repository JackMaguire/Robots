#pragma once

//#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WText.h>

//#include "board_widget.hh"
class ScoreWidget : public Wt::WTable {
public:
  ScoreWidget(){
    setHeaderCount(0);
    elementAt(0, 0)->addNew< Wt::WText >( "Score:" );
    elementAt(1, 0)->addNew< Wt::WText >( "0" );
  }

private:
  
};
