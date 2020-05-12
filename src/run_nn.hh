#include "tf"

#include "data_reps.hh"

#pragma once

struct StatusDeleter {
  void operator()( TF_Status * ptr ) const {
    TF_DeleteStatus( ptr );
  }
};
static std::unique_ptr< TF_Status, StatusDeleter > tf_status;

struct GraphDeleter {
  void operator()( TF_Graph * ptr ) const {
    TF_DeleteGraph( ptr );
  }
};
static std::unique_ptr< TF_Graph, GraphDeleter > tf_graph;

struct SODeleter {
  void operator()( TF_SessionOptions * ptr ) const {
    TF_DeleteSessionOptions( ptr );
  }
};
static std::unique_ptr< TF_SessionOptions, SODeleter > tf_session_options;

struct SessionDeleter {
  void operator()( TF_Session * ptr ) const {
    TF_DeleteSession( ptr, tf_status.get() );
  }
};
static std::unique_ptr< TF_Session, SessionDeleter > tf_session;

void
load_session(){
  tf_status.get() = TF_NewStatus();
  tf_graph.get() = TF_NewGraph();
  tf_session_options.get() = TF_NewSessionOptions();
  std::string const path = "../pretraining/first_model/saved_model";
  std::string const tag = "serve";
  const char *tagchars[] = { &tag.c_str()[0] };

  tf_session.get() = TF_LoadSessionFromSavedModel(
    tf_session_options.get(),
    nullptr,
    path.c_str(),
    "serve",
    tagchars,
    1,
    tf_graph.get(),
    nullptr,
    tf_status.get()
  );
}

KeyPress
run_tf(
  BoardInput< 9 > const & board_input,
  LocalInput const & local_input
) const {

  std::string const name1 = "in1";
  std::string const name2 = "in2";
  std::string const outname = "out/Softmax";

  platform::Size const ninputs = input_names.size();
  platform::Size const noutputs = 1;

  std::unique_ptr<TF_Output[]> inputs(new TF_Output[ninputs]);
  std::unique_ptr<TF_Tensor *[]> input_values(new TF_Tensor *[ninputs]);
  std::unique_ptr<TF_Output[]> outputs(new TF_Output[noutputs]);
  std::unique_ptr<TF_Tensor *[]> output_values(new TF_Tensor *[noutputs]);

  for( platform::Size ii = 1; ii <= input_tensors.size(); ++ii ){
    input_values.get()[ ii - 1 ] = input_tensors[ ii ].raw_tensor_ptr();
    debug_assert( input_tensors[ ii ].raw_tensor_ptr() != nullptr );

    TF_Operation * const op = TF_GraphOperationByName( graph_, input_names[ ii ].c_str() );
    inputs.get()[ ii - 1 ] = TF_Output{ op, 0 };
    debug_assert( op != nullptr );
  }

  {
    TF_Operation * const out_op = TF_GraphOperationByName( graph_, output_name.c_str() );
    outputs.get()[0] = TF_Output{ out_op, 0 };
    debug_assert( out_op != nullptr);
  }

  output_values.get()[0] = output_tensor.raw_tensor_ptr();

  std::chrono::time_point<std::chrono::system_clock> const starttime( ROSETTA_TENSORFLOW_CLOCK::now() );
  TF_SessionRun( session_, nullptr, inputs.get(), input_values.get(), ninputs,
    outputs.get(), output_values.get(), noutputs, nullptr, 0,
    nullptr, status_ );
  std::chrono::time_point<std::chrono::system_clock> const endtime( ROSETTA_TENSORFLOW_CLOCK::now() );
  runtime = endtime - starttime;

  //Check that the run was okay:
  if ( TF_GetCode(status_) != TF_OK ) {
    utility_exit_with_message( "Unable to evaluate TensorFlow session: " + std::string( TF_Message(status_) ) );
  }

  output_tensor.set_tensor( output_values.get()[0] );

}
