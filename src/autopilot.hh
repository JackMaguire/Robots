#pragma once

#include <iostream>
#include "robots.hh"
#include "gcn.hh"

enum class AutoPilotResultEnum {
  MOVE,
  CASCADE,
  TELEPORT
};

struct AutoPilotResult {
  AutoPilotResultEnum apre;
  int dx;
  int dy;
};

template< typename GAME >
AutoPilotResult
run_autopilot(
  GAME const & game,
  Prediction const & pred
){
  auto const & board = game.board();
  
  AutoPilotResult apr;

  if( board.move_is_cascade_safe( 0,0 ) ){
    apr.apre = AutoPilotResultEnum::CASCADE;
    return apr;
  }

  int n_robots_remaining_BEST = -1;
  apr.dx = pred.dx;
  apr.dy = pred.dy;

  bool safe_move_exists = false;

  auto const human_p = board.human_position();
  for( int dx = -1; dx <= 1; ++dx ){
    if( human_p.x + dx >= WIDTH ) continue;
    for( int dy = -1; dy <= 1; ++dy ){
      if( human_p.y + dy >= HEIGHT ) continue;

      int n_robots_remaining = 0;

      if( board.move_is_safe( dx, dy ) ){
	safe_move_exists = true;
	if( board.move_is_cascade_safe( dx, dy, n_robots_remaining ) ){
	  if( n_robots_remaining > n_robots_remaining_BEST ){
	    apr.dx = dx;
	    apr.dy = dy;
	    n_robots_remaining = n_robots_remaining_BEST;
	  }
	}
      }
    }
  }

  if( ! safe_move_exists ){
    apr.apre = AutoPilotResultEnum::TELEPORT;
    return apr;
  }

  apr.apre = AutoPilotResultEnum::MOVE;
  return apr;
}
