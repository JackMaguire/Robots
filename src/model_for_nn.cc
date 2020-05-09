#include "robots.cc"

#include <boost/python.hpp>
#include <boost/python/numpy.hpp>

using namespace boost;
using namespace boost::python;
using namespace boost::python::numpy;

namespace p = boost::python;
namespace np = boost::python::numpy;

BOOST_PYTHON_MODULE( model_for_nn )
{
  using namespace boost::python;
  Py_Initialize();
  np::initialize();

  class_<Board>("FilePos")
    .def("get_stringified_representation", &Base::get_stringified_representation )
    .def("load_from_stringified_representation", &Base::load_from_stringified_representation );
}
