// g++ robots.cc -std=c++2a -c -o robots.so
// g++ robots.cc -std=c++2a -c -o robots.so -Wall -pedantic -Wshadow

#include <array>
#include <vector>
#include <set>

#include <cstdlib> //rand()
#include <random>
#include <iostream>
#include <algorithm>

//using uint = unsigned int;
//using luint = long unsigned int;

constexpr int WIDTH = 32;
constexpr int HEIGHT = 24;

//constexpr int MAX_N_ROUNDS = 66;

enum class Occupant
{
 EMPTY = 0,
 ROBOT,
 HUMAN,
 FIRE
};

enum class MoveResult
{
 CONTINUE,
 YOU_LOSE,
 YOU_WIN_ROUND,
 YOU_WIN_GAME
};


struct Position {

  // auto operator<=>(const Position&) const = default;

  bool operator==( Position const & o ) const {
    return x == o.x && y == o.y;
  }

  bool operator!=( Position const & o ) const {
    return ! ( *this == o );
  }

  int x;
  int y;

};

constexpr Position STARTING_POSITION({ WIDTH / 2, HEIGHT / 2 });

int nrobots_per_round( int round ){
  return round * 10; //TODO
}

int random_x(){
  return rand() % WIDTH;
}

int random_y(){
  return rand() % HEIGHT;
}

class Board {
  Board(){
    init( 1 );
  }

  void clear_board(){
    for( std::array< Occupant, WIDTH > & arr : cells_ ){
      for( Occupant & o : arr ){
	o = Occupant::EMPTY;
      }
    }
  }

  void init( int const round ){
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
      for( int w = 0; w < WIDTH; ++w ){
	for( int h = 0; h < HEIGHT; ++h ){
	  Position const p = { w, h };
	  if( p != STARTING_POSITION ){
	    empty_positions.emplace_back( p );
	  }
	}
      }

      std::random_device rd;
      std::mt19937 g( rd() );
      std::shuffle( empty_positions.begin(), empty_positions.end(), g );

      for( int i = 0; i < robot_positions_.size(); ++i ){
	robot_positions_[ i ] = empty_positions[ i ];
	cell( robot_positions_[ i ] ) = Occupant::ROBOT;
      }
    }
  }

  Occupant & cell( Position const & p ){
    return cells_[ p.x ][ p.y ];
  }

  Occupant const & cell( Position const & p ) const {
    return cells_[ p.x ][ p.y ];
  }

  MoveResult
  move_robots_1_step(){
    //Clear robots from map
    for( int w = 0; w < WIDTH; ++w ){
      for( int h = 0; h < HEIGHT; ++h ){
	if( cells_[ w ][ h ] == Occupant::ROBOT ){
	  cells_[ w ][ h ] = Occupant::EMPTY;
	}
      }
    }

    //Some robots will be deleted if they run into each other or into fire
    std::set< int > robots_to_delete;
 
    //keep temporary track of robots just in case they clash
    //elements are indices in robot_positions_
    std::array< std::array< int , WIDTH >, HEIGHT > robot_indices;

    for( int r = 0; r < robot_positions_.size(); ++r ){
      Position & pos = robot_positions_[ r ];

      if( human_position_.x < pos.x ) pos.x -= 1;
      else if( human_position_.x > pos.x ) pos.x += 1;

      if( human_position_.y < pos.y ) pos.y -= 1;
      else if( human_position_.y > pos.y ) pos.y += 1;

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
      }
    }// for int r

    for( auto iter = robots_to_delete.rbegin(), end = robots_to_delete.rend();
	 iter != end; ++iter ){
      robot_positions_.erase( robot_positions_.begin() + (*iter) );
    }

    return MoveResult::CONTINUE;
  } //move_robots_one_step

  int n_robots() const {
    return robot_positions_.size();
  }

  void move_human( int dx, int dy ) {
    
  }

private:
  std::array< std::array< Occupant, WIDTH >, HEIGHT > cells_;
  //TODO hold pointers to robots?

  Position human_position_;
  std::vector< Position > robot_positions_;

};

/*
class RobotsGame {

  int
  cascade(){
    int const n_robots_start = board_.n_robots();

    MoveResult result = MoveResult::CONTINUE;
    while( result == MoveResult::CONTINUE ){
      board_.move_robots_1_step();
      //TODO cache state for visualization?
      //Maybe this should be moved to the python layer
    }

    //TODO just do this in python
  }

private:
  Board board_;

  int round_ = 1;
  int n_safe_teleports_remaining_ = 0;
};
*/

//int main(){};
