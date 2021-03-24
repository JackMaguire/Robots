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
  int nrobots_killed = -1; //-1 UNLESS CASCADE
};

template< int TOTAL_DEPTH >
SearchResult< TOTAL_DEPTH >
_recursive_search(
  Board const & board,
  std::array< Move, DEPTH > & moves,
  int recursion_round //first call is 0
){

  using ResultType = SearchResult< TOTAL_DEPTH >;

  // Check for termination
  // Case 1: Max Depth
  if( recursion_round == TOTAL_DEPTH ){
    ResultType result;
    result.moves = moves;
    if( board.move_is_cascade_safe( 0, 0 ) ){
      result.cascade = true;
      result.nrobots_killed = board.n_robots();
    } else {
      result.cascade = false;
      result.nrobots_killed = -1;
    }
    return result;
  }

  // Case 2: Valid Solution
  if( board.move_is_cascade_safe( 0, 0 ) ){
    ResultType result;
    result.cascade = true;
    result.nrobots_killed = board.n_robots();
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
      
      switch( move_result ){
      case( MoveResult::YOU_LOSE ):
	continue;
      case( MoveResult::YOU_WIN_ROUND ):
      case( MoveResult::YOU_WIN_GAME ):
	break;
      case( MoveResult::CONTINUE ):
	
	break;
      }

    }
  }
}

template< int TOTAL_DEPTH >
SearchResult
recursive_search_for_cascade(
  Board const & board
){
  std::array< Move, DEPTH > moves;
  return _recursive_search( board, moves, 0 );
}
