#include "carpc/runtime/application/RootComponent.hpp"
#include "carpc/runtime/events/Events.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "RootComponent"



using namespace carpc::application;



RootComponent::RootComponent( const std::string& name )
   : IComponent( name )
{
   SYS_DBG( "Created: %s", m_name.c_str( ) );
}

RootComponent::~RootComponent( )
{
   SYS_DBG( "Destroyed: %s", m_name.c_str( ) );
}

void RootComponent::shutdown( const std::string& message )
{
   events::system::System::Event::create( { events::system::eID::shutdown } )->
      data( { "shutdown application" } )->send( );
}
