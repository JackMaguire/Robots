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
    button->clicked().connect
      (
       [=] {
	 std::cout << "CLICK" << std::endl;
	 display_controls();
       }
       );

    //safe_ = elementAt(3, 0)->addNew< Wt::WLineEdit >( "Safe Mode: off" );
    //safe_->setReadOnly( true );

    //scout_ = elementAt(4, 0)->addNew< Wt::WLineEdit >( "Scout Mode: on" );
    //scout_->setReadOnly( true );

    //ml_ = elementAt(5, 0)->addNew< Wt::WLineEdit >( "ML Mode: off" );
    //ml_->setReadOnly( true );
  }

  void display_controls(){
    Wt::WMessageBox * const messageBox = this->addChild(
      Wt::cpp14::make_unique< Wt::WMessageBox >(
	"Controls",
	"<p>Move: QWEASDZXC</p>"
	"<p>Teleport: T</p>"
	"<p>Wait: SpaceBar</p>"
	"<p>See Last State: '?'</p>"
	"<p>Toggle Safe Mode: '1' (human will be lighter green in safe mode)</p>"
	"<p>Toggle Scout Mode: '2'</p>"
	"<p>Toggle ML Mode: '3'</p>"
	"<p>Sometimes you need to click on the board for it to start listening to your keys</p>",
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

  
  template< typename GAME >
  void update(
    GAME const & game,
    [[maybe_unused]] bool const safe_mode,
    [[maybe_unused]] bool const scout_mode,
    [[maybe_unused]] bool const ml_mode
  ){
    score_->setText( "Score: " + std::to_string( game.score() ) );
    tele_->setText( "Teleports: " + std::to_string( game.n_safe_teleports_remaining()));
    //safe_->setText( "Safe Mode: " + std::string(safe_mode?"on":"off") );
    //scout_->setText( "Scout Mode: " + std::string(scout_mode?"on":"off") );
    //ml_->setText( "ML Mode: " + std::string(ml_mode?"on":"off") );
  }

private:
  Wt::WLineEdit * score_;
  Wt::WLineEdit * tele_;
  //Wt::WLineEdit * safe_;
  //Wt::WLineEdit * scout_;
  //Wt::WLineEdit * ml_;
};
