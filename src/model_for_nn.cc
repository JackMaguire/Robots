#include "robots.cc"

#include <boost/python.hpp>
//#include <boost/python/numpy.hpp>

#include <string>
#include <sstream>
#include <iostream>
#include <vector>


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

template< int SIZE >
struct BoardInput{
  static_assert( SIZE % 2 == 1, "Must be an odd size" );

  using NSTATES = 5;
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
