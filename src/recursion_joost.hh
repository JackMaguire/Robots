#pragma once

#include <iostream>
#include "robots.hh"

#include <vector>
#include <array>

#include <joost/dynamic_depth_first_recursion.hh>

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

      if( other.cascade ) return nrobots_killed_cascading > other.nrobots_killed_cascading;
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
  int const min_sufficient_robots_killed,
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

  bool min_sufficient_robots_killed_met = false;

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

	int const inner_suff_cutoff = std::min( min_sufficient_robots_killed, copy.n_robots() );

	ResultType const best_subresult =
	  _recursive_search< TOTAL_DEPTH >(
	    copy,
	    inner_suff_cutoff, //min_sufficient_robots_killed,
	    result.moves,
	    min_n_robots,
	    recursion_round + 1
	  );

	result = best_subresult;
	break;
      }

      if( result.is_better_than( best_result ) ){
	best_result = result;
      }

      min_sufficient_robots_killed_met = best_result.nrobots_killed_cascading >= min_sufficient_robots_killed;

      if( min_sufficient_robots_killed_met ) break;
    }
    if( min_sufficient_robots_killed_met ) break;
  }

  return best_result;
}

template< int MAX_DEPTH >
SearchResult< MAX_DEPTH >
recursive_search_for_cascade(
  Board const & board,
  int const min_sufficient_robots_killed = 10,
  int const min_n_robots = 0
){

  using StateType = Board;
  using OutcomeType = SearchResult< MAX_DEPTH >;
  using DDFR = joost::DDFRCache< StateType, OutcomeType, 9 >;

  DDFR ddfr( board );

  struct Forecaster {
    //TODO
  };

  struct OutcomeRanker {
    //TODO
  };

  struct StopEarlyFailure {
    //TODO
  };

  struct StopEarlySuccess {
    //TODO
  };

  StopEarlyFailure const sef( min_n_robots );
  StopEarlySuccess const ses( min_sufficient_robots_killed );

  joost::RecursionSolution< 9 > const solution =
    ddfr.sample_to_depth_dynamic< Forecaster, OutcomeRanker, StopEarlyFailure, StopEarlySuccess >( MAX_DEPTH, sef, ses );

  return *solution.outcome;
}
