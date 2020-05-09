#include "robots.cc"

#include <boost/python.hpp>
//#include <boost/python/numpy.hpp>

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <math.h>

using namespace boost;
using namespace boost::python;
//using namespace boost::python::numpy;

namespace p = boost::python;
//namespace np = boost::python::numpy;

namespace {
class DummyVisualizer {
public:
  static void show( Board const & ){}
};

std::vector< std::string >
split_by_comma( std::string const & instring ){
  std::stringstream ss( instring );
  std::vector< std::string > result;

  while( ss.good() ) {
    std::string substr;
    getline( ss, substr, ',' );
    result.push_back( substr );
  }
  return result;
}

}//anonymous namespace

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
	  }
	}

	//Channel 5: n robots in region
	int nbots_region = 0;
	if( dx != 0 && dy != 0 ){
	  for( Position const robot : board.robots() ){
	    bool valid_for_dx = true;
	    switch( dx ){
	    case( -1 ):
	      valid_for_dx = robot.x < hpos.x;
	      break;
	    case( 0 ):
	      valid_for_dx = robot.x == hpos.x;
	      break;
	    case( 1 ):
	      valid_for_dx = robot.x > hpos.x;
	      break;
	    }
	    if( ! valid_for_dx ) continue;

	    bool valid_for_dy = true;
	    switch( dy ){
	    case( -1 ):
	      valid_for_dy = robot.y < hpos.y;
	      break;
	    case( 0 ):
	      valid_for_dy = robot.y == hpos.y;
	      break;
	    case( 1 ):
	      valid_for_dy = robot.y > hpos.y;
	      break;
	    }

	    if( valid_for_dx && valid_for_dy ){
	      ++nbots_region;
	    }
	  }
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

using Game = RobotsGame< DummyVisualizer, false >;

boost::python::tuple
parse_string( std::string const & str ){
  //Format:
  //BOARD,NTELEPORT,ROUND,KEY
  std::vector< std::string > tokens = split_by_comma( str );
  if( tokens.size() != 4 ){
    std::cout << "tokens.size(): " << tokens.size() << std::endl;
    return boost::python::make_tuple( int( -1 ) );
  }

  Game game;
  game.load_from_stringified_representation(
    tokens[ 0 ],
    std::stoi( tokens[ 2 ] ),
    std::stoi( tokens[ 1 ] ),
    0 //score = 0, I guess
  );

  return boost::python::make_tuple( int( -1 ) );
}

BOOST_PYTHON_MODULE( model_for_nn )
{
  using namespace boost::python;
  Py_Initialize();
  //np::initialize();

  class_<Board>("Board")
    .def("get_stringified_representation", &Board::get_stringified_representation )
    .def("load_from_stringified_representation", &Board::load_from_stringified_representation );

  class_<Game>("Game")
    .def( "fast_cascade", &Game::cascade )
    .def( "move_human", &Game::move_human )
    .def( "teleport", &Game::teleport )
    .def( "n_safe_teleports_remaining", &Game::n_safe_teleports_remaining )
    .def( "round", &Game::round )
    .def( "load_from_stringified_representation", &Game::load_from_stringified_representation );
}
