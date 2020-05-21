#pragma once

//#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WLineEdit.h>

//#include "board_widget.hh"
class ScoreWidget : public Wt::WTable {
public:
  ScoreWidget(){
    setHeaderCount(0);
    table->elementAt(0, 0)->addNew< Wt::WLineEdit >( "Score" );
  }

private:
  
}
