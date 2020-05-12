#include "robots.hh"

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <math.h>

template< int SIZE, typename T >
std::array< T, SIZE >
mirror( std::array< T, SIZE > const & src ){
  std::array< T, SIZE > mirrored;

  for( int i = 0; i < SIZE; ++i ){
      mirrored[ (SIZE - 1) - i ] = src[ i ];
  }

  return mirrored;
}

template< int SIZE, typename T >
std::array< std::array< T, SIZE >, SIZE >
rotate_right( std::array< std::array< T, SIZE >, SIZE > const & src ){
  std::array< std::array< T, SIZE >, SIZE > rotated;

  //Outer index is X
  //Inner index is Y

  //Table
  /*
    OLD X    NEW Y
    -1       1
    0        0
    1        -1
    2        -2

    OLD Y    NEW X
    -1       -1
    0        0
    1        1
    2        2
   */

  static_assert( SIZE % 2 == 1, "Must be an odd size" );  
  constexpr int OFFSET = SIZE / 2;

  auto && dx_to_i =
    []( int const dx ){
      return dx + OFFSET;
    };

  auto && i_to_dx =
    []( int const i ){
      return i - OFFSET;
    };

  for( int i = 0; i < SIZE; ++i ){
    for( int j = 0; j < SIZE; ++j ){
      int const dx = i_to_dx( i );
      int const dy = i_to_dx( j );

      int const newdx = dy;
      int const newdy = dx * -1;

      int const newi = dx_to_i( newdx );
      int const newj = dx_to_i( newdy );

      rotated[ newi ][ newj ] = src[ i ][ j ];
    }
  }

  return rotated;
}

template< int SIZE >
struct BoardInput{
  static_assert( SIZE % 2 == 1, "Must be an odd size" );

  static constexpr int NSTATES = 5;
  using Type = std::array< std::array< std::array< float, NSTATES >, SIZE >, SIZE >;
  Type data_;

  static
  std::array< float, NSTATES >
  onehot_encoding_for_OOB(){
    std::array< float, NSTATES > array;
    array.fill( 0 );
    //Set last value to 1
    array[ NSTATES - 1 ] = 1.0;
    return array;
  }

  static
  std::array< float, NSTATES >
  onehot_encode( Occupant o ){
    std::array< float, NSTATES > array;
    array.fill( 0 );
    //All enums in this project are 0-indexed
    array[ int(o) ] = 1.0;
    return array;    
  }

  BoardInput() = default;
  BoardInput( BoardInput const & ) = default;
  BoardInput( BoardInput && ) = default;

  BoardInput( Board const & board ){
    Position const hpos = board.human_position();

    Position pos;
    constexpr int OFFSET = SIZE / 2;
    for( int i = 0; i < SIZE; ++i ){
      pos.x = hpos.x + i - OFFSET;
      for( int j = 0; j < SIZE; ++j ){
	pos.y = hpos.y + j - OFFSET;
	if( Board::position_is_in_bounds( pos ) ){
	  data_[ i ][ j ] = onehot_encode( board.cell( pos ) );
	} else {
	  data_[ i ][ j ] = onehot_encoding_for_OOB();
	}
      } 
    }
  }
};

bool count_for_c5(
  int const dx,
  int const dy,
  Position const hpos,
  Position const rpos
){
  switch( dx ){

  case( -1 ):
    switch( dy ){
    case( -1 ):
      //7 oclock
      return (rpos.x < hpos.x) && (rpos.y < hpos.y) &&
	( hpos.y - rpos.y > hpos.x - rpos.x );
    case( 0 ):
      //8 oclock
      return (rpos.x < hpos.x) && (rpos.y < hpos.y) &&
	( hpos.y - rpos.y < hpos.x - rpos.x );
    case( 1 ):
      //10 oclock
      return (rpos.x < hpos.x) && (rpos.y > hpos.y) &&
	( rpos.y - hpos.y < hpos.x - rpos.x );
    }
    break;

  case( 0 ):
    switch( dy ){
    case( -1 ):
      //5 oclock
      return (rpos.x > hpos.x) && (rpos.y < hpos.y) &&
	( hpos.y - rpos.y > rpos.x - hpos.x );
    case( 0 ):
      //n robots total
      return true;
    case( 1 ):
      //11 oclock
      return (rpos.x < hpos.x) && (rpos.y > hpos.y) &&
	( rpos.y - hpos.y > hpos.x - rpos.x );
    }
    break;

  case( 1 ):
    switch( dy ){
    case( -1 ):
      //4 oclock
      return (rpos.x > hpos.x) && (rpos.y < hpos.y) &&
	( hpos.y - rpos.y < rpos.x - hpos.x );
    case( 0 ):
      //2 oclock
      return (rpos.x > hpos.x) && (rpos.y > hpos.y) &&
	( rpos.y - hpos.y < rpos.x - hpos.x );
    case( 1 ):
      //1 oclock
      return (rpos.x > hpos.x) && (rpos.y > hpos.y) &&
	( rpos.y - hpos.y > rpos.x - hpos.x );
    }
    break;

  }//switch dx

  //unreachable
  return false;
}

int get_channel_5(
  int const dx,
  int const dy,
  Board const & board
){
  Position const hpos = board.human_position();
  int nrobots = 0;
  return nrobots;
}

struct LocalInput {
  using Type = std::array< std::array< std::array< float, 5 >, 3 >, 3 >;
  Type data_;

  LocalInput() = default;
  LocalInput( LocalInput const & ) = default;
  LocalInput( LocalInput && ) = default;

  LocalInput( Board const & board ){
    Position const hpos = board.human_position();

    Position pos;
    for( int const dx : { -1, 0, 1 } ){
      pos.x = hpos.x + dx;
      for( int const dy : { -1, 0, 1 } ){
	pos.y = hpos.y + dy;

	Board clone = board;

	//Channel 1 : Is it safe to move here?
	MoveResult const move_result = clone.move_human( dx, dy );
	bool const safe_to_move = ( move_result != MoveResult::YOU_LOSE );

	//Channel 2: If it is safe, how many robots die?
	int const n_dead_robots = ( safe_to_move ? board.n_robots() - clone.n_robots() : 0 );

	//Channel 3: If it is safe, can you cascade from there?
	bool safe_to_cascade;
	if( move_result == MoveResult::CONTINUE ){
	  MoveResult cascade_result = MoveResult::CONTINUE;
	  while( cascade_result == MoveResult::CONTINUE ){
	    cascade_result = clone.move_robots_1_step();
	  }
	  safe_to_cascade = ( cascade_result != MoveResult::YOU_LOSE );
	} else {
	  //lose, win round, win game
	  safe_to_cascade = safe_to_move;
	}

	//Channel 4: n_robots in line of sight
	int nbots_los = 0;
	if( dx != 0 && dy != 0 ){
	  Position p = hpos;
	  p.x += dx;
	  p.y += dy;
	  while( Board::position_is_in_bounds( p ) ){
	    if( board.cell( p ) == Occupant::ROBOT ){
	      ++nbots_los;
	    }
	    p.x += dx;
	    p.y += dy;
	  }
	}

	//Channel 5: n robots in region
	int nbots_region = 0;
	for( Position const robot : board.robots() ){
	  if( count_for_c5( dx, dy, hpos, robot ) ) ++nbots_region;
	}

	//Normalizing!
	int const i = dx + 1;
	int const j = dy + 1;

	data_[ i ][ j ][ 0 ] = ( safe_to_move ? 1.0 : 0.0 );
	data_[ i ][ j ][ 1 ] = ( n_dead_robots == 0 ? -2 : log(n_dead_robots) - 1 ); //ln
	data_[ i ][ j ][ 2 ] = ( safe_to_cascade ? 1.0 : 0.0 );
	data_[ i ][ j ][ 3 ] = ( nbots_los == 0 ? -2 : log10(nbots_los) - 1 ); //log10
	data_[ i ][ j ][ 4 ] = ( nbots_region == 0 ? -2 : log10(nbots_region) - 1 ); //log10
      }
    }
    
  }
};

struct KeyPress {
  enum class
  Key {
       NONE,
       Q,
       W,
       E,
       A,
       S,
       D,
       Z,
       X,
       C,
       T,
       SPACE,
       DELETE
  };

  Key press_;

  KeyPress( int const press )
  {
    press_ = Key( press );
  }

  std::array< float, 11 >
  get_one_hot() const {
    std::array< float, 11 > data;
    data.fill( 0 );
    switch( press_ ){
      case( Key::Q ): data[ 0 ] = 1.0; break;
      case( Key::W ): data[ 1 ] = 1.0; break;
      case( Key::E ): data[ 2 ] = 1.0; break;
      case( Key::A ): data[ 3 ] = 1.0; break;
      case( Key::S ): data[ 4 ] = 1.0; break;
      case( Key::D ): data[ 5 ] = 1.0; break;
      case( Key::Z ): data[ 6 ] = 1.0; break;
      case( Key::X ): data[ 7 ] = 1.0; break;
      case( Key::C ): data[ 8 ] = 1.0; break;
      case( Key::T ): data[ 9 ] = 1.0; break;
      case( Key::SPACE ): data[ 10 ] = 1.0; break;
      default: break;
    }
    return data;
  }

  void rotate_to_the_right(){
    switch( press_ ){
    case( Key::Q ):
      press_ = Key::E;
      break;
    case( Key::W ):
      press_ = Key::D;
      break;
    case( Key::E ):
      press_ = Key::C;
      break;
    case( Key::D ):
      press_ = Key::X;
      break;
    case( Key::C ):
      press_ = Key::Z;
      break;
    case( Key::X ):
      press_ = Key::A;
      break;
    case( Key::Z ):
      press_ = Key::Q;
      break;
    case( Key::A ):
      press_ = Key::W;
      break;
    default:
      break;
    }
  }

  void mirror_horizontally(){
    switch( press_ ){
    case( Key::Q ):
      press_ = Key::E;
      break;
    case( Key::E ):
      press_ = Key::Q;
      break;
    case( Key::A ):
      press_ = Key::D;
      break;
    case( Key::D ):
      press_ = Key::A;
      break;
    case( Key::Z ):
      press_ = Key::C;
      break;
    case( Key::C ):
      press_ = Key::Z;
      break;
    default:
      break;
    }
  }

};
