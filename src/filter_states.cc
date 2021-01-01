// g++ filter_states.cc -std=c++2a -o filter_states -Wall -pedantic -Wshadow
// g++ filter_states.cc -std=c++2a -o filter_states -Wall -pedantic -Wshadow -g -D_GLIBCXX_DEBUG

#include "robots.hh"
//#include "run_nn.hh"

#include <array>

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

int main(){

  //example input line:
  //000000000000000000000000000000000000000000000000030000000000000000000000000000000000000000000000000000000010030000000000000000000003011100000000000000000000000001100030000000000000000000000000000000000000000000000000000000000001000300000000000000000001000301000000000000000000000003000103100000000000000000000312110133130000000000000000000003000000001000000000000000000000000000000000000000000000000001100000000000000000000000000001010000330000000000003000000000000300300000000000000000000303300000300033000000000000000001011000301000000000000000000001000000000000000000000000000001010100131000000000000000000001000100300000000000000000000001000000330000000000000000000011000100001000000000000000000000100000031000000000000000000000000101000000000000000000000000100000000000000000000000000001000100300000000000000000000000000000100000000000000000000013000000301000000000000000000000000000110000000000000000000001000100000000000000000000000001010000000000000000000000000003000100310000000000000000000001000100000000000000000000000001000101310000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000300000000000000000000000000000000300000000000000000000000000000000000000000000000000000000000000000000000000000000000030000000000000000000000000000000000000000000000000000000000000000000000000,6,17,10
  //Board,tele,level,move

  for( std::string line; std::getline(std::cin, line); ){
    std::vector< std::string > tokens;
    std::stringstream ss( line );
    while( ss.good() ){
      std::string substr;
      std::getline( ss, substr, ',' );
      tokens.push_back( substr );
    }
    //std::cout << tokens.size() << std::endl;

    if( tokens.size() < 4 ) continue;

    int const move = std::stoi( tokens[3] );
    switch( move ){
    case int( Key::NONE ):
    case int( Key::T ):
    case int( Key::SPACE ):
    case int( Key::DELETE ):
    case int( Key::R ):
      continue;
    default:
      break;
    }
    //std::cout << move << std::endl;

    Board b;
    b.load_from_stringified_representation( tokens[0] );

    std::array< std::array< ForecastResults, 3 >, 3 > const forecasts =
      forcast_all_moves( b );

    int n_options = 0;
    for( auto const & i : forecasts )
      for( auto const & j : i )
	if( j.legal ) ++n_options;

    //std::cout << n_options << std::endl;
    if( n_options < 2 ) continue;

    std::cout << line << '\n';
  }

  //std::cout << std::endl;
}
