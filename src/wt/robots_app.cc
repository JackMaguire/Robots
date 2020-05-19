#include <Wt/WApplication.h>
#include <Wt/WBootstrapTheme.h>

#include <memory>
#include <iostream>
#include <string>
#include <vector>

#include "wt/board_widget"

using namespace Wt;

std::unique_ptr< Wt::WApplication >
createApplication( Wt::WEnvironment const & env ) {

  std::unique_ptr< Wt::WApplication > app = Wt::cpp14::make_unique< Wt::WApplication >( env );
  app->setTitle( "Deep Chase" );
  app->root()->setStyleClass( "root" );
  app->root()->addWidget( Wt::cpp14::make_unique< BoardWidget >( );
  app->setCssTheme( "polished" );
  app->setTheme( std::make_shared< Wt::WBootstrapTheme >() ) ;

  return app;
}

int main( int argc, char **argv )
{
  return WRun( argc, argv, &createApplication );
}
