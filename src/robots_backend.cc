// g++ robots_backend.cc -std=c++2a -o robots_backend -Wall -pedantic -Wshadow
// g++ robots_backend.cc -std=c++2a -o robots_backend -Wall -pedantic -Wshadow -g -D_GLIBCXX_DEBUG

#include "robots.hh"
//#include "run_nn.hh"

#include <array>

#include <string>
#include <fstream>
#include <stdio.h>
#include <iostream>

#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define OUTPUT_TRAINING_DATA_DIR "training_data/"

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

struct ConsolePiper {
  static void show( Board const & b ){
    std::cout << "UPDATE " << b.get_stringified_representation() << " " << b.get_safe_moves() << std::endl;
  }
};

#ifdef OUTPUT_TRAINING_DATA_DIR
struct Logger {

  ~Logger(){
    //Dump to file
    std::string const filename =
      OUTPUT_TRAINING_DATA_DIR + std::to_string( rand() );
      std::ofstream myfile;
      myfile.open ( filename );
      myfile << output.str();
      myfile.close();
  }

  template< typename GAME >
  void
  log( GAME const & game, Key const key ){
    output << game.board().get_stringified_representation()
    << ',' << game.n_safe_teleports_remaining()
    << ',' << game.round()
    << ',' << int( key ) << std::endl;
    // << ',' << game.board().get_safe_moves() << std::endl;
  }

  std::stringstream output;
};  
#endif

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

  struct termios oldSettings, newSettings;

  tcgetattr( fileno( stdin ), &oldSettings );
  newSettings = oldSettings;
  newSettings.c_lflag &= (~ICANON & ~ECHO);
  tcsetattr( fileno( stdin ), TCSANOW, &newSettings );    

  RobotsGame< ConsolePiper > game( 60, 10 );

#ifdef OUTPUT_TRAINING_DATA_DIR
      Logger logger;
#endif

  //listen for keys
  while( true ){
    fd_set set;
    struct timeval tv;

    tv.tv_sec = 10;
    tv.tv_usec = 0;

    FD_ZERO( &set );
    FD_SET( fileno( stdin ), &set );

    int const res = select( fileno( stdin )+1, &set, NULL, NULL, &tv );
    if( res > 0 ){
      char c;
      read( fileno( stdin ), &c, 1 );
      int const command = int( c );
      // std::cout << command << std::endl;
      Key const key = parse_int( command );

#ifdef OUTPUT_TRAINING_DATA_DIR
      logger.log( game, key );
#endif

      GameOverBool gameover = false;
      switch( key ){
	// Zooming:
      case Key::Q:
	gameover = game.move_human( -1, 1 );
	break;
      case Key::W:
	gameover = game.move_human( 0, 1 );
	break;
      case Key::E:
	gameover = game.move_human( 1, 1 );
	break;
      case Key::A:
	gameover = game.move_human( -1, 0 );
	break;
      case Key::S:
	gameover = game.move_human( 0, 0 );
	break;
      case Key::D:
	gameover = game.move_human( 1, 0 );
	break;
      case Key::Z:
	gameover = game.move_human( -1, -1 );
	break;
      case Key::X:
	gameover = game.move_human( 0, -1 );
	break;
      case Key::C:
	gameover = game.move_human( 1, -1 );
	break;
      case Key::T:
	gameover = game.teleport();
	break;
      case Key::SPACE:
	gameover = game.cascade();
	break;
      case Key::R:
	std::cout << "R" << std::endl;
	//predict_ai( game );
	break;
      default:
	break;
      }

      if( gameover || key == Key::DELETE ){
	break;
      }
    }
  }

  std::cout << "EXIT" << std::endl;
}
