#include "gcn.hh"

#include <boost/python.hpp>
#include <boost/python/numpy.hpp>

using namespace boost;
using namespace boost::python;
using namespace boost::python::numpy;

namespace p = boost::python;
namespace np = boost::python::numpy;

template< unsigned int ROUND >
boost::python::tuple
generate_data_from_str_tmpl
(
 std::string const str,
 unsigned int const N
 )
{
  using GCNForm = GCNFormulation< ROUND >;
  
  np::dtype const dtype = np::dtype::get_builtin<float>();
  
  try {
    
    Options const options = { N };
    typename GCNFormulation< ROUND >::Data const d =
      GCNForm::make_data( str, options );


    constexpr auto FF = GCNForm::F + GCNForm::Fx;
    p::tuple const Xshape = p::make_tuple( N, FF );
    
    np::ndarray const X_input_py = np::empty( Xshape, dtype );
    {
      /*
      std::vector< std::array< float, FF > > combinedF;
      assert( d.X.size() == d.X2.size() );
      combinedF.resize( d.X.size() );
      for( uint i = 0; i < d.X.size(); ++i ){
	for( uint j = 0; j < GCNForm::F; ++j ){
	  combinedF[ i ][ j ] = d.X[ i ][ j ];
	}
	for( uint j = 0; j < GCNForm::Fx; ++j ){
	  combinedF[ i ][ F+j ] = d.X2[ i ][ j ];
	}      
	}*/

      typename GCNForm::X12vec const combinedF = d.mergeXs();
      float * ndarray_data = reinterpret_cast< float * > ( X_input_py.get_data() );
      memcpy( ndarray_data, combinedF.data(), sizeof(float) * ( N * FF ) );
    }

    p::tuple const Ashape = p::make_tuple( N, N );
    np::ndarray const A_input_py = np::empty( Ashape, dtype );
    
    {
      std::vector< float > Adata;
      Adata.reserve( N*N );
      for( auto & v : d.A )
	for( float const f : v )
	  Adata.push_back( f );
      assert( Adata.size() == N*N );
    
      float * ndarray_data = reinterpret_cast< float * > ( A_input_py.get_data() );
      memcpy( ndarray_data, Adata.data(), sizeof(float)*N*N );
    }

    p::tuple const Eshape = p::make_tuple( N, N, GCNForm::S );
    np::ndarray const E_input_py = np::empty( Eshape, dtype );
    {
      std::vector< float > Edata;
      Edata.reserve( N*N*GCNForm::S );
      for( auto & v1 : d.E )
	for( auto & v2 : v1 )
	  for( float const f : v2 )
	    Edata.push_back( f );
      assert( Edata.size() == N*N*GCNForm::S );
    
      float * ndarray_data = reinterpret_cast< float * > ( E_input_py.get_data() );
      memcpy( ndarray_data, Edata.data(), sizeof(float)*N*N*GCNForm::S );
    }
    
    p::tuple const Oshape = p::make_tuple( GCNForm::O );
    np::ndarray const O_input_py = np::empty( Oshape, dtype );
    {        
      float * ndarray_data = reinterpret_cast< float * > ( O_input_py.get_data() );
      memcpy( ndarray_data, d.out.data(), sizeof(float)*GCNForm::O );
    }

    return boost::python::make_tuple( X_input_py, A_input_py, E_input_py, O_input_py );
  } catch ( IllegalMoveException & e ) {
    p::tuple const Xshape = p::make_tuple( 1 );
    np::ndarray const X_input_py = np::empty( Xshape, dtype );
    return boost::python::make_tuple( X_input_py, X_input_py, X_input_py, X_input_py );
  }
}

boost::python::tuple
generate_data_from_str
(
 std::string const str,
 unsigned int const N,
 unsigned int const round
 )
{
  switch( round ){
  case( 1 ):
    return generate_data_from_str_tmpl< 1 >( str, N );
  case( 2 ):
    return generate_data_from_str_tmpl< 2 >( str, N );
  default:
    assert( false );
  }
}


BOOST_PYTHON_MODULE( gcn_model_for_nn )
{
  using namespace boost::python;
  Py_Initialize();
  np::initialize();

  def( "generate_data_from_str", generate_data_from_str );
}
