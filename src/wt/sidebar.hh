#pragma once

//#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WPushButton.h>
#include <Wt/WLineEdit.h>

#include "robots.hh"

#include <string>
#include <iostream>

//#include "board_widget.hh"
class ScoreWidget : public Wt::WTable {
public:
  ScoreWidget(){
    setHeaderCount(0);

    score_ = elementAt(0, 0)->addNew< Wt::WLineEdit >( "Score: 0" );
    score_->setReadOnly( true );

    tele_ = elementAt(1, 0)->addNew< Wt::WLineEdit >( "Teleports: 0" );
    tele_->setReadOnly( true );

    auto button = elementAt(2, 0)->addNew< Wt::WPushButton >( "Controls" );
    button->checked().connect
      (
       [=] {
	 std::cout << "CLICK" << std::endl;
	 display_controls();
       }
       );
  }

  void display_controls(){
    Wt::WMessageBox * const messageBox = this->addChild(
      Wt::cpp14::make_unique< Wt::WMessageBox >(
	"Controls",
	"<p>Move: QWEASDZXC\nTeleport: T\nWait: SpaceBar\nSee Last State: '?'</p>",
	Wt::Icon::Warning, Wt::StandardButton::Yes //| Wt::StandardButton::No
      )
    );
    messageBox->buttonClicked().connect(
      [=] {
	if( messageBox->buttonResult() == Wt::StandardButton::Yes ) {
	  this->removeChild( messageBox );
	}
      }
    );
    messageBox->show();
  }

  
  void update( RobotsGame<> const & game  ){
    score_->setText( "Score: " + std::to_string( game.score() ) );
    tele_->setText( "Teleports: " + std::to_string( game.n_safe_teleports_remaining()));
  }

private:
  Wt::WLineEdit * score_;
  Wt::WLineEdit * tele_;
};
