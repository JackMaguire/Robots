#pragma once

#include <iostream>
#include "robots.hh"

#include <vector>
#include <array>

struct Move {
  signed char dx = -2; //nullop
  signed char dy = -2; //nullop
  bool nullop = true;
};

template< int DEPTH >
struct SearchResult {
  //path
  std::array< Move, DEPTH > moves; //default construct to nullop
  
  //outcome
  bool cascade = false;
  int nrobots_killed_cascading = -1; //-1 UNLESS CASCADE


  bool
  is_better_than( SearchResult< DEPTH > const & other ) const {
    
    if( cascade ){

      if( other.cascade ) return nrobots_killed_cascading < other.nrobots_killed_cascading;
      else                return true;

    } else {

      if( other.cascade ) return false;
      else                return false; //both are losers

    }    
  }
};

template< int TOTAL_DEPTH >
SearchResult< TOTAL_DEPTH >
_recursive_search(
  Board const & board,
  std::array< Move, TOTAL_DEPTH > const & moves,
  int const min_n_robots,
  int const recursion_round //first call is 0
){

  using ResultType = SearchResult< TOTAL_DEPTH >;

  // Check for termination

  // Case 1: Valid Solution
  if( board.move_is_cascade_safe( 0, 0 ) ){
    ResultType result;
    result.moves = moves;
    result.cascade = true;
    result.nrobots_killed_cascading = board.n_robots();
    return result;
  }

  if(
    (recursion_round == TOTAL_DEPTH) // Case 2: Max Depth
    or
    (board.n_robots() < min_n_robots) // Case 3: Not Enough Robots
  ){
    ResultType result;
    result.moves = moves;
    result.cascade = false;
    result.nrobots_killed_cascading = -1;
    return result;
  }

  // Propogate
  ResultType best_result;
  Position hpos = board.human_position();

  for( int dx = -1; dx <= 1; ++dx ){
    for( int dy = -1; dy <= 1; ++dy ){
      switch( board.cell( hpos.x + dx, hpos.y + dy ) ){
      case( Occupant::ROBOT ):
      case( Occupant::FIRE ):
      case( Occupant::OOB ):
	continue;
      case( Occupant::EMPTY ):
      case( Occupant::HUMAN ):
	break;
      }

      Board copy( board );
      MoveResult const move_result = copy.move_human( dx, dy );
      ResultType result;
      result.moves = moves;

      switch( move_result ){
      case( MoveResult::YOU_LOSE ):
	continue;

      case( MoveResult::YOU_WIN_ROUND ):
      case( MoveResult::YOU_WIN_GAME ):
	result.moves[ recursion_round ].dx = dx;
	result.moves[ recursion_round ].dy = dy;
	result.moves[ recursion_round ].nullop = false;
	result.cascade = true; //technically...
	result.nrobots_killed_cascading = 0;
	break;

      case( MoveResult::CONTINUE ):
	result.moves[ recursion_round ].dx = dx;
	result.moves[ recursion_round ].dy = dy;
	result.moves[ recursion_round ].nullop = false;	
	ResultType const best_subresult =
	  _recursive_search< TOTAL_DEPTH >( copy, result.moves, min_n_robots, recursion_round + 1 );

	result = best_subresult;
	break;
      }

      if( result.is_better_than( best_result ) ){
	best_result = result;
      }

    }
  }

  return best_result;
}

template< int MAX_DEPTH >
SearchResult< MAX_DEPTH >
recursive_search_for_cascade(
  Board const & board,
  int const min_n_robots = 0
){
  std::array< Move, MAX_DEPTH > moves;
  return _recursive_search< MAX_DEPTH >( board, moves, min_n_robots, 0 );
}
