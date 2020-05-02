// g++ robots.cc -std=c++2a -c -o robots.so

#include <array>
#include <vector>
#include <cstdlib> //rand()
#include <random>
#include <iostream>
#include <algorithm>

using uint = unsigned int;

constexpr uint WIDTH = 32;
constexpr uint HEIGHT = 24;

constexpr uint MAX_N_ROUNDS = 66;

enum class Occupant
{
 EMPTY = 0,
 ROBOT,
 HUMAN
};

struct Position {

  bool operator==( Position const & o ) const {
    return x == o.x && y == o.y;
  }

  bool operator!=( Position const & o ) const {
    return ! ( *this == o );
  }

  uint x;
  uint y;
};
constexpr Position STARTING_POSITION({ WIDTH / 2, HEIGHT / 2 });

uint nrobots_per_round( uint round ){
  return round * 10; //TODO
}

uint random_x(){
  return rand() % WIDTH;
}

uint random_y(){
  return rand() % HEIGHT;
}

class Board {
  Board(){
    init();
  }

  void clear_board(){
    for( std::array< Occupant, WIDTH > & arr : cells_ ){
      for( Occupant & o : arr ){
	o = Occupant::EMPTY;
      }
    }
  }

  void init(){
    //RESET
    clear_board();

    //HUMAN
    human_position_ = STARTING_POSITION;
    cell( human_position_ ) = Occupant::HUMAN;
    
    //ROBOTS
    robot_positions_.resize( nrobots_per_round( round_ ) );
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
      for( uint w = 0; w < WIDTH; ++w ){
	for( uint h = 0; h < HEIGHT; ++h ){
	  Position const p = { w, h };
	  if( p != STARTING_POSITION ){
	    empty_positions.emplace_back( p );
	  }
	}
      }

      std::random_device rd;
      std::mt19937 g( rd() );
      std::shuffle( empty_positions.begin(), empty_positions.end(), g );

      for( uint i = 0; i < robot_positions_.size(); ++i ){
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

private:
  std::array< std::array< Occupant, WIDTH >, HEIGHT > cells_;

  Position human_position_;
  std::vector< Position > robot_positions_;

  uint round_ = 1;
};

class RobotsGame {
  
};

//int main(){};
