#pragma once

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
#include <algorithm> //min

#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <exception>

struct IllegalMoveException : public std::exception
{
  IllegalMoveException(){
    std::cout << "Bad Move!" << std::endl;
  }

};

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
  unsigned int N;
};

struct NodeCandidate {
  NodeCandidate() = default;
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
  for( p.y = h.y+1; p.y >= h.y-1; --p.y ){ //QWE ASD ZXC
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
  {
    Position const p({ -1, human_position.y });
    if( abs( p.x - human_position.x ) > 1 || abs( p.y - human_position.y ) > 1 ){
      all.emplace_back( p, Occupant::OOB, human_position ); 
    }
  }
  {
    Position const p({ WIDTH, human_position.y });
    if( abs( p.x - human_position.x ) > 1 || abs( p.y - human_position.y ) > 1 ){
      all.emplace_back( p, Occupant::OOB, human_position ); 
    }
  }
  {
    Position const p({ human_position.x, -1 });
    if( abs( p.x - human_position.x ) > 1 || abs( p.y - human_position.y ) > 1 ){
      all.emplace_back( p, Occupant::OOB, human_position ); 
    }
  }
  {
    Position const p({ human_position.x, HEIGHT });
    if( abs( p.x - human_position.x ) > 1 || abs( p.y - human_position.y ) > 1 ){
      all.emplace_back( p, Occupant::OOB, human_position ); 
    }
  }
  
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

  assert( nskipped >= 4 );
  assert( nskipped <= 9 );

  std::sort( all.begin(), all.end(), 
    []( NodeCandidate const & a, NodeCandidate const & b ) -> bool { 
      return a.distance < b.distance; 
    });

  unsigned int const max_size = options.N - 9;
  if( all.size() > max_size ){
    all.resize( max_size );
  }

  return all;
}

using Forecasts = std::array< std::array< ForecastResults, 3 >, 3 >;

constexpr int F = 3;
constexpr int Fx = 3;
constexpr int S = 5;

// X: (N,F)
using Xvec = std::vector< std::array< float, F > >;

// X: (N,Fx)
using X2vec = std::vector< std::array< float, Fx > >;

using X12vec = std::vector< std::array< float, F + Fx > >;

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

  X12vec mergeXs() const {
    X12vec combinedX;
    combinedX.resize( X.size() );
    for( uint i = 0; i < X.size(); ++i ){
      for( uint j = 0; j < F; ++j ){
	combinedX[ i ][ j ] = X[ i ][ j ];
      }
      for( uint j = 0; j < Fx; ++j ){
	combinedX[ i ][ F+j ] = X2[ i ][ j ];
      }      
    }
    return combinedX;
  }
};

std::array< float, F >
calcF( NodeCandidate const & c ){
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

template< int DX, int DY >
unsigned int
n_robots_in_corner( Board const & board ) {
  unsigned int count = 0;
  Position const h = board.human_position();
  Position p;
  for( p.x = h.x + DX; p.x >= 0 && p.x < WIDTH; p.x += DX ){
    for( p.y = h.y + DY; p.y >= 0 && p.y < HEIGHT; p.y += DY ){
      if( board.cell( p ) == Occupant::ROBOT ){
	++count;
      }
    }
  }
  return count;
}

template< int DX, int DY >
unsigned int
n_robots_in_line( Board const & board ) {
  unsigned int count = 0;
  Position const h = board.human_position();
  Position p;
  p.x = h.x + DX;
  p.y = h.y + DY;
  static_assert( DX == 0 || DY == 0, "MESSAGE" );
  while( board.position_is_in_bounds( p ) ){
    if( board.cell( p ) == Occupant::ROBOT ){
      ++count;
    }
    p.x += DX;
    p.y += DY;
  }
  return count;
}


std::array< float, Fx >
calcFx(
  unsigned int const i,
  Forecasts const & forecasts,
  Board const & board,
  int const n_safe_tele
){
  std::array< float, Fx > values = {}; //zero initialized

  constexpr float CARDINAL = 1.0;
  constexpr float DIAGONAL = -1.0;

  auto && transform =
    []( int const n_robots ) -> float {
      auto const trans = ( n_robots > 0 ? log( n_robots ) : -1.0 );
      //std::cout << "!!!" << n_robots << " " << trans << std::endl;
      return trans;
    };

  if( i == 0 ){ //Q
    values[ 0 ] = DIAGONAL;
    auto const n_robots = n_robots_in_corner< -1, 1 >( board );
    values[ 1 ] = transform( n_robots );
    values[ 2 ] = forecasts[ 0 ][ 2 ].legal ? 1.0 : 0.0;
    return values;
  }

  if( i == 1 ){ //W
    values[ 0 ] = CARDINAL;
    auto const n_robots = n_robots_in_line< 0, 1 >( board );
    values[ 1 ] = transform( n_robots );
    values[ 2 ] = forecasts[ 1 ][ 2 ].legal ? 1.0 : 0.0;    
    return values;
  }

  if( i == 2 ){ //E
    values[ 0 ] = DIAGONAL;
    auto const n_robots = n_robots_in_corner< 1, 1 >( board );
    values[ 1 ] = transform( n_robots );
    values[ 2 ] = forecasts[ 2 ][ 2 ].legal ? 1.0 : 0.0;    
    return values;
  }

  if( i == 3 ){ //A
    values[ 0 ] = CARDINAL;
    auto const n_robots = n_robots_in_line< -1, 0 >( board );
    values[ 1 ] = transform( n_robots );
    values[ 2 ] = forecasts[ 0 ][ 1 ].legal ? 1.0 : 0.0;    
    return values;
  }

  if( i == 4 ){ //S
    values[ 0 ] = 0;
    values[ 1 ] = ( float(n_safe_tele) - 5.0 ) / 2.0;
    values[ 2 ] = forecasts[ 1 ][ 1 ].legal ? 1.0 : 0.0;    
    return values;
  }

  if( i == 5 ){ //D
    values[ 0 ] = CARDINAL;
    auto const n_robots = n_robots_in_line< 1, 0 >( board );
    values[ 1 ] = transform( n_robots );
    values[ 2 ] = forecasts[ 2 ][ 1 ].legal ? 1.0 : 0.0;    
    return values;
  }

  if( i == 6 ){ //Z
    values[ 0 ] = DIAGONAL;
    auto const n_robots = n_robots_in_corner< -1, -1 >( board );
    values[ 1 ] = transform( n_robots );
    values[ 2 ] = forecasts[ 0 ][ 0 ].legal ? 1.0 : 0.0;    
    return values;
  }

  if( i == 7 ){ //X
    values[ 0 ] = CARDINAL;
    auto const n_robots = n_robots_in_line< 0, -1 >( board );
    values[ 1 ] = transform( n_robots );
    values[ 2 ] = forecasts[ 1 ][ 0 ].legal ? 1.0 : 0.0;    
    return values;
  }

  if( i == 8 ){ //C
    values[ 0 ] = DIAGONAL;
    auto const n_robots = n_robots_in_corner< 1, -1 >( board );
    values[ 1 ] = transform( n_robots );
    values[ 2 ] = forecasts[ 2 ][ 0 ].legal ? 1.0 : 0.0;    
    return values;
  }

  return values;
}

bool
has_edge(
  std::vector< NodeCandidate > const & all_elements,
  unsigned int const i,
  unsigned int const j
){
  if( all_elements[ i ].occ == Occupant::HUMAN ) return true;
  if( all_elements[ j ].occ == Occupant::HUMAN ) return true;

  constexpr double distance_cutoff = 7.5;
  auto const dist = all_elements[ i ].pos.distance( all_elements[ j ].pos );
  return dist <= distance_cutoff;
}

void
calcE(
  std::vector< NodeCandidate > const & all_elements,
  unsigned int const i,
  unsigned int const j,
  std::array< float, S > & Eij,
  std::array< float, S > & Eji
) {
  auto const dist = all_elements[ i ].pos.distance( all_elements[ j ].pos );
  
  // [0] : distance
  Eij[ 0 ] = Eji[ 0 ] = dist - 3.0;

  Position dpos = all_elements[ i ].pos - all_elements[ j ].pos;
  dpos.x = abs( dpos.x );
  dpos.y = abs( dpos.y );
  int const min = std::min( dpos.x, dpos.y );
  int const max = std::max( dpos.x, dpos.y );
  assert( min + max == dpos.x + dpos.y );
  
  // [ 1 ]: smaller
  Eij[ 1 ] = Eji[ 1 ] = min - 2; //-2 offset

  // [ 2 ]: larger
  Eij[ 2 ] = Eji[ 2 ] = max - 2; //-2 offset

  // [ 3 ]: ratio
  Eij[ 3 ] = Eji[ 3 ] = (4.0*float(min)/float(max)) - 2.0;

  // [ 4 ] : which one is closer to the human?
  if( all_elements[ i ].distance < all_elements[ j ].distance ){
    Eij[ 4 ] = 1;
    Eji[ 4 ] = -1;
  } else if( all_elements[ j ].distance < all_elements[ i ].distance ){
    Eij[ 4 ] = -1;
    Eji[ 4 ] = 1;
  } else {
    //Same distance
    Eij[ 4 ] = Eji[ 3 ] = 0;
  }
}

template< typename T >
void
zero_out( T & t ){
  std::fill( t.begin(), t.end(), 0 );
}

std::vector< NodeCandidate >
get_candidates(
  Board const & b,
  Options const & options
){  
  std::vector< NodeCandidate > local_candidates = get_local_candidates( b );
  std::vector< NodeCandidate > far_candidates = get_top_candidates( b, options );
  std::vector< NodeCandidate > all_elements_i;
  all_elements_i.reserve( options.N );
  all_elements_i.insert( all_elements_i.end(),
    local_candidates.begin(), local_candidates.end() );
  all_elements_i.insert( all_elements_i.end(),
    far_candidates.begin(), far_candidates.end() );
  return all_elements_i;
}


Data
make_data(
  Board const & b,
  int const n_safe_tele,
  int const move,
  Options const & options
){

  std::array< std::array< ForecastResults, 3 >, 3 > const forecasts =
    forcast_all_moves( b );

  Data data;  //TODO init arrays and zero out
  data.X.resize( options.N );
  for( auto & x : data.X )
    zero_out( x );
  data.X2.resize( options.N );
  for( auto & x : data.X2 )
    zero_out( x );
  data.A.resize( options.N );
  for( auto & a : data.A )
    a.resize( options.N );
  data.E.resize( options.N );
  for( auto & e : data.E )
    e.resize( options.N );
  zero_out( data.out );

  switch( move ){
  case int( Key::NONE ):
    break;
  case int( Key::T ):
  case int( Key::SPACE ):
  case int( Key::DELETE ):
  case int( Key::R ):
    assert( false );
  case int( Key::Q ):
    if( ! forecasts[ 0 ][ 2 ].legal ) throw IllegalMoveException();
    data.out[ 0 ] = 1.0;
    break;
  case int( Key::W ):
    if( ! forecasts[ 1 ][ 2 ].legal ) throw IllegalMoveException();
    data.out[ 1 ] = 1.0;
    break;
  case int( Key::E ):
    if( ! forecasts[ 2 ][ 2 ].legal ) throw IllegalMoveException();
    data.out[ 2 ] = 1.0;
    break;
  case int( Key::A ):
    if( ! forecasts[ 0 ][ 1 ].legal ) throw IllegalMoveException();
    data.out[ 3 ] = 1.0;
    break;
  case int( Key::S ):
    if( ! forecasts[ 1 ][ 1 ].legal ) throw IllegalMoveException();
    data.out[ 4 ] = 1.0;
    break;
  case int( Key::D ):
    if( ! forecasts[ 2 ][ 1 ].legal ) throw IllegalMoveException();
    data.out[ 5 ] = 1.0;
    break;
  case int( Key::Z ):
    if( ! forecasts[ 0 ][ 0 ].legal ) throw IllegalMoveException();
    data.out[ 6 ] = 1.0;
    break;
  case int( Key::X ):
    if( ! forecasts[ 1 ][ 0 ].legal ) throw IllegalMoveException();
    data.out[ 7 ] = 1.0;
    break;
  case int( Key::C ):
    if( ! forecasts[ 2 ][ 0 ].legal ) throw IllegalMoveException();
    data.out[ 8 ] = 1.0;
    break;
  default:
    break;
  }

  
  std::vector< NodeCandidate > const all_elements =
    get_candidates( b, options );

  for( unsigned int i = 0; i < all_elements.size(); ++i ){
    //std::cout << "Element " << i << " " << all_elements[i].pos.x << " " << " " << all_elements[i].pos.y	<< " " << int(all_elements[i].occ) << " " << all_elements[i].distance << std::endl;
    
    //X
    data.X[ i ] = calcF( all_elements[ i ] );

    if( i < 9 ){
      data.X2[ i ] = calcFx( i, forecasts, b, n_safe_tele );
      //std::cout << "??? " << data.X2[ i ][ 2 ] << std::endl;
    } else {
      std::fill( data.X2[ i ].begin(), data.X2[ i ].end(), -5 );
    }

    for( unsigned int j = i+1; j < all_elements.size(); ++j ){
      bool const has_e = has_edge( all_elements, i, j );
      if( has_e ){
	//A
	data.A[ i ][ j ] = 1;
	data.A[ j ][ i ] = 1;

	//E
	calcE( all_elements, i, j, data.E[ i ][ j ], data.E[ j ][ i ] );
      }

    }
  }

  return data;
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

  int const n_safe_tele = std::stoi( tokens[1] );
  int const move = std::stoi( tokens[3] );

  Board b;
  b.load_from_stringified_representation( tokens[0] );
  
  return make_data( b, n_safe_tele, move, options );
}


#ifdef MAIN

int main(){


  //for( std::string line; std::getline(std::cin, line); ){

    std::string const line = "000000000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000000000000000230000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000,0,1,9";
    Options const options = { 25 };
    auto const data = make_data( line, options );

    std::cout << "X" << std::endl;
    for( auto const & x : data.X ){
      for( auto const & i : x )
	std::cout << i << " ";
      std::cout << std::endl;
    }
    std::cout << std::endl;

    std::cout << "X2" << std::endl;
    for( auto const & x : data.X2 ){
      for( auto const & i : x )
	std::cout << i << " ";
      std::cout << std::endl;
    }
    std::cout << std::endl;

    std::cout << "A" << std::endl;
    for( auto const & x : data.A ){
      for( auto const & i : x )
	std::cout << i << " ";
      std::cout << std::endl;
    }
    std::cout << std::endl;

    std::cout << "E" << std::endl;
    for( auto const & x : data.E ){
      for( auto const & i : x ) {
	for( auto const & j : i ) {
	  std::cout << j << " ";
	}
	std::cout << std::endl;
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;

    std::cout << "Out" << std::endl;
    for( auto const & x : data.out ){
      std::cout << x << " ";
    }
    std::cout << std::endl;

  //}

  //std::cout << std::endl;
}


#endif
