#include "robots.cc"

#include <boost/python.hpp>
//#include <boost/python/numpy.hpp>

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
}

BOOST_PYTHON_MODULE( model_for_nn )
{
  using namespace boost::python;
  Py_Initialize();
  //np::initialize();

  class_<Board>("Board")
    .def("get_stringified_representation", &Board::get_stringified_representation )
    .def("load_from_stringified_representation", &Board::load_from_stringified_representation );

  using Game = RobotsGame< DummyVisualizer >;
  class_<Game>("Game")
    .def( "cascade_fast", &Game::cascade< false > )
    .def( "move_human", &Game::move_human )
    .def( "teleport", &Game::teleport )
    .def( "n_safe_teleports_remaining", &Game::n_safe_teleports_remaining )
    .def( "round", &Game::round )
    .def( "load_from_stringified_representation", &Board::load_from_stringified_representation );
}
