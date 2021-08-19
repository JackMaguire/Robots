// g++ robots.cc -std=c++2a -c -o robots.so
// g++ robots.cc -std=c++2a -c -o robots.so -Wall -pedantic -Wshadow

#pragma once

//#define MUTE

#include <array>
#include <vector>
#include <set>

#include <cstdlib> //rand()
#include <random>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <assert.h>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <math.h> //sqrt

#define USE_BOOST_SMALL_VEC //benchmarked - slightly faster with N = MAX_ROBOT / 10

#ifdef USE_BOOST_SMALL_VEC
#include <boost/container/small_vector.hpp>
#endif


using GameOverBool = bool;

// signed char fails for some stupid reason
using sm_int = signed short int; //char; //-127 to 127

#define GO_HUMAN_SPEED true

//using uint = unsigned int;
//using luint = long unsigned int;

constexpr sm_int WIDTH = 45;
constexpr sm_int HEIGHT = 30;

constexpr int MAX_N_ROUNDS = 66;
constexpr int MAX_N_ROBOTS = MAX_N_ROUNDS * 10;

enum class Occupant : unsigned char
{
 EMPTY = 0,
 ROBOT,
 HUMAN,
 FIRE,
 OOB
};

enum class MoveResult : unsigned char
{
 CONTINUE = 0,
 YOU_LOSE = 1,
 YOU_WIN_ROUND = 2,
 YOU_WIN_GAME = 3
};


struct Position {
  
  bool operator==( Position const & o ) const {
    return x == o.x && y == o.y;
  }

  bool operator!=( Position const & o ) const {
    return ! ( *this == o );
  }

  sm_int x;
  sm_int y;

  Position operator+( Position const & o ) const {
    Position p;
    p.x = x + o.x;
    p.y = y + o.y;
    return p;
  }

  Position operator-( Position const & o ) const {
    Position p;
    p.x = x - o.x;
    p.y = y - o.y;
    return p;
  }

  float distance( Position const & o ) const {
    Position diff = (*this) - o;
    return sqrt( diff.x*diff.x + diff.y*diff.y );
  }

};

namespace {

constexpr Position STARTING_POSITION({ 23, 15 });

int nrobots_per_round( int round ){
  return round * 10;
}

sm_int random_x(){
  return rand() % WIDTH;
}

sm_int random_y(){
  return rand() % HEIGHT;
}

class Board {
public:

#ifdef USE_BOOST_SMALL_VEC
  using PositionVec = boost::container::small_vector< Position, MAX_N_ROBOTS >;
#else
  using PositionVec = std::vector< Position >;
#endif

  Board(){
    srand(time(NULL));
    init( 1 );
  }

  Board( std::string const & stringified_rep ){
    load_from_stringified_representation( stringified_rep );
  }

  void clear_board();

  //should this be const?
  Position
  find_open_space( bool const allow_robot_movement = true );

  bool
  cell_is_safe_for_teleport( Position const p ) const;

  MoveResult
  teleport( bool const safe );

  void init( int const round );

  Occupant & cell( Position const & p ){
    return cells_[ p.x ][ p.y ];
  }

  Occupant const & cell( Position const & p ) const {
    return cells_[ p.x ][ p.y ];
  }

  Occupant & cell( int const x, int const y ) {
    return cells_[ x ][ y ];
  }

  Occupant const & cell( int const x, int const y ) const {
    return cells_[ x ][ y ];
  }

  MoveResult
  move_robots_1_step( bool const human_is_safe = false );

  int n_robots() const {
    return robot_positions_.size();
  }

  MoveResult
  move_human( sm_int const dx, sm_int const dy );

  bool
  move_is_safe( sm_int const dx, sm_int const dy ) const;

  bool
  move_is_cascade_safe( sm_int const dx, sm_int const dy ) const;

  bool
  move_is_cascade_safe( sm_int const dx, sm_int const dy, int & n_robots_remaining ) const;

  __attribute__((unused))  
  std::string
  get_stringified_representation() const;

  void
  load_from_stringified_representation( std::string const & str );

  std::string
  get_safe_moves() const;

  Position const & human_position() const {
    return human_position_;
  }

  static bool position_is_in_bounds( Position p ){
    return p.x >= 0 && p.x < WIDTH && p.y >= 0 && p.y < HEIGHT;
  }

  PositionVec const & robots() const {
    return robot_positions_;
  }

private:
  std::array< std::array< Occupant, HEIGHT >, WIDTH > cells_;

  PositionVec robot_positions_;
  Position human_position_;
};
}

struct ForecastResults {
  bool legal = false;
  bool cascade_safe = false;
  unsigned int robots_killed = 0;
};

ForecastResults
forcast_move( Board const & board, sm_int const dx, sm_int const dy ){
  ForecastResults results;
  Board copy = board;

  //std::cout << board.n_robots() << " " << copy.n_robots() << std::endl;

  MoveResult move_result = copy.move_human( dx, dy );
  //std::cout << "    " << int( move_result ) << std::endl;

  if( move_result == MoveResult::YOU_LOSE ) return results;  

  results.legal = true;
  results.robots_killed = board.n_robots() - copy.n_robots();

  //Check for cascade safety
  while ( move_result == MoveResult::CONTINUE ){
    move_result = copy.move_human( 0, 0 );
  }
  results.cascade_safe = (move_result != MoveResult::YOU_LOSE);

  return results;
}

__attribute__((unused))
std::array< std::array< ForecastResults, 3 >, 3 >
forcast_all_moves( Board const & board ) {
  std::array< std::array< ForecastResults, 3 >, 3 > forecasts;

  Position const human_position = board.human_position();

  for( sm_int dx = -1; dx <= 1; ++dx ){
    if( human_position.x+dx < 0 || human_position.x+dx >= WIDTH ) continue;

    for( sm_int dy = -1; dy <= 1; ++dy ){
      if( human_position.y+dy < 0 || human_position.y+dy >= HEIGHT ) continue;

      forecasts[ dx+1 ][ dy+1 ] = forcast_move( board, dx, dy );
    }
  }

  return forecasts;
}


struct NullVisualizer {
  static void show( Board const & ){}
};

template< typename Visualizer = NullVisualizer, bool go_slow = GO_HUMAN_SPEED, int sleepsize = 500 >
class RobotsGame {
public:
  RobotsGame( int const round = 1, int const tele = 0 ) :
    round_( round ),
    n_safe_teleports_remaining_( tele )
  {
    board_.init( round_ );

    long int expected_score = 0;
    for( int r = 1; r <= round_; ++r ){
      expected_score += r * 10;
    }
    score_ = expected_score;

    Visualizer::show( board_ );
  }

  void
  new_round(){
#ifndef MUTE
    std::cout << "You have " << n_safe_teleports_remaining_ << " safe teleports remaining" << std::endl;
    std::cout << "Score: " << score_ << std::endl;
#endif

    long int expected_score = 0;
    for( int r = 1; r <= round_; ++r ){
      expected_score += r * 10;
    }
#ifndef MUTE
    if( score_ != expected_score ){
      std::cout << "Expected score is " << expected_score << std::endl;
    }
#endif

    if( round_ == MAX_N_ROUNDS ){
      latest_result_ = MoveResult::YOU_WIN_GAME;
#ifndef MUTE
      std::cout << "YOU WIN!" << std::endl;
#endif
    } else {
      board_.init( ++round_ );
      if( GO_HUMAN_SPEED ){
	std::this_thread::sleep_for (std::chrono::milliseconds(sleepsize));
      }
      Visualizer::show( board_ );    
    }
  }

  GameOverBool
  old_cascade(){
    //TODO call lower function from this one
    int const n_robots_start = board_.n_robots();

    latest_result_ = MoveResult::CONTINUE;
    while( latest_result_ == MoveResult::CONTINUE ){
      latest_result_ = board_.move_robots_1_step();
      Visualizer::show( board_ );
      if( go_slow ){
	std::this_thread::sleep_for (std::chrono::milliseconds(sleepsize));
      }
    }

    if( latest_result_ == MoveResult::YOU_WIN_ROUND ){
      score_ += n_robots_start;
      n_safe_teleports_remaining_ += n_robots_start;
      if( n_safe_teleports_remaining_ > 10 ) n_safe_teleports_remaining_ = 10;
      new_round();
    }

    //std::cout << "result: " << int( result ) << std::endl;
    return latest_result_ == MoveResult::YOU_LOSE || latest_result_ == MoveResult::YOU_WIN_GAME;
  }

  GameOverBool
  cascade(){  
    auto && show = [=](){ Visualizer::show( board_ ); };
    return cascade( show );
  }

  template< typename T >
  GameOverBool
  cascade( T && updater ){
    int const n_robots_start = board_.n_robots();

    latest_result_ = MoveResult::CONTINUE;
    while( latest_result_ == MoveResult::CONTINUE ){
      latest_result_ = board_.move_robots_1_step();
      updater();
      if( go_slow ){
	std::this_thread::sleep_for (std::chrono::milliseconds(sleepsize));
      }
    }

    if( latest_result_ == MoveResult::YOU_WIN_ROUND ){
      score_ += n_robots_start;
      n_safe_teleports_remaining_ += n_robots_start;
      if( n_safe_teleports_remaining_ > 10 ) n_safe_teleports_remaining_ = 10;
      new_round();
    }

    //std::cout << "result: " << int( result ) << std::endl;
    return latest_result_ == MoveResult::YOU_LOSE || latest_result_ == MoveResult::YOU_WIN_GAME;
  }

  //true if game over
  GameOverBool
  move_human( sm_int const dx, sm_int const dy ){
    int const n_robots_start = board_.n_robots();

    latest_result_ = board_.move_human( dx, dy );
    Visualizer::show( board_ );

    score_ += ( n_robots_start - board_.n_robots() );

    //std::cout << "result: " << int( result ) << std::endl;

    if( latest_result_ == MoveResult::YOU_WIN_ROUND ){
      new_round();
    }

    return latest_result_ == MoveResult::YOU_LOSE || latest_result_ == MoveResult::YOU_WIN_GAME;
  }

private:
  void danger_tele(){
      latest_result_ = board_.teleport( false );
      Visualizer::show( board_ );
#ifndef MUTE
      std::cout << "You have 0 safe teleports remaining" << std::endl;
#endif
      if( latest_result_ == MoveResult::CONTINUE ){
	assert( board_.n_robots() > 0 );
      }
  }

  void safe_tele(){
    latest_result_ = board_.teleport( true );
    Visualizer::show( board_ );
    --n_safe_teleports_remaining_;
#ifndef MUTE
    if( latest_result_ == MoveResult::YOU_LOSE ){
      std::cout << "That loss should not have counted!" << std::endl;
    }
    std::cout << "You have " << n_safe_teleports_remaining_ << " safe teleports remaining" << std::endl;
#endif
    // assert( ! game_over );
  }

public:  
  GameOverBool
  teleport(){
    int const n_robots_start = board_.n_robots();
    
    if( n_safe_teleports_remaining_ == 0 ) danger_tele();
    else safe_tele();

    score_ += ( n_robots_start - board_.n_robots() );
    
    if( latest_result_ == MoveResult::YOU_WIN_ROUND ){
      new_round();
    }
    else if( latest_result_ == MoveResult::CONTINUE ){
      assert( board_.n_robots() > 0 );
    }

    GameOverBool const game_over =
      (latest_result_ == MoveResult::YOU_LOSE || latest_result_ == MoveResult::YOU_WIN_GAME);
    return game_over;
  }

  Board const & board() const {
    return board_;
  }

  Board & nonconst_board() {
    return board_;
  }

  int n_safe_teleports_remaining() const {
    return n_safe_teleports_remaining_;
  }

  int round() const {
    return round_;
  }

  void
  load_from_stringified_representation(
    std::string const & str,
    int const round,
    int const n_safe_teleports_remaining,
    long int const score
  ) {
    board_.load_from_stringified_representation( str );
    round_ = round;
    n_safe_teleports_remaining_ = n_safe_teleports_remaining;
    score_ = score;
  }

  MoveResult latest_result() const {
    return latest_result_;
  }

  long int score() const {
    return score_;
  }

  void reset(){
    board_ = Board();
    round_ = 1;
    n_safe_teleports_remaining_ = 0;
    score_ = 0;
    latest_result_ = MoveResult::CONTINUE;
    new_round();
  }
  
private:
  Board board_;

  int round_;
  int n_safe_teleports_remaining_;

  long int score_ = 0;
  
  MoveResult latest_result_ = MoveResult::CONTINUE;
};


void Board::clear_board(){
  for( std::array< Occupant, HEIGHT > & arr : cells_ ){
    for( Occupant & o : arr ){
      o = Occupant::EMPTY;
    }
  }
}

Position
Board::find_open_space( bool const allow_robot_movement ){
  if( robot_positions_.size() < 100 ){
    Position openp;
    do {
      openp.x = random_x();
      openp.y = random_y();
    } while( cell( openp ) != Occupant::EMPTY );
    return openp;
  } else {
    //This can be very constexpr
    std::vector< Position > empty_positions;
    empty_positions.reserve( (HEIGHT*WIDTH) - 1 - robot_positions_.size() );
    for( sm_int w = 0; w < WIDTH; ++w ){
      for( sm_int h = 0; h < HEIGHT; ++h ){
	Position const p = { w, h };
	if( allow_robot_movement ){
	  if( cell_is_safe_for_teleport( p ) ){
	    empty_positions.emplace_back( p );
	  }
	} else {
	  if( cell( p ) == Occupant::EMPTY ){
	    empty_positions.emplace_back( p );
	  }
	}
      }
    }

    if( empty_positions.empty() ){
      if( allow_robot_movement ){
#ifndef MUTE
	std::cout << "No Safe Positions! Trying Fallback Plan" << std::endl;
#endif
	return find_open_space( false );
      } else {
#ifndef MUTE
	std::cout << "No safe positions even in fallback plan!!" << std::endl;
#endif
	//This is very unexpected, don't know how to handle it
      }
    }

    std::random_device rd;
    std::mt19937 g( rd() );
    std::shuffle( empty_positions.begin(), empty_positions.end(), g );
    return empty_positions[ 0 ];
    //TODO Just pick random index instead of shuffling
  }
}

bool
Board::cell_is_safe_for_teleport( Position const p ) const {
  for( sm_int x = p.x - 1; x <= p.x + 1; ++x ){
    if( x < 0 || x >= WIDTH) continue;
    for( sm_int y = p.y - 1; y <= p.y + 1; ++y ){
      if( y < 0 || y >= HEIGHT) continue;
      if( cell( Position({ x, y }) ) != Occupant::EMPTY ) return false;
    }
  }

  return cell( p ) == Occupant::EMPTY;
}

MoveResult
Board::teleport( bool const safe ){
  cell( human_position_ ) = Occupant::EMPTY;

  if( safe ){
    human_position_ = find_open_space();
  } else {
    human_position_.x = random_x();
    human_position_.y = random_y();
  }

  cell( human_position_ ) = Occupant::HUMAN;
  return move_robots_1_step( safe );
}

void
Board::init( int const round ){
  //RESET
  clear_board();

  //HUMAN
  human_position_ = STARTING_POSITION;
  cell( human_position_ ) = Occupant::HUMAN;
    
  //ROBOTS
  robot_positions_.resize( nrobots_per_round( round ) );
  if( robot_positions_.size() < 100 ){
    //Use different algorithm if the robot count is << the number of cells
    for( Position & robot : robot_positions_ ){
      do {
	robot.x = random_x();
	robot.y = random_y();
      } while( cell( robot ) != Occupant::EMPTY );

      cell( robot ) = Occupant::ROBOT;	
    }
  } else {
    //This can be very constexpr
    std::vector< Position > empty_positions;
    empty_positions.reserve( (HEIGHT*WIDTH) - 1 );
    for( sm_int w = 0; w < WIDTH; ++w ){
      for( sm_int h = 0; h < HEIGHT; ++h ){
	Position const p = { w, h };
	if( p != STARTING_POSITION ){
	  empty_positions.emplace_back( p );
	}
      }
    }

    std::random_device rd;
    std::mt19937 g( rd() );
    std::shuffle( empty_positions.begin(), empty_positions.end(), g );

    for( unsigned int i = 0; i < robot_positions_.size(); ++i ){
      robot_positions_[ i ] = empty_positions[ i ];
      cell( robot_positions_[ i ] ) = Occupant::ROBOT;
    }
  }
}

MoveResult
Board::move_robots_1_step(
  bool const human_is_safe
){
  //Clear robots from map
  for( sm_int w = 0; w < WIDTH; ++w ){
    for( sm_int h = 0; h < HEIGHT; ++h ){
      if( cells_[ w ][ h ] == Occupant::ROBOT ){
	cells_[ w ][ h ] = Occupant::EMPTY;
      }
    }
  }

  //Some robots will be deleted if they run into each other or into fire
  std::set< int > robots_to_delete;
 
  //keep temporary track of robots just in case they clash
  //elements are indices in robot_positions_
  std::array< std::array< sm_int, HEIGHT >, WIDTH > robot_indices;

  for( unsigned int r = 0; r < robot_positions_.size(); ++r ){
    Position & pos = robot_positions_[ r ];

    if( human_position_.x < pos.x ) pos.x -= 1;
    else if( human_position_.x > pos.x ) pos.x += 1;

    if( human_position_.y < pos.y ) pos.y -= 1;
    else if( human_position_.y > pos.y ) pos.y += 1;

    if( human_is_safe && pos == human_position_ ){
      //This is rare, but just have the robot take one step to the left or right
      if( pos.x == 0 ) ++pos.x;//Don't go out of bounds
      else --pos.x;
    }

    switch( cell( pos ) ){
    case Occupant::EMPTY:
      robot_indices[ pos.x ][ pos.y ] = r;
      cell( pos ) = Occupant::ROBOT;
      break;
    case Occupant::ROBOT:
      {
	int const other_robot_ind = robot_indices[ pos.x ][ pos.y ];
	robots_to_delete.insert( other_robot_ind );
      }
      robots_to_delete.insert( r );
      cell( pos ) = Occupant::FIRE;
      break;
    case Occupant::HUMAN:
      return MoveResult::YOU_LOSE;
      //break;
    case Occupant::FIRE:
      robots_to_delete.insert( r );
      break;
    case Occupant::OOB:
      break;
    }
  }// for int r

  for( auto iter = robots_to_delete.rbegin(), end = robots_to_delete.rend();
       iter != end; ++iter ){
    robot_positions_.erase( robot_positions_.begin() + (*iter) );
  }

  if( robot_positions_.size() == 0 ){
    return MoveResult::YOU_WIN_ROUND;
  }

  return MoveResult::CONTINUE;
} //move_robots_one_step

MoveResult
Board::move_human( sm_int const dx, sm_int const dy ) {
  cell( human_position_ ) = Occupant::EMPTY;

  human_position_.x += dx;
  if( human_position_.x < 0 ) human_position_.x = 0;
  if( human_position_.x >= WIDTH ) human_position_.x -= 1;

  human_position_.y += dy;
  if( human_position_.y < 0 ) human_position_.y = 0;
  if( human_position_.y >= HEIGHT ) human_position_.y -= 1;

  if( cell( human_position_ ) == Occupant::FIRE ) return MoveResult::YOU_LOSE;
  if( cell( human_position_ ) == Occupant::ROBOT ) return MoveResult::YOU_LOSE;

  cell( human_position_ ) = Occupant::HUMAN;

  return move_robots_1_step();
}

bool
Board::move_is_safe( sm_int const dx, sm_int const dy ) const {
  Board copy = (*this);
  MoveResult const result = copy.move_human( dx, dy );
  return result != MoveResult::YOU_LOSE;
}

bool
Board::move_is_cascade_safe( sm_int const dx, sm_int const dy ) const {
  Board copy = (*this);
  MoveResult result = copy.move_human( dx, dy );
  while ( result == MoveResult::CONTINUE ){
    result = copy.move_human( 0, 0 );
  }
  return result != MoveResult::YOU_LOSE;
}

bool
Board::move_is_cascade_safe(
  sm_int const dx,
  sm_int const dy,
  int & nremaining
) const {
  Board copy = (*this);
  MoveResult result = copy.move_human( dx, dy );
  while ( result == MoveResult::CONTINUE ){
    result = copy.move_human( 0, 0 );
  }
  nremaining = copy.n_robots();
  return result != MoveResult::YOU_LOSE;
}


std::string
Board::get_stringified_representation() const {
  std::stringstream ss;
  for( std::array< Occupant, HEIGHT > const & arr : cells_ ){
    for( Occupant const & o : arr ){
      ss << int( o );
    }
  }
  return ss.str();
}

void
Board::load_from_stringified_representation( std::string const & str ) {
  robot_positions_.clear();

  int index = 0;
  Position pos({0,0});
  for( pos.x = 0; pos.x < WIDTH; ++pos.x ){
    for( pos.y = 0; pos.y < HEIGHT; ++pos.y ){
      char const val = str[ index ]; ++index;
      Occupant const state = Occupant(std::stoi(std::string(1,val)));
      cell( pos ) = state;

      //std::cout << "state: " << int(state) << std::endl;
      //std::cout << "state: " << int(state) << std::endl;

      switch( state ){
      case( Occupant::ROBOT ):
	//std::cout << "LOADING ROBOT" << std::endl;
	robot_positions_.push_back( pos );
	break;
      case( Occupant::HUMAN ):
	human_position_ = pos;
	break;
      case( Occupant::EMPTY ):
      case( Occupant::FIRE ):
      case( Occupant::OOB ):
	break;
      }
    }//y
  }//x
}

__attribute__((unused))
std::string
Board::get_safe_moves() const {
  std::stringstream ss;
  for( sm_int dx = -1; dx < 2; ++dx )
    for( sm_int dy = -1; dy < 2; ++dy )
      if( move_is_cascade_safe( dx, dy ) ){
	ss << "1";
      } else {
	ss << "0";
      }
  return ss.str();
}

//int main(){};
