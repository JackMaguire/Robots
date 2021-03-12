#pragma once

#include <cppflow/ops.h>
#include <cppflow/model.h>

#include <iostream>
#include "robots.hh"
#include "gcn.hh"

struct Prediction {
  int dx = -2;
  int dy = -2;
};

constexpr unsigned int N32 = 32;

inline
Prediction
argmax2pred( int const argmax ){
  Prediction pred;

  //dx
  switch( argmax ){
    case( 2 ):
    case( 5 ):
    case( 8 ):
      pred.dx = 1;
    break;

    case( 1 ):
    case( 4 ):
    case( 7 ):
      pred.dx = 0;
    break;

    case( 0 ):
    case( 3 ):
    case( 6 ):
      pred.dx = -1;
    break;

    default:
      assert( false );
  }

  //dy
  switch( argmax ){
    case( 0 ):
    case( 1 ):
    case( 2 ):
      pred.dy = 1;
    break;

    case( 3 ):
    case( 4 ):
    case( 5 ):
      pred.dy = 0;
    break;

    case( 6 ):
    case( 7 ):
    case( 8 ):
      pred.dy = -1;
    break;

    default:
      assert( false );
  }

  return pred;
}

struct GCN {
  GCN() :
    model("/saved_models/2BM.6")
  {
    for ( std::string const & s : model.get_operations() ){
      //std::cout << s << std::endl;
      //show_shape( s );
    }

    auto Xshape = model.get_operation_shape( "serving_default_X_in" );
    for ( auto const x : Xshape ){
      //std::cout << "X " << x << std::endl;
    }

    auto Ashape = model.get_operation_shape( "serving_default_A_in" );
    for ( auto const x : Ashape ){
      //std::cout << "A " << x << std::endl;
    }

    auto Eshape = model.get_operation_shape( "serving_default_E_in" );
    for ( auto const x : Eshape ){
      //std::cout << "E " << x << std::endl;
    }

    /*auto Oshape = model.get_operation_shape( "StatefulPartitionedCall:0" );
    for ( auto const x : Oshape ){
      //std::cout << "O " << x << std::endl;
    }*/

    run_sanity_check();
  }

  void show_shape( std::string const & op ) const {
    auto shape = model.get_operation_shape( op );
    for ( auto const x : shape ){
      //std::cout << op << " " << x << std::endl;
    }    
  }

  void
  run_sanity_check(){

    X12vec X( N32 );
    for( auto & x: X ) x.fill( 0 );

    Avec A( N32 );
    for( auto & a : A ) a.assign( N32, 1 );

    Evec E( N32 );
    for( auto & e : E ){
      e.resize( N32 );
      for( auto & e2 : e ){
	e2.fill( 2 );
      }
    }

    cppflow::tensor Xtensor(
      cppflow::deduce_tf_type< float >(),
      X.data(),
      N32 * (F+Fx) * sizeof( float ),
      {1, N32, F+Fx}
    );

    cppflow::tensor Atensor(
      cppflow::deduce_tf_type< float >(),
      A.data(),
      N32 * N32 * sizeof( float ),
      {1, N32, N32}
    );

    cppflow::tensor Etensor(
      cppflow::deduce_tf_type< float >(),
      E.data(),
      N32 * N32 * S * sizeof( float ),
      {1, N32, N32, S}
    );

    std::vector< cppflow::tensor > const output = model(
      {
	{"serving_default_X_in:0", Xtensor},
	  {"serving_default_A_in:0", Atensor},
	    {"serving_default_E_in:0", Etensor},
	      },
      {"StatefulPartitionedCall:0"});
      //{"conv1d_1/bias/v/Read/ReadVariableOp:0"});

    //std::cout << "output.size() " << output.size() << std::endl;
    assert( output.size() == 1 );
    for( auto o : output[0].get_data< float >() ){
      //assert( abs(o-0.11111) < 0.001 );
      //std::cout << "O: " << o << std::endl;
    }
  }

  template< typename GAME >
  Prediction
  predict( GAME const & game ){
    
    Options options;
    options.N = N32;

    Data const data = make_data(
      game.board(),
      game.n_safe_teleports_remaining(),
      int(Key::S), //dummy
      options
    );

    X12vec const X = data.mergeXs();
    auto const & A = data.A;
    auto const & E = data.E;

    cppflow::tensor Xtensor(
      cppflow::deduce_tf_type< float >(),
      X.data(),
      N32 * (F+Fx) * sizeof( float ),
      {1, N32, F+Fx}
    );

    cppflow::tensor Atensor(
      cppflow::deduce_tf_type< float >(),
      A.data(),
      N32 * N32 * sizeof( float ),
      {1, N32, N32}
    );

    cppflow::tensor Etensor(
      cppflow::deduce_tf_type< float >(),
      E.data(),
      N32 * N32 * S * sizeof( float ),
      {1, N32, N32, S}
    );

    std::vector< cppflow::tensor > const output = model(
      {
	{"serving_default_X_in:0", Xtensor},
	  {"serving_default_A_in:0", Atensor},
	    {"serving_default_E_in:0", Etensor},
	      },
      {"StatefulPartitionedCall:0"});

    //std::cout << "output.size() " << output.size() << std::endl;
    assert( output.size() == 1 );
    for( auto o : output[0].get_data< float >() ){
      //std::cout << "O: " << o << std::endl;
    }
    
    Prediction pred;
    return pred;
  }

  cppflow::model model;
};

TF_Tensor *FloatTensor(const int64_t *dims, int num_dims, const float *values) {
  int64_t num_values = 1;

  for (int i = 0; i < num_dims; ++i) {
    num_values *= dims[i];
  }

  TF_Tensor *t =
    TF_AllocateTensor(TF_FLOAT, dims, num_dims, sizeof(float) * num_values);

  memcpy(TF_TensorData(t), values, sizeof(float) * num_values);

  return t;
}

struct OldSchoolGCN {

  OldSchoolGCN(){
    load();
    run_sanity_check();
  }

  ~OldSchoolGCN(){
    TF_DeleteSession( session_, status_ );
    TF_DeleteGraph( graph_ );
    TF_DeleteSessionOptions( sess_options_ );
    TF_DeleteStatus( status_ );
  }

  void load(){
    constexpr char kSavedModelTagServe[] = "serve";
    const char *tags[] = {kSavedModelTagServe};

    status_ = TF_NewStatus();
    graph_ = TF_NewGraph();
    sess_options_ = TF_NewSessionOptions();

    session_ = TF_LoadSessionFromSavedModel(
      sess_options_, nullptr, "/saved_models/2BM.6",
      tags, 1, graph_, nullptr, status_ );

    status_check();
  }

  void run_sanity_check(){

    constexpr int ninputs = 3;
    constexpr int noutputs = 1;

    std::unique_ptr<TF_Output[]> inputs(new TF_Output[ninputs]);
    std::unique_ptr<TF_Tensor *[]> input_values(new TF_Tensor *[ninputs]);
    std::unique_ptr<TF_Output[]> outputs(new TF_Output[noutputs]);
    std::unique_ptr<TF_Tensor *[]> output_values(new TF_Tensor *[noutputs]);
    
    {//input1
      //const float input1_row[ 3 ] = { -1, 0, 1 };
      std::array< float, N32 * (F+Fx) > Xdata;
      Xdata.fill( 0.0 );
      //const float input1_row[ N32 * (F+Fx) ] = Xdata.data();
      const int64_t input1_dims[3] = { 1, N32, F+Fx };
      auto input1_tensor = FloatTensor(input1_dims, 3, Xdata.data() );

      TF_Operation * const op = TF_GraphOperationByName( graph_, "serving_default_X_in" );
      assert( op != nullptr );
      inputs.get()[0] = TF_Output{op, 0};

      input_values.get()[0] = input1_tensor;
    }

    {//input2
      std::array< float, N32 * N32 > Adata;
      Adata.fill( 1.0 );
      //const float input2_row[ N32 * N32 ] = Adata.data();
      const int64_t input2_dims[3] = { 1, N32, N32 };
      auto input2_tensor = FloatTensor(input2_dims, 3, Adata.data() );

      TF_Operation * const op = TF_GraphOperationByName( graph_, "serving_default_A_in" );
      assert( op != nullptr );
      inputs.get()[1] =
	TF_Output{op, 0};

      input_values.get()[1] = input2_tensor;
    }

    {//input3
      std::array< float, N32 * N32 * S > Edata;
      Edata.fill( 2.0 );
      //const float input3_row[ N32*N32*S ] = Edata.data();
      const int64_t input3_dims[4] = { 1, N32, N32, S };
      auto input3_tensor = FloatTensor(input3_dims, 4, Edata.data());

      TF_Operation * const op = TF_GraphOperationByName( graph_, "serving_default_E_in" );
      assert( op != nullptr );
      inputs.get()[2] =
	TF_Output{op, 0};

      input_values.get()[2] = input3_tensor;
    }

    {
      const float output_row[9] = { 0,0,0, 0,0,0, 0,0,0 };
      const int64_t output_dims[2] = {1, 9};
      auto output_tensor = FloatTensor( output_dims, 2, output_row );

      TF_Operation * const out_op = TF_GraphOperationByName( graph_, "StatefulPartitionedCall" );
      assert( out_op != nullptr );
      outputs.get()[0] =
	TF_Output{ out_op, 0 };

      output_values.get()[0] = output_tensor;
    }

    TF_SessionRun( session_, nullptr, inputs.get(),
      input_values.get(), ninputs, outputs.get(),
      output_values.get(), noutputs, nullptr, 0,
      nullptr, status_ ); 

    status_check();

    float *values = static_cast<float *>(TF_TensorData(output_values.get()[0]));

    for( uint i = 0; i < 9; ++i ){
      //std::cout << "O: " << values[ i ] << std::endl;
      assert( abs(values[ i ]-0.11111) < 0.001 );
    }
    
    TF_DeleteTensor( input_values.get()[0] );
    TF_DeleteTensor( input_values.get()[1] );
    TF_DeleteTensor( input_values.get()[2] );
    TF_DeleteTensor( output_values.get()[0] );
  }

  template< typename GAME >
  Prediction
  predict( GAME const & game ){

    Options options;
    options.N = N32;

    Data const data = make_data(
      game.board(),
      game.n_safe_teleports_remaining(),
      int(Key::NONE), //dummy
      options
    );

    X12vec const X = data.mergeXs();
    auto const & A = data.A;
    auto const & E = data.E;


    std::unique_ptr<TF_Output[]> inputs(new TF_Output[ninputs]);
    std::unique_ptr<TF_Tensor *[]> input_values(new TF_Tensor *[ninputs]);
    std::unique_ptr<TF_Output[]> outputs(new TF_Output[noutputs]);
    std::unique_ptr<TF_Tensor *[]> output_values(new TF_Tensor *[noutputs]);
    
    {//input1
      const int64_t input1_dims[3] = { 1, N32, F+Fx };
      auto input1_tensor = FloatTensor(input1_dims, 3, &X[0][0] );

      TF_Operation * const op = TF_GraphOperationByName( graph_, "serving_default_X_in" );
      assert( op != nullptr );
      inputs.get()[0] = TF_Output{op, 0};

      input_values.get()[0] = input1_tensor;
    }

    {//input2
      float input2_row[ N32 * N32 ];

      int count = 0;
      for( uint i = 0; i < N32; ++i ){
	for( uint j = 0; j < N32; ++j ){
	  input2_row[ count ] = A[ i ][ j ];
	  ++count;
	}
      }

      const int64_t input2_dims[3] = { 1, N32, N32 };
      auto input2_tensor = FloatTensor(input2_dims, 3, input2_row );

      TF_Operation * const op = TF_GraphOperationByName( graph_, "serving_default_A_in" );
      assert( op != nullptr );
      inputs.get()[1] =
	TF_Output{op, 0};

      input_values.get()[1] = input2_tensor;
    }

    {//input3
      float input3_row[ N32*N32*S ];

      int count = 0;
      for( uint i = 0; i < N32; ++i ){
	for( uint j = 0; j < N32; ++j ){
	  for( uint k = 0; k < S; ++k ){
	    input3_row[ count ] = E[ i ][ j ][ k ];
	    ++count;
	  }
	}
      }


      const int64_t input3_dims[4] = { 1, N32, N32, S };
      auto input3_tensor = FloatTensor(input3_dims, 4, input3_row);

      TF_Operation * const op = TF_GraphOperationByName( graph_, "serving_default_E_in" );
      assert( op != nullptr );
      inputs.get()[2] =
	TF_Output{op, 0};

      input_values.get()[2] = input3_tensor;
    }

    {
      const float output_row[9] = { 0,0,0, 0,0,0, 0,0,0 };
      const int64_t output_dims[2] = {1, 9};
      auto output_tensor = FloatTensor( output_dims, 2, output_row );

      TF_Operation * const out_op = TF_GraphOperationByName( graph_, "StatefulPartitionedCall" );
      assert( out_op != nullptr );
      outputs.get()[0] =
	TF_Output{ out_op, 0 };

      output_values.get()[0] = output_tensor;
    }

    TF_SessionRun( session_, nullptr, inputs.get(),
      input_values.get(), ninputs, outputs.get(),
      output_values.get(), noutputs, nullptr, 0,
      nullptr, status_ ); 

    status_check();

    float *values = static_cast<float *>(TF_TensorData(output_values.get()[0]));

    int argmax = -1;
    float high = 0;
    for( uint i = 0; i < 9; ++i ){
      //std::cout << "O*: " << values[ i ] << std::endl;
      if( values[ i ] > high ){
	high = values[ i ];
	argmax = i;
      }
    }
    
    TF_DeleteTensor( input_values.get()[0] );
    TF_DeleteTensor( input_values.get()[1] );
    TF_DeleteTensor( input_values.get()[2] );
    TF_DeleteTensor( output_values.get()[0] );

    return argmax2pred( argmax );
  }

  void status_check() const {
    if (TF_GetCode( status_ ) != TF_OK) {
      std::cout << "ERROR " << std::string(TF_Message(status_)) << std::endl; 
      assert( false );
    }
  }

private:

  static constexpr int ninputs = 3;
  static constexpr int noutputs = 1;

  /// @brief Status of the Tensorflow session.
  TF_Status* status_ = nullptr;

  /// @brief Graph for the Tensorflow session.
  TF_Graph* graph_ = nullptr;

  /// @brief Options for the Tensorflow session.
  TF_SessionOptions* sess_options_ = nullptr;

  /// @brief The Tensorflow session.
  /// @details Note that this is stored by raw pointer.  The destructor for this container class destroys the session using
  /// Tensorflow's safe cleanup functions.
  TF_Session* session_ = nullptr;


};
