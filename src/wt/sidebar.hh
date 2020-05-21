#pragma once

//#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WLineEdit.h>

#include "robots.hh"

#include <string>

//#include "board_widget.hh"
class ScoreWidget : public Wt::WTable {
public:
  ScoreWidget(){
    setHeaderCount(0);

    score_ = elementAt(0, 0)->addNew< Wt::WLineEdit >( "Score: 0" );
    score_->setReadOnly( true );


    tele_ = elementAt(1, 0)->addNew< Wt::WLineEdit >( "Teleports: 0" );
    tele_->setReadOnly( true );
  }

  void update( RobotsGame const & game  ){
    score_->setText( "Score: " + std::to_string( game.score() ) );
    tele_->setText( "Teleports: " + std::to_string( game.n_safe_teleports_remaining()));
  }

private:
  Wt::WLineEdit * score_;
  Wt::WLineEdit * tele_;
};
