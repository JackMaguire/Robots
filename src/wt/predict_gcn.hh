#pragma once

#include <cppflow/ops.h>
#include <cppflow/model.h>

#include <iostream>
#include "robots.hh"

struct Prediction {
  int dx = -2;
  int dy = -2;
};

struct GCN {
  GCN() :
    model("/saved_models/2BM.6")
  {
    /*for ( std::string const & s : model.get_operations() ){
      std::cout << s << std::endl;
    }*/
  }

  template< typename GAME >
  Prediction
  predict( GAME const & game ){
   
    auto output = model(
      {
	{"serving_default_X_in:0", input_X},
	  {"serving_default_A_in:0", input_A},
	    {"serving_default_E_in:0", input_E},
      },
      {"StatefulPartitionedCall"});

    Prediction pred;
    return pred;
  }

  cppflow::model model;
};
