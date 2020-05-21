#pragma once

//#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WLineEdit.h>

#include <string>

//#include "board_widget.hh"
class ScoreWidget : public Wt::WTable {
public:
  ScoreWidget(){
    setHeaderCount(0);

    score_ = elementAt(0, 0)->addNew< Wt::WLineEdit >( "Score: 0" );
    score_->setReadOnly( true );
  }

  void update_with_new_score( long unsigned int const score ){
    score_->setText( "Score: " + std::to_string( score ) );
  }

private:
  Wt::WLineEdit * score_;
};
