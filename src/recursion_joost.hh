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

template< int MAX_DEPTH >
SearchResult< MAX_DEPTH >
recursive_search_for_cascade(
  Board const & board,
  int const min_sufficient_robots_killed = 10,
  int const min_n_robots = 0
){

  using StateType = Board;
  //using OutcomeType = SearchResult< MAX_DEPTH >;

  struct OutcomeType {
    MoveResult move_result;
    bool can_cascade = false;
    int nrobots_killed_in_cascade = -1;
  };

  using DDFR = joost::DDFRCache< StateType, OutcomeType, 9 >;

  DDFR ddfr( board );

  struct Forecaster {
    static
    void
    forecast(
      StateType const & incoming_board,
      unsigned int const move,
      StateType & outgoing_board,
      OutcomeType & outcome
    ){
      OutcomeType outcome;

      int dx, dy;
      switch( move ){
      case 0: case 1: case 2: dx = -1; break;
      case 3: case 4: case 5: dx = 0;  break;
      case 6: case 7: case 8: dx = 1;  break;
      }
      switch( move ){
      case 0: case 3: case 6: dy = -1; break;
      case 1: case 4: case 7: dy = 0;  break;
      case 2: case 5: case 8: dy = 1;  break;
      }

      outgoing_board = incoming_board;
      outcome.move_result = outgoing_board.move_human( dx, dy );

      switch( outcome.move_result ){
      case( MoveResult::YOU_LOSE ): return;

      case( MoveResult::YOU_WIN_ROUND ):
      case( MoveResult::YOU_WIN_GAME ):
	outcome.can_cascade = true;
      outcome.nrobots_killed_in_cascade = 0;
      break;

      case( MoveResult::CONTINUE ):
	//TODO check for cascade?
      }
    }
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
