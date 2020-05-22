#pragma once
#include "tensorflow/c/c_api.h" //https://www.tensorflow.org/install/lang_c

#include "data_reps.hh"
#include "robots.hh"

#include <assert.h>
#include <iostream>

struct StatusDeleter {
  void operator()( TF_Status * ptr ) const {
    TF_DeleteStatus( ptr );
  }
};
using StatusPtr = std::unique_ptr< TF_Status, StatusDeleter >;
static StatusPtr tf_status;


struct GraphDeleter {
  void operator()( TF_Graph * ptr ) const {
    TF_DeleteGraph( ptr );
  }
};
using GraphPtr = std::unique_ptr< TF_Graph, GraphDeleter >;
static GraphPtr tf_graph;


struct SODeleter {
  void operator()( TF_SessionOptions * ptr ) const {
    TF_DeleteSessionOptions( ptr );
  }
};
using SOPtr = std::unique_ptr< TF_SessionOptions, SODeleter >;
static SOPtr tf_session_options;


struct SessionDeleter {
  void operator()( TF_Session * ptr ) const {
    TF_DeleteSession( ptr, tf_status.get() );
  }
};
using SessionPtr = std::unique_ptr< TF_Session, SessionDeleter >;
static SessionPtr tf_session;

struct TensorDeleter {
  void operator()( TF_Tensor * ptr ) const {
    TF_DeleteTensor( ptr );
  }
};
using TensorPtr = std::unique_ptr< TF_Tensor, TensorDeleter >;

void run_sanity_check();

void
load_session(){
  tf_status = StatusPtr( TF_NewStatus() );
  tf_graph = GraphPtr( TF_NewGraph() );
  tf_session_options = SOPtr( TF_NewSessionOptions() );
  std::string const path = "../pretraining/first_model/saved_model";
  std::string const tag = "serve";
  const char *tagchars[] = { &tag.c_str()[0] };

  tf_session = SessionPtr( TF_LoadSessionFromSavedModel(
      tf_session_options.get(),
      nullptr,
      path.c_str(),
      tagchars,
      1,
      tf_graph.get(),
      nullptr,
      tf_status.get()
    ) );


  {
    size_t pos = 0;
    TF_Operation* oper;
    std::cout << "All Operations: " << std::endl;
    while ((oper = TF_GraphNextOperation( tf_graph.get(), &pos )) != nullptr) {
      std::cout << TF_OperationName( oper ) << std::endl;
    }
  }
}


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

std::array< float, 11 >
run_tf(
  BoardInput< 9 > const & board_input,
  LocalInput const & local_input
) {

  std::string const name1 = "serving_default_in1";
  std::string const name2 = "serving_default_in2";
  std::string const outname = "StatefulPartitionedCall";//"output/Softmax";

  if ( tf_session == nullptr ){
    load_session();
    assert( TF_GetCode( tf_status.get() ) == TF_OK );
    //std::array< float, 11 > r;
    //return r;
  }

  const int ninputs = 2;
  const int noutputs = 1;

  std::unique_ptr<TF_Output[]> inputs(new TF_Output[ninputs]);
  std::unique_ptr<TF_Tensor *[]> input_values(new TF_Tensor *[ninputs]);
  std::unique_ptr<TF_Output[]> outputs(new TF_Output[noutputs]);
  std::unique_ptr<TF_Tensor *[]> output_values(new TF_Tensor *[noutputs]);

  const int64_t input1_dims[4] = {1, 9, 9, 5 };
  TF_Tensor * input1_tensor = FloatTensor( input1_dims, 4, &board_input.data_[0][0][0] );
  TensorPtr in1 = TensorPtr( input1_tensor );

  const int64_t input2_dims[4] = {1, 3, 3, 5 };
  TF_Tensor * input2_tensor = FloatTensor( input2_dims, 4, &local_input.data_[0][0][0] );
  TensorPtr in2 = TensorPtr( input2_tensor );

  {//input1
    TF_Operation * const op = TF_GraphOperationByName( tf_graph.get(), name1.c_str() );
    assert( op );
    inputs.get()[0] =
      TF_Output{op, 0};
  }

  {//input2
    TF_Operation * const op = TF_GraphOperationByName( tf_graph.get(), name2.c_str() );
    assert( op );
    inputs.get()[1] =
      TF_Output{op, 0};
  }

  if ( TF_GetCode( tf_status.get() ) != TF_OK ) {
    std::cout << "Unable to fetch input operation: " << std::string( TF_Message( tf_status.get() ) ) << std::endl;
  }
  assert( TF_GetCode( tf_status.get() ) == TF_OK );

  input_values.get()[0] = input1_tensor;
  input_values.get()[1] = input2_tensor;

  std::array< float, 11 > output_row;
  output_row.fill( 0.0 );
  const int64_t output_dims[2] = {1, 11};
  auto output_tensor = FloatTensor( output_dims, 2, output_row.data() );

  {
    TF_Operation * const out_op =
      TF_GraphOperationByName( tf_graph.get(), outname.c_str() );
    assert( out_op );
    outputs.get()[0] =
      TF_Output{ out_op, 0 };
  }

  output_values.get()[0] = output_tensor;

  if ( TF_GetCode( tf_status.get() ) != TF_OK ) {
    std::cout << "Unable to fetch input operation: " << std::string( TF_Message( tf_status.get() ) ) << std::endl;
  }
  assert( TF_GetCode( tf_status.get() ) == TF_OK );

  TF_SessionRun(
    tf_session.get(),
    nullptr,
    inputs.get(), input_values.get(), ninputs,
    outputs.get(), output_values.get(), noutputs,
    nullptr, 0, nullptr, tf_status.get() );

  if ( TF_GetCode( tf_status.get() ) != TF_OK ) {
    std::cout << "Unable to fetch input operation: " << std::string( TF_Message( tf_status.get() ) ) << std::endl;
  }
  assert( TF_GetCode( tf_status.get() ) == TF_OK );

  float *values = static_cast<float *>(TF_TensorData(output_values.get()[0]));
  memcpy( output_row.data(), values, sizeof( std::array< float, 11 > ) );

  return output_row;
}

template< typename T >
void
predict_ai( RobotsGame<T> const & game ){
  DefaultBoardInput const board( game.board() );
  LocalInput const local( game.board() );
  std::array< float, 11 > const & results = run_tf( board, local );
  for( int i = 0; i < 11; ++i ){
    std::cout << i << " " << results[ i ] << std::endl;
  }
}

void run_sanity_check(){
  Board b;
  BoardInput< 9 > board_input( b );
  LocalInput local_input( b );

  std::array< float, 11 > results = run_tf( board_input, local_input );
  std::cout << "Sanity Check" << std::endl;
  for( float f : results ){
    std::cout << f << std::endl;
  }
}
