#pragma once

#include <cppflow/ops.h>
#include <cppflow/model.h>

struct Prediction {
  int dx = -2;
  int dy = -2;
};

template< typename GAME >
inline
Prediction
predict( GAME const & ){
  Prediction pred;

  

  return pred;
}
