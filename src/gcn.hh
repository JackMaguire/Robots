// g++ gcn.cc -std=c++2a -o gcn -Wall -pedantic -Wshadow
// g++ gcn.cc -std=c++2a -o gcn -Wall -pedantic -Wshadow -g -D_GLIBCXX_DEBUG

#include "robots.hh"
//#include "run_nn.hh"

#include <array>
#include <optional>

#include <string>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <cstdlib> //rand()

#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

constexpr int Q_key = 113;  //
constexpr int W_key = 119; //capitol is 87
constexpr int E_key = 101; //
constexpr int A_key = 97;  //65
constexpr int S_key = 115; //83
constexpr int D_key = 100; //68
constexpr int Z_key = 122;
constexpr int X_key = 120;
constexpr int C_key = 99;

constexpr int T_key = 116;
constexpr int SPACEBAR = 32;

constexpr int R_key = 114; //

constexpr int delete_key = 127; //

enum class
Key {
     NONE = 0,
     Q,
     W,
     E,
     A,
     S,
     D,
     Z,
     X,
     C,
     T,
     SPACE,
     DELETE,
     R
};

Key
parse_int( int const command ){
  switch( command ){

  case A_key: return Key::A;
  case C_key: return Key::C;
  case D_key: return Key::D;
  case E_key: return Key::E;
  case Q_key: return Key::Q;
  case S_key: return Key::S;
  case W_key: return Key::W;
  case X_key: return Key::X;
  case Z_key: return Key::Z;
  case T_key: return Key::T;
  case SPACEBAR: return Key::SPACE;
  case delete_key: return Key::DELETE;
  case R_key: return Key::R;
  default: return Key::NONE;
  };
  return Key::NONE;
}

struct Options {
  int N;
};

struct NodeCandidate {
  NodeCandidate( int x, int y, Occupant o, Position const & human_position ){
    pos.x = x;
    pos.y = y;
    occ = o;
    distance = human_position.distance( pos );
  }
  NodeCandidate( Position const & p, Occupant o, Position const & human_position ){
    pos = p;
    occ = o;
    distance = human_position.distance( pos );
  }
  Position pos;
  Occupant occ;
  float distance;
};

std::vector< NodeCandidate >
get_local_candidates( Board const & board ){
  std::vector< NodeCandidate > all;
  all.reserve( 9 );
  Position const h = board.human_position();
  
  Position p;
  for( p.y = h.y-1; p.y <= h.y+1; ++p.y ){
    for( p.x = h.x-1; p.x <= h.x+1; ++p.x ){
      if( ! board.position_is_in_bounds( p ) ){
	all.emplace_back( p, Occupant::OOB, h );
      } else {
	all.emplace_back( p, board.cell( p ), h );
      }
    }
  }

  assert( all.size() == 9 );

  return all;
}


std::vector< NodeCandidate >
get_top_candidates( Board const & board, Options const & options ){
  std::vector< NodeCandidate > all;
  all.reserve( 100 );
  Position const human_position = board.human_position();

  //Do the 4 OOB:
  all.emplace_back( -1, human_position.y,     Occupant::OOB, human_position );
  all.emplace_back( WIDTH, human_position.y,  Occupant::OOB, human_position );
  all.emplace_back( human_position.x, -1,     Occupant::OOB, human_position );
  all.emplace_back( human_position.x, HEIGHT, Occupant::OOB, human_position );
  
  int nskipped = 0;
  Position p;
  for( p.x = 0; p.x < WIDTH; ++p.x ){
    for( p.y = 0; p.y < HEIGHT; ++p.y ){
      if( abs( p.x - human_position.x ) <= 1 && abs( p.y - human_position.y ) <= 1 ){
	++nskipped;
	continue;
      }

      switch( board.cell( p ) ){
      case Occupant::EMPTY:
	continue;
      case Occupant::HUMAN:
      case Occupant::OOB:
	assert( false );
	break;
      case Occupant::ROBOT:
      case Occupant::FIRE:
	all.emplace_back( p, board.cell( p ), human_position );
	break;
      }
    }
  }

  assert( nskipped == 9 );

  std::sort( all.begin(), all.end(), 
    []( NodeCandidate const & a, NodeCandidate const & b ) -> bool { 
      return a.distance < b.distance; 
    });

  int const max_size = options.N - 9;
  if( all.size() > max_size ){
    all.resize( max_size );
  }

  return all;
}

using Forecasts = std::array< std::array< ForecastResults, 3 >, 3 >;

constexpr int F = 3;  // TODO
constexpr int Fx = 2; // TODO
constexpr int S = 1;  // TODO

// X: (N,F)
using Xvec = std::vector< std::array< float, F > >;

// X: (N,Fx)
using X2vec = std::vector< std::array< float, Fx > >;

// A: (N,N)
using Avec = std::vector< std::vector< float > >;

// E: (N,N,S)
using Evec = std::vector< std::vector< std::array< float, S > > >;

// O: (9)
using Ovec = std::array< float, 9 >;

struct Data {
  Xvec X;
  X2vec X2;
  Avec A;
  Evec E;
  Ovec out;
};

std::array< float, F >
calcF( unsigned int const i, NodeCandidate const & c, Forecasts const & forecasts ){
  std::array< float, F > values; //zero initialized
  switch( c.occ ) {
    case Occupant::EMPTY:
      values[ 0 ] = 0;
      values[ 1 ] = 0;
      break;
    case Occupant::ROBOT:
      values[ 0 ] = 0;
      values[ 1 ] = 1;
      break;
    case Occupant::HUMAN:
      values[ 0 ] = 1;
      values[ 1 ] = 0;
      break;
    case Occupant::FIRE:
      values[ 0 ] = 0;
      values[ 1 ] = -1;
      break;
    case Occupant::OOB:
      values[ 0 ] = -1;
      values[ 1 ] = 0;
      break;
  }

  //[2]: distance from human
  if( c.occ == Occupant::HUMAN ){
    values[ 2 ] = -3.0;
  } else {
    values[ 2 ] = log( c.distance );
  }

  return values;
}

std::array< float, Fx >
calcFx( unsigned int const i, NodeCandidate const & c, Forecasts const & forecasts ){
  std::array< float, Fx > values; //zero initialized

  if( c.distance == 0 ){
    //human
    values[ 0 ] = 0;
    values[ 1 ] = ( total_n_robots > 0 ? log( total_n_robots ) : -1.0 );
    values[ 2 ] = legal;
  } else if( c.distance == 1.0 ) {
    // cardinal
    // TODO assert cardinal
    values[ 0 ] = 1.0;
    values[ 1 ] = ( n_robots > 0 ? log( n_robots ) : -1.0 );
    values[ 2 ] = legal;
  } else {
    // diagonal
    // TODO assert diagonal
    values[ 0 ] = -1.0;
    values[ 1 ] = ( n_robots > 0 ? log( n_robots ) : -1.0 );
    values[ 2 ] = legal;
  }

  return values;
}


Data
make_data( std::string const & line, Options const & options ){

  //example input line:
  //000000000000000000000000000000000000000000000000030000000000000000000000000000000000000000000000000000000010030000000000000000000003011100000000000000000000000001100030000000000000000000000000000000000000000000000000000000000001000300000000000000000001000301000000000000000000000003000103100000000000000000000312110133130000000000000000000003000000001000000000000000000000000000000000000000000000000001100000000000000000000000000001010000330000000000003000000000000300300000000000000000000303300000300033000000000000000001011000301000000000000000000001000000000000000000000000000001010100131000000000000000000001000100300000000000000000000001000000330000000000000000000011000100001000000000000000000000100000031000000000000000000000000101000000000000000000000000100000000000000000000000000001000100300000000000000000000000000000100000000000000000000013000000301000000000000000000000000000110000000000000000000001000100000000000000000000000001010000000000000000000000000003000100310000000000000000000001000100000000000000000000000001000101310000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000300000000000000000000000000000000300000000000000000000000000000000000000000000000000000000000000000000000000000000000030000000000000000000000000000000000000000000000000000000000000000000000000,6,17,10
  //Board,tele,level,move

  std::vector< std::string > tokens;
  std::stringstream ss( line );
  while( ss.good() ){
    std::string substr;
    std::getline( ss, substr, ',' );
    tokens.push_back( substr );
  }

  if( tokens.size() != 4 ){
    assert( false );
  }

  int const move = std::stoi( tokens[3] );
  switch( move ){
  case int( Key::NONE ):
  case int( Key::T ):
  case int( Key::SPACE ):
  case int( Key::DELETE ):
  case int( Key::R ):
    assert( false );
  default:
    break;
  }

  Board b;
  b.load_from_stringified_representation( tokens[0] );

  std::array< std::array< ForecastResults, 3 >, 3 > const forecasts =
    forcast_all_moves( b );
  
  std::vector< NodeCandidate > const all_elements =
    [&](){
      std::vector< NodeCandidate > local_candidates = get_local_candidates( b );
      std::vector< NodeCandidate > far_candidates = get_top_candidates( b, options );
      std::vector< NodeCandidate > all_elements;
      all_elements.reserve( options.N );
      all_elements.insert( all_elements.end(),
	local_candidates.begin(), local_candidates.end() );
      all_elements.insert( all_elements.end(),
	far_candidates.begin(), far_candidates.end() );
      return all_elements;
    }();

  //TODO init arrays and zero out

  Data data;
  
  for( unsigned int i = 0; i < all_elements.size(); ++i ){
    //X
    data.X[ i ] = calcF( i, all_elements[ i ], forecasts );

    if( i < 9 ){
      data.X2[ i ] = calcFx( i, all_elements[ i ], forecasts );
    }

    for( unsigned int j = i+1; j < all_elements.size(); ++j ){
      bool const has_e = has_edge( all_elements, i, j );
      if( has_e ){
	//A
	data.A[ i ][ j ] = 1;
	data.A[ j ][ i ] = 1;

	//E
	calcS( all_elements, i, j, data.E[ i ][ j ], data.E[ j ][ i ] )
      }

    }
  }

  return data;
}


/*int main(){


  for( std::string line; std::getline(std::cin, line); ){

    //std::cout << move << std::endl;

    int n_options = 0;
    for( auto const & i : forecasts ){
      for( auto const & j : i ){
	if( j.legal ) ++n_options;
	if( j.cascade_safe ) continue;
      }
    }

    //std::cout << n_options << std::endl;
    if( n_options < 2 ) continue;

    std::cout << line << '\n';
  }

  //std::cout << std::endl;
}
*/

int main(){

}
