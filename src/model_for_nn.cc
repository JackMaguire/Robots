#include "robots.hh"
#include "data_reps.hh"

#include <boost/python.hpp>
#include <boost/python/numpy.hpp>

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <math.h>
#include <array>

#include <memory>

using namespace boost;
using namespace boost::python;
using namespace boost::python::numpy;

namespace p = boost::python;
namespace np = boost::python::numpy;

constexpr int board_input_size = 9;

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

using Game = RobotsGame< DummyVisualizer, false >;

boost::python::tuple
create_data(
  BoardInput< 9 > const & board_input,
  LocalInput const & local_input,
  KeyPress const key
){
  np::dtype const dtype = np::dtype::get_builtin<float>();
 
  p::tuple const board_input_shape =
    p::make_tuple( board_input_size, board_input_size, BoardInput< board_input_size >::NSTATES );
  np::ndarray const board_input_py = np::empty( board_input_shape, dtype );
  {
    float * ndarray_data = reinterpret_cast< float * > ( board_input_py.get_data() );
    memcpy( ndarray_data, board_input.data_.data(), sizeof( BoardInput< board_input_size >::Type ) );
  }

  p::tuple const local_input_shape = p::make_tuple( 3, 3, 5 );
  np::ndarray const local_input_py = np::empty( local_input_shape, dtype );
  {
    float * ndarray_data = reinterpret_cast< float * > ( local_input_py.get_data() );
    memcpy( ndarray_data, local_input.data_.data(), sizeof( LocalInput::Type ) );
  }

  p::tuple const key_shape = p::make_tuple( 11 );
  np::ndarray const output_py = np::empty( key_shape, dtype );
  {
    std::array< float, 11 > const data = key.get_one_hot();
    float * ndarray_data = reinterpret_cast< float * > ( output_py.get_data() );
    memcpy( ndarray_data, data.data(), sizeof( std::array< float, 11 > ) );
  }

  //TODO data augmentation
  return boost::python::make_tuple( board_input_py, local_input_py, output_py );
}

boost::python::tuple
sanity_check_values(){
  Board b;
  BoardInput< board_input_size > board_input( b );
  LocalInput local_input( b );

  KeyPress key( 1 );

  return create_data( board_input, local_input, key );
}

boost::python::tuple
parse_string(
  std::string const & str,
  bool const do_mirror,
  int const n_rotates
){
  //Format:
  //BOARD,NTELEPORT,ROUND,KEY
  std::vector< std::string > tokens = split_by_comma( str );
  if( tokens.size() != 4 ){
    std::cout << "tokens.size(): " << tokens.size() << std::endl;
    return boost::python::make_tuple( int( -1 ) );
  }

  KeyPress key( std::stoi( tokens[ 3 ] ) );
  if( key.press_ == KeyPress::Key::NONE ){
    return boost::python::make_tuple( int( -1 ) );
  }  

  Game game;
  game.load_from_stringified_representation(
    tokens[ 0 ],
    std::stoi( tokens[ 2 ] ),
    std::stoi( tokens[ 1 ] ),
    0 //score = 0, I guess
  );

  BoardInput< board_input_size > board_input( game.board() );
  LocalInput local_input( game.board() );

  if( do_mirror ){
    board_input.data_ = mirror< board_input_size >( board_input.data_ );
    local_input.data_ = mirror< 3 >( local_input.data_ );
    key.mirror_horizontally();
  }

  for( int i = 0; i < n_rotates; ++i ){
    board_input.data_ = rotate_right< board_input_size >( board_input.data_ );
    local_input.data_ = rotate_right< 3 >( local_input.data_ );
    key.rotate_to_the_right();
  }

  return create_data( board_input, local_input, key );
}

boost::python::tuple
get_observations( Game const & game ){
  BoardInput< board_input_size > board_input( game.board() );
  LocalInput local_input( game.board() );
  KeyPress key( 1 ); //dummy
  return create_data( board_input, local_input, key );
}

struct GamePtr {
  GamePtr(){
    game_ = std::make_shared< Game >();
  }

  GameOverBool
  cascade(){
    return game_->old_cascade();
  }  
  
  GameOverBool
  move_human( int const dx, int const dy ){
    return game_->move_human( dx, dy );
  }
  GameOverBool teleport(){ return game_->teleport(); }
  
  int n_safe_teleports_remaining(){ return game_->n_safe_teleports_remaining(); }
  int round(){ return game_->round(); }
  long int score(){ return game_->score(); }
  void reset(){ game_->reset(); }
  
  std::shared_ptr< Game > game_;
};

BOOST_PYTHON_MODULE( model_for_nn )
{
  using namespace boost::python;
  Py_Initialize();
  np::initialize();

  def( "parse_string", parse_string );
  def( "sanity_check_values", sanity_check_values );
  def( "get_observations", get_observations );

  class_<Board>("Board")
    .def("get_stringified_representation", &Board::get_stringified_representation )
    .def("load_from_stringified_representation", &Board::load_from_stringified_representation );

  class_<GamePtr>("GamePtr")
    .def( "fast_cascade", &GamePtr::cascade )
    .def( "move_human", &GamePtr::move_human )
    .def( "teleport", &GamePtr::teleport )
    .def( "n_safe_teleports_remaining", &GamePtr::n_safe_teleports_remaining )
    .def( "round", &GamePtr::round )
    .def( "score", &GamePtr::score )
    .def( "reset", &GamePtr::reset );
  
  class_<Game>("Game")
    .def( "fast_cascade", &Game::old_cascade )
    .def( "move_human", &Game::move_human )
    .def( "teleport", &Game::teleport )
    .def( "n_safe_teleports_remaining", &Game::n_safe_teleports_remaining )
    .def( "round", &Game::round )
    .def( "score", &Game::score )
    .def( "reset", &Game::reset )
    .def( "load_from_stringified_representation", &Game::load_from_stringified_representation );
}
