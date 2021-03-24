// g++ benchmark_recursion.cc -std=c++2a -o benchmark_recursion -Wall -pedantic -Wshadow
// g++ benchmark_recursion.cc -std=c++2a -o benchmark_recursion -Wall -pedantic -Wshadow -g -D_GLIBCXX_DEBUG

#include "robots.hh"
#include "recursion.hh"

#include <array>

#include <string>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <cstdlib> //rand()

int main(){

  bool any_cascade = false;

  for( int i = 0; i < 5; ++i ){
    RobotsGame< NullVisualizer, false, 0 > game( 1, 0 );
    auto result = recursive_search_for_cascade< 10 >( game.board() );
    any_cascade |= result.cascade;
  }
  
  std::cout << any_cascade << std::endl;

}
