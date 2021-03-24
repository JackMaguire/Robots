#pragma once

#include <iostream>
#include "robots.hh"

#include <vector>
#include <array>

struct Move {
  signed char dx = -2; //nullop
  signed char dy = -2; //nullop
  bool nullop = false;
};

template< int DEPTH >
struct SearchResult {
  //path
  std::array< Move, DEPTH > moves;
  
  //outcome
  bool cascade = false;
  int nrobots_killed = -1; //-1 UNLESS CASCADE
};

template< int TOTAL_DEPTH >
SearchResult
_recursive_search(
  Board const & board,
  std::array< Move, DEPTH > & moves,
  int recursion_round
){
  if( recursion_round == TOTAL_DEPTH ){
    SearchResult result;
    if( board.move_is_cascade_safe( 0, 0 ) ){
      result.cascade = true;
      result.nrobots_killed = board.n_robots();
    } else {
      result.cascade = false;
      result.nrobots_killed = board.n_robots();
    }
    return result;
  }

  for( int dx = -1; dx <= 1; ++dx ){
    for( int dy = -1; dy <= 1; ++dy ){
      Board copy = board;
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
