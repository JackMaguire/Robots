#pragma once

#include <Wt/WPaintDevice.h>
#include <Wt/WPaintedWidget.h>
#include <Wt/WPen.h>
#include <Wt/WPainter.h>
#include <Wt/WInteractWidget.h>
#include <Wt/WMessageBox.h>
#include <Wt/WApplication.h>
#include <Wt/WIntValidator.h>
#include <Wt/WSlider.h>
#include <Wt/WGroupBox.h>
#include <Wt/WCheckBox.h>

#include <wt_util/SingleStringDialog.hh>
#include <wt_util/SinglePromptDialog.hh>

#include "robots.hh"
#include "gcn.hh"
#include "sidebar.hh"
#include "predict_gcn.hh"
#include "autopilot.hh"
#include "recursion.hh"

#include <iostream>
#include <mutex>
#include <string>
//#include <array>

//Logger
#include <cstdlib> //rand()
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <sstream>

/*struct AutoPilotResult {
  Prediction move;
  bool teleport = false;
  bool cascade = false;
};*/


struct Logger {
  std::stringstream ss;

public:

  ~Logger(){
    std::string const filename =
      "round2_data/" + std::to_string( rand() );

    std::string const data = ss.str();

    if( ! data.empty() ){
      std::ofstream myfile;
      myfile.open( filename );
      myfile << data;
      myfile.close();    
    }
  }

  template< typename GAME >
  void log(
    GAME const & game,
    Key const key
  ){
    ss << game.board().get_stringified_representation()
      << ',' << game.n_safe_teleports_remaining()
      << ',' << game.round()
      << ',' << int( key ) << '\n';
  }

};

struct PaintPalette {

  Wt::WBrush human_brush;
  Wt::WBrush safe_human_brush;
  Wt::WBrush robot_brush;
  Wt::WBrush fire_brush;
  Wt::WBrush ml_brush;
  Wt::WBrush safety_brush;

  Wt::WPen wide_pen;
  Wt::WPen medium_pen;
  Wt::WPen thin_pen;

  PaintPalette() :
    human_brush( Wt::WColor( 10, 100, 10 ) ),
    safe_human_brush( Wt::WColor( 10, 200, 10 ) ),
    robot_brush( Wt::WColor( 10, 10, 100 ) ),
    fire_brush( Wt::WColor( 200, 10, 10 ) ),
    ml_brush( Wt::WColor( 250, 250, 250 ) ),
    safety_brush( Wt::WColor( 0, 0, 0 ) )
  {
    safety_brush.setStyle( Wt::BrushStyle::None );

    wide_pen.setWidth( 1.0 );
    medium_pen.setWidth( 0.6 );
    thin_pen.setWidth( 0.1 );
  }
};

template< typename GAME >
class BoardWidget : public Wt::WPaintedWidget {//, Wt::WInteractWidget {
private:
  GAME game_;
  int width_ = 0;
  int height_ = 0;

  std::mutex move_mutex_;

  const PaintPalette palette_;

  ScoreWidget * sidebar_;
  // NEVER nullptr
  // NEVER changes

  Wt::WApplication * app_;

  //GCN gcn_;
  OldSchoolGCN gcn_;
  Prediction current_prediction_;

  Board cached_board_;
  bool display_cached_board_;

  bool safe_mode_ = true;
  bool show_safe_moves_ = true;
  bool show_ml_ = true;
  bool show_lines_ = false;

  bool ignore_keys_ = false;

  Logger logger_;
  bool dump_training_data_ = false;

public:
  BoardWidget(
    ScoreWidget * sidebar,
    Wt::WApplication * app
  ):
    game_(),
    sidebar_( sidebar ),
    app_( app )
  {
    setLayoutSizeAware( true );
    setSelectable( true );
    setCanReceiveFocus( true );
    //resize(200, 60);
    init_listeners();

    cached_board_ = game_.board();

    set_up_buttons();
  }

  void set_up_buttons(){
    int row = 2;

    Wt::WGroupBox * settings_box = sidebar_->elementAt(++row, 0)->addNew<Wt::WGroupBox>( "Settings" );

    Wt::WCheckBox * show_lines = settings_box->addNew< Wt::WCheckBox >( "Show Graph" );
    show_lines->checked().connect(
      [=](){
	show_lines_ = true;
	update();
      }
    );
    show_lines->unChecked().connect(
      [=](){
	show_lines_ = false;
	update();
      }
    );

    settings_box->addNew< Wt::WBreak >();
    settings_box->addNew<Wt::WText>( "Game Speed:" );

    Wt::WSlider *slider = settings_box->addNew<Wt::WSlider>();
    slider->setTickInterval(50);
    slider->setMinimum(0);
    slider->setMaximum(1000);
    slider->setValue(0);
    

    make_ml_box( row, slider );
    make_recursion_box( row, slider );
    make_hybrid_box( row, slider );
  }

  void
  make_ml_box( int & row, Wt::WSlider * gs_slider ){
    Wt::WGroupBox * box = sidebar_->elementAt(++row, 0)->addNew<Wt::WGroupBox>( "XENet" );

    box->addNew<Wt::WText>( "Number of moves to perform:" );

    box->addNew< Wt::WBreak >();


    Wt::WLineEdit * n_ml = box->addNew< Wt::WLineEdit >( "1" );
    auto validator = std::make_shared< Wt::WIntValidator >( 1, 1000 );
    validator->setMandatory( true );
    n_ml->setValidator( validator );
      
    auto && get_n_ml = [=]() -> int {
      try {
	int const answer = std::stoi( n_ml->text().toUTF8() );

	if( answer < 0 or answer > 1000 ){
	  n_ml->setText( "Please provide an integer between 0 and 1000" );
	} else {
	  return answer;
	}
      } catch( ... ){
	n_ml->setText( "Please provide an integer" );
      }
      return 0;
    };

    box->addNew< Wt::WBreak >();

    box->addNew< Wt::WPushButton >( "Listen to ML" )->clicked().connect(
      [=]{
	if( ignore_keys_ ) return; 
	if( show_ml_ ){
	  ignore_keys_ = true;

	  int const nloop = get_n_ml();
	  loop_autopilot( -1, gs_slider->value(), nloop );

	  ignore_keys_ = false;
	}
      }
    );
  }

  
  void
  make_recursion_box( int & row, Wt::WSlider * gs_slider ){
    Wt::WGroupBox * recursive_box = sidebar_->elementAt(++row, 0)->addNew<Wt::WGroupBox>( "Recursion" );

    Wt::WText * label = recursive_box->addNew<Wt::WText>( "5 moves ahead" );

    recursive_box->addNew< Wt::WBreak >();

    Wt::WSlider *slider = //sidebar_->elementAt(++row, 0)->addNew<Wt::WSlider>();
      recursive_box->addNew<Wt::WSlider>();
    slider->setTickInterval(1);
    slider->setMinimum(3);
    slider->setMaximum(7);
    slider->setValue(5);

    slider->valueChanged().connect(
      [=](){
	label->setText( std::to_string(slider->value()) + " moves ahead" );
      }
    );

    recursive_box->addNew< Wt::WBreak >();

    Wt::WPushButton * recurse = recursive_box->addNew< Wt::WPushButton >( "Run Recursion" );
    recurse->clicked().connect(
      [=]{
	if( ignore_keys_ ) return; 
	if( show_ml_ ){
	  recurse->setEnabled( false );
	  ignore_keys_ = true;

	  switch( slider->value() ){
	  case( 3 ):
	    run_recursive_search< 3 >( gs_slider->value(), 8 );
	    break;
	  case( 4 ):
	    run_recursive_search< 4 >( gs_slider->value(), 8 );
	    break;
	  case( 5 ):
	    run_recursive_search< 5 >( gs_slider->value(), 8 );
	    break;
	  case( 6 ):
	    run_recursive_search< 6 >( gs_slider->value(), 8 );
	    break;
	  case( 7 ):
	    run_recursive_search< 7 >( gs_slider->value(), 8 );
	    break;
	  default:
	    assert( false );
	  }

	  ignore_keys_ = false;
	  recurse->setEnabled( true );
	}
      }
    );
  }

  void
  make_hybrid_box( int & row, Wt::WSlider * gs_slider ){
    Wt::WGroupBox * box = sidebar_->elementAt(++row, 0)->addNew<Wt::WGroupBox>( "Beat The Game" );

    box->addNew< Wt::WPushButton >( "Skip to Risky" )->clicked().connect(
      [=]{
	if( ignore_keys_ ) return; 
	if( show_ml_ ){
	  ignore_keys_ = true;
	  skip_to_risky_w_recursion( gs_slider->value() );
	  ignore_keys_ = false;
	}
      }
    );

    box->addNew< Wt::WBreak >();

    box->addNew<Wt::WText>( "Warning: this will not stop once started!" );

    box->addNew< Wt::WBreak >();

    box->addNew< Wt::WPushButton >( "Run Hybrid" )->clicked().connect(
      [=]{
	if( ignore_keys_ ) return; 
	if( show_ml_ ){
	  ignore_keys_ = true;

	  loop_smart_autopilot( gs_slider->value() );

	  ignore_keys_ = false;
	}
      }
    );

  }


  void init_listeners(){
    //keyWentDown().connect( this, & BoardWidget::keyDown );
    keyPressed().connect( this, & BoardWidget::keyDown );
  }

  void paintEvent( Wt::WPaintDevice * paintDevice ) override {
    Wt::WPainter painter( paintDevice );

    int const grid_size = calc_grid_size();
    draw_background( painter, grid_size );

    if( display_cached_board_ ) {
      draw_foreground( cached_board_, painter, grid_size );
    } else {
      draw_foreground( game_.board(), painter, grid_size );
    }
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
  void
  draw_background( Wt::WPainter & painter, int const grid_size );

  void
  draw_foreground(
    Board const & board,
    Wt::WPainter & painter,
    int const grid_size
  );

  void keyDown( Wt::WKeyEvent const & e );

  void toggle_cached_board(){
    display_cached_board_ = ! display_cached_board_;
    update();
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
	  game_ = GAME();
	  update();
	}
	this->removeChild( messageBox );
      }
    );
    messageBox->show();
  }

  bool handle_wait(){
    //TODO look into WTimer: https://www.webtoolkit.eu/wt/doc/reference/html/classWt_1_1WTimer.html
    //This has some good ideas too: https://redmine.webtoolkit.eu/boards/2/topics/14880?r=14884#message-14884

    if( safe_mode_ ){
      if( ! game_.board().move_is_cascade_safe( 0, 0 ) ) return false;
    }


    //TODO
    //This is going to require some hacking.
    //Need to cascade INSIDE the painting function?
    //But maybe that still won't work
    bool const game_over = game_.cascade( [=](){
	//std::cout << "update!" << std::endl;
	//std::this_thread::sleep_for(std::chrono::milliseconds(250));
	this->update();
	app_->processEvents();
      } );
    return game_over;
  }

  GameOverBool
  loop_autopilot( int tele_limit, int ms = 250,
    unsigned int nloop = 300 );

  void
  loop_smart_autopilot( int ms = 250 );

  GameOverBool
  skip_to_risky( int ms = 250 );

  GameOverBool
  skip_to_risky_w_recursion( int ms = 250 );

  bool
  stall_for_time( int ms = 250 );

  template< int Depth = 6 >
  bool
  run_recursive_search( int ms = 250, int ntele_desired = 8 );

  GameOverBool
  handle_move(
    sm_int const dx,
    sm_int const dy,
    bool const teleport = false,
    bool const wait = false
  ){
    if( display_cached_board_ ){
      display_cached_board_ = false;
      update();
      return false;//Don't make them make a mistake
    }

    cached_board_ = game_.board();

    bool game_over = false;
    if( wait ){
      game_over = handle_wait();
    } else if( teleport ){
      game_over = game_.teleport();
    } else {
      if( safe_mode_ ){
	if( game_.board().move_is_safe( dx, dy ) ){
	  game_over = game_.move_human( dx, dy );
	}
      } else {
	game_over = game_.move_human( dx, dy );
      }
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

    update_sidebar();

    return game_over;
  }

  void update_sidebar(){
    sidebar_->update(
      game_,
      safe_mode_,
      show_safe_moves_,
      show_ml_
    );
  }

};

template< typename GAME >
void
BoardWidget< GAME >::draw_background( Wt::WPainter & painter, int const grid_size ){

  Wt::WPen pen;
  pen.setWidth( 0.1 );
  painter.setPen( pen );

  Wt::WColor c1( 220, 220, 220 );
  Wt::WColor c2( 200, 200, 200 );

  bool use_c1 = false;

  for( sm_int i = 0; i < WIDTH; ++i ){
    for( sm_int j = 0; j < HEIGHT; ++j ){
      use_c1 = !use_c1;
      if( use_c1 ) painter.setBrush(Wt::WBrush(c1));
      else         painter.setBrush(Wt::WBrush(c2));
      painter.drawRect( i*grid_size, j*grid_size, grid_size, grid_size );
    }
    use_c1 = !use_c1;
  }

}

template< typename GAME >
void
BoardWidget< GAME >::draw_foreground(
  Board const & board,
  Wt::WPainter & painter,
  int const grid_size
){
  painter.setPen( palette_.wide_pen );
    
  bool safe_cascade_exists = false;

  if( show_safe_moves_ ){
    painter.setBrush( palette_.safety_brush );
  }

  auto const human_p = board.human_position();
  for( sm_int dx = -1; dx <= 1; ++dx ){
    if( human_p.x + dx >= WIDTH ) continue;
    for( sm_int dy = -1; dy <= 1; ++dy ){
      if( human_p.y + dy >= HEIGHT ) continue;
      if( board.move_is_cascade_safe( dx, dy ) ){
	if( show_safe_moves_ ){
	  int const i = human_p.x + dx;
	  int const j = (HEIGHT-1) - (human_p.y + dy);
	  painter.drawEllipse( i*grid_size, j*grid_size, grid_size, grid_size );
	}
	if( show_ml_ ){
	  safe_cascade_exists = true;
	}
      }
    }
  }

  if( show_ml_ and !display_cached_board_ and !safe_cascade_exists ){ // ML
    painter.setBrush( palette_.ml_brush );
    Prediction const pred = gcn_.predict( game_ );
    current_prediction_ = pred;

    {
      int const i = human_p.x + pred.dx;
      int const j = (HEIGHT-1) - (human_p.y + pred.dy);
      painter.drawEllipse( i*grid_size, j*grid_size, grid_size, grid_size );
    }

    if( show_lines_ ){
      painter.setPen( palette_.medium_pen );

      //lines
      Options options;
      options.N = 32;
      auto const elements = get_candidates( board, options );
      for( uint a = 0; a < elements.size(); ++a ){
	for( uint b = a+1; b < elements.size(); ++b ){
	  if( has_edge( elements, a, b ) ){
	    int const i1 = elements[a].pos.x;
	    int const j1 = (HEIGHT-1) - (elements[a].pos.y);
	    int const i2 = elements[b].pos.x;
	    int const j2 = (HEIGHT-1) - (elements[b].pos.y);

	    int half = grid_size / 2;

	    painter.drawLine( i1*grid_size+half, j1*grid_size+half, i2*grid_size+half, j2*grid_size+half );

	  }
	}
      }
    }
  }

  { // members

    painter.setPen( palette_.thin_pen );

    Position p;
    for( sm_int i = 0; i < WIDTH; ++i ){
      p.x = i;
      for( sm_int j = 0; j < HEIGHT; ++j ){
	p.y = (HEIGHT-1) - j;
	switch( board.cell( p ) ){
	case( Occupant::EMPTY ):
	case( Occupant::OOB ):
	  continue;
	case( Occupant::ROBOT ):
	  painter.setBrush( palette_.robot_brush );
	  break;
	case( Occupant::HUMAN ):
	  if( safe_mode_ )
	    painter.setBrush( palette_.safe_human_brush );
	  else
	    painter.setBrush( palette_.human_brush );
	  break;
	case( Occupant::FIRE ):
	  painter.setBrush( palette_.fire_brush );
	  break;
	}
	painter.drawEllipse( i*grid_size +1, j*grid_size +1, grid_size -2, grid_size -2 );
      }
    }
  }

  if( dump_training_data_ ){
    int const i = human_p.x;
    int const j = (HEIGHT-1) - human_p.y;
    if( safe_mode_ )
      painter.setBrush( palette_.safe_human_brush );
    else
      painter.setBrush( palette_.human_brush );
    painter.drawRect( i*grid_size +3, j*grid_size +3, grid_size -6, grid_size -6 );    
  }
}

template< typename GAME >
GameOverBool
BoardWidget< GAME >::loop_autopilot(
  int const teleport_limit,
  int const ms,
  unsigned int const nloop
){
  for( unsigned int i = 0; i < nloop; ++i ) {
    AutoPilotResult const apr = run_autopilot( game_, current_prediction_ );

    bool const game_over = handle_move(
      apr.dx,
      apr.dy,
      apr.apre == AutoPilotResultEnum::TELEPORT,
      apr.apre == AutoPilotResultEnum::CASCADE
    );

    this->update();
    app_->processEvents();

    if( game_over ) return true;

    if( game_.n_safe_teleports_remaining() <= teleport_limit ) break;

    std::this_thread::sleep_for (std::chrono::milliseconds( ms ));
  }

  return false;
}

template< typename GAME >
void
BoardWidget< GAME >::loop_smart_autopilot(
  int const ms
){
  while( true ){
    if( skip_to_risky_w_recursion( ms ) ) break;

    if( run_recursive_search< 6 >( ms, 7 ) ) continue;

    if( stall_for_time( ms ) ) {
      if( run_recursive_search< 6 >( ms, 7 ) ) continue;
    }

    if( game_.n_safe_teleports_remaining() > 0 ) {
      bool const game_over = handle_move( 0, 0, true );
      if( game_over ) break;      
    } else {
      bool const game_over = loop_autopilot( -1, ms, 1 );
      if( game_over ) break;
    }

  }
}


template< typename GAME >
GameOverBool
BoardWidget< GAME >::skip_to_risky(
  int const ms
){
  for( unsigned int i = 0; i < 1000; ++i ) {

    AutoPilotResult const apr =
      run_autopilot( game_, current_prediction_ );

    if( apr.apre == AutoPilotResultEnum::MOVE ){
      Board copy = game_.board();
      int const n_robots_before = copy.n_robots();
      auto const result = copy.move_human( apr.dx, apr.dy );
      if( result == MoveResult::YOU_LOSE ) break;
      if( result == MoveResult::YOU_WIN_GAME ) break;

      int const n_robots_desired = ( 15-game_.n_safe_teleports_remaining() );

      if( copy.n_robots() < n_robots_before ){
	if( copy.n_robots() < n_robots_desired ){
	  break;
	}
      }

      if( game_.n_safe_teleports_remaining() < 2 ){
	break;
      }
    }

    bool const game_over =
      handle_move(
	apr.dx,
	apr.dy,
	apr.apre == AutoPilotResultEnum::TELEPORT,
	apr.apre == AutoPilotResultEnum::CASCADE
      );

    this->update();
    app_->processEvents();

    if( game_over ) return true;

    std::this_thread::sleep_for (std::chrono::milliseconds( ms ));
  }

  return false;
}

template< typename GAME >
GameOverBool
BoardWidget< GAME >::skip_to_risky_w_recursion(
  int const ms
){
  while( true ) {

    bool const recursion_success = run_recursive_search< 4 >( ms, 7 );
    if( recursion_success ){
      bool const game_over_rec = handle_move( 0, 0, false, true );

      //TODO RAII?
      this->update();
      app_->processEvents();

      if( game_over_rec ) return true;
      continue;
    }

    AutoPilotResult const apr =
      run_autopilot( game_, current_prediction_ );

    if( apr.apre == AutoPilotResultEnum::MOVE ){
      Board copy = game_.board();
      int const n_robots_before = copy.n_robots();
      auto const result = copy.move_human( apr.dx, apr.dy );
      if( result == MoveResult::YOU_LOSE ) break;
      if( result == MoveResult::YOU_WIN_GAME ) break;

      int const n_robots_desired = ( 15-game_.n_safe_teleports_remaining() );

      if( copy.n_robots() < n_robots_before ){
	if( copy.n_robots() < n_robots_desired ){
	  break;
	}
      }

      if( game_.n_safe_teleports_remaining() < 2 ){
	break;
      }
    }

    bool const game_over =
      handle_move(
	apr.dx,
	apr.dy,
	apr.apre == AutoPilotResultEnum::TELEPORT,
	apr.apre == AutoPilotResultEnum::CASCADE
      );

    this->update();
    app_->processEvents();

    if( game_over ) return true;

    std::this_thread::sleep_for (std::chrono::milliseconds( ms ));
  }

  return false;
}

template<class GAME>
template<int Depth>
bool
BoardWidget< GAME >::run_recursive_search(
  int const ms,
  int const ntele_desired
){
  //constexpr int Depth = 6;

  int min_n_robots = ntele_desired - game_.n_safe_teleports_remaining();
  if( min_n_robots > game_.board().n_robots() ){
    min_n_robots = game_.board().n_robots();
  }

  SearchResult< Depth > const search_result =
    recursive_search_for_cascade< Depth >( game_.board() );

  if( not search_result.cascade ){
    //std::cout << "No cascade" << std::endl;
    return false;
  }

  if( search_result.nrobots_killed_cascading < min_n_robots ){
    //std::cout << "No good cascade" << std::endl;
    return false;
  }

  for( Move const m : search_result.moves ){

    if( m.nullop ) break;

    bool const game_over = handle_move( m.dx, m.dy, false, false );

    assert( not game_over );

    this->update();
    app_->processEvents();

    if( game_over ) break;

    std::this_thread::sleep_for (std::chrono::milliseconds( ms ));
  }

  return true;
}

template< typename GAME >
bool
BoardWidget< GAME >::stall_for_time(
  int const ms
){

  bool any_solution_found = false;
  bool solution_found = true;

  while( solution_found ){
    solution_found = false;
    for( int dx = -1; dx <= 1; ++dx ){
      for( int dy = -1; dy <= 1; ++dy ){
	Board copy( game_.board() );
	MoveResult const move_result = copy.move_human( dx, dy );

	if( move_result != MoveResult::CONTINUE ) break;

	solution_found = copy.n_robots() == game_.board().n_robots();

	if( solution_found ) {
	  any_solution_found = true;
	  handle_move( dx, dy, false, false );
	  break;
	}
      }
      if( solution_found ) break;
    }

    this->update();
    app_->processEvents();

    std::this_thread::sleep_for( std::chrono::milliseconds( ms ) );

  }

  return any_solution_found;
}




template< typename GAME >
void
BoardWidget< GAME >::keyDown( Wt::WKeyEvent const & e ){
  if( ignore_keys_ ){
    switch( e.charCode() ){
    case( '4' ):
    case( '=' ):
      break;
    default:
      return;
    }
  }

  ignore_keys_ = true;

  //std::cout << "KEY " << e.charCode() << std::endl;
  std::lock_guard<std::mutex> guard( move_mutex_ );

  switch( e.charCode() ){
  case( 'q' ):
  case( 'Q' ):
    if( dump_training_data_ ) logger_.log( game_, Key::Q );
    handle_move( -1,  1 );
  break;

  case( 'w' ):
  case( 'W' ):
    if( dump_training_data_ ) logger_.log( game_, Key::W );
    handle_move(  0,  1 );
  break;

  case( 'e' ):
  case( 'E' ):
    if( dump_training_data_ ) logger_.log( game_, Key::E );
    handle_move(  1,  1 );
  break;


  case( 'a' ):
  case( 'A' ):
    if( dump_training_data_ ) logger_.log( game_, Key::A );
    handle_move( -1,  0 );
  break;

  case( 's' ):
  case( 'S' ):
    if( dump_training_data_ ) logger_.log( game_, Key::S );
    handle_move(  0,  0 );
  break;

  case( 'd' ):
  case( 'D' ):
    if( dump_training_data_ ) logger_.log( game_, Key::D );
    handle_move(  1,  0 );
  break;


  case( 'z' ):
  case( 'Z' ):
    if( dump_training_data_ ) logger_.log( game_, Key::Z );
    handle_move( -1, -1 );
  break;

  case( 'x' ):
  case( 'X' ):
    if( dump_training_data_ ) logger_.log( game_, Key::X );
    handle_move(  0, -1 );
  break;

  case( 'c' ):
  case( 'C' ):
    if( dump_training_data_ ) logger_.log( game_, Key::C );
    handle_move(  1, -1 );
  break;



  case( 'p' ):
  case( 'P' ):
    if( show_ml_ ){
      AutoPilotResult const apr = run_autopilot( game_, current_prediction_ );
      handle_move( apr.dx, apr.dy,
	apr.apre == AutoPilotResultEnum::TELEPORT,
	apr.apre == AutoPilotResultEnum::CASCADE );
    }
  break;

  /*
  case( '6' ):
    run_recursive_search( 0, 8 );
  break;

  case( 'Y' ):
    run_recursive_search( 0, 0 );
  break;

  case( '7' ):
    stall_for_time( 0 );
  break;

  case( '8' ):
    if( show_ml_ ) skip_to_risky( 0 );
  break;

  case( '9' ):
    if( show_ml_ ) loop_smart_autopilot( 0 );
  break;

  case( '0' ):
    if( show_ml_ ) loop_autopilot( -1, 0 );
  break;
  */

  case( '=' ):
    {
      std::string const board_str = game_.board().get_stringified_representation();
      wt_util::SingleStringDialog::add( app_->root(), board_str );
      update();
    }
    break;

  case( '+' ):
    {
      wt_util::SinglePromptDialog::add(
	app_->root(),"Board State", "",
	[=]( std::string const & input ){
	  try{
	    game_.nonconst_board().load_from_stringified_representation( input );
	  } catch( ... ){}
	}
      );
      update();
    }
    break;


  case( 't' ):
  case( 'T' ):
    if( dump_training_data_ ) logger_.log( game_, Key::T );
    handle_move( -1, -1, true );
  break;

  case( ' ' ):
  case( ';' ):
    if( dump_training_data_ ) logger_.log( game_, Key::SPACE );
    handle_move( -1, -1, false, true );
    break;

  case( '?' ):
    toggle_cached_board();
    break;


    //MODES
  /*case( '1' ):
    safe_mode_ = !safe_mode_;
    update();
    update_sidebar();
    break;

  case( '2' ):
    show_safe_moves_ = !show_safe_moves_;
    update();	
    update_sidebar();
    break;

  case( '3' ):
    show_ml_ = !show_ml_;
    update();	
    update_sidebar();
    break;

  case( '4' ):
    show_lines_ = !show_lines_;
    update();	
    break;*/

  case( '5' ):
    dump_training_data_ = !dump_training_data_;
    update();	
    update_sidebar();
    break;

  default:
    break;
  }

  ignore_keys_ = false;
}
