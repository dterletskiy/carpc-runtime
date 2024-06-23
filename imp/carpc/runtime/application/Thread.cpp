#include "carpc/runtime/application/Thread.hpp"
#include "SystemEventConsumer.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "Srv"



using namespace carpc::application;



Thread::Thread( const Configuration& config )
   : ThreadBase( config.m_name, config.m_wd_timeout )
   , m_components( )
   , m_component_creators( config.m_component_creators )
{
   SYS_VRB( "'%s': created", m_name.c_str( ) );
}

Thread::~Thread( )
{
   SYS_VRB( "'%s': destroyed", m_name.c_str( ) );
}

void Thread::thread_loop( )
{
   SYS_INF( "'%s': enter", m_name.c_str( ) );
   m_started.store( true );

   SystemEventConsumer system_event_consumer( *this );

   // Creating components
   for( auto creator : m_component_creators )
      m_components.emplace_back( creator( ) );

   while( m_started.load( ) )
   {
      async::IAsync::tSptr p_async = get_async( );
      SYS_VRB( "'%s': processing async object (%s)",
            m_name.c_str( ),
            p_async->signature( )->dbg_name( ).c_str( )
         );
      notify_consumers( p_async );
   }

   // Destroying components
   m_components.clear( );

   SYS_INF( "'%s': exit", m_name.c_str( ) );
}

bool Thread::start( )
{
   SYS_INF( "'%s': starting", m_name.c_str( ) );
   bool result = m_thread.run( m_name.c_str( ) );
   if( false == result )
   {
      SYS_ERR( "'%s': can't be started", m_name.c_str( ) );
   }

   return result;
}

void Thread::stop( )
{
   SYS_INF( "'%s': stopping", m_name.c_str( ) );
   m_started.store( false );
}

void Thread::boot( const std::string& message )
{
   SYS_INF( "'%s': booting", m_name.c_str( ) );
   for( auto component : m_components )
      if( component->is_root( ) )
         component->process_boot( message );
}

void Thread::shutdown( const std::string& message )
{
   SYS_INF( "'%s': shutting down components", m_name.c_str( ) );
   auto blocker = callback::tBlockerRoot::create(
         [ this ]( )
         {
            SYS_INF( "'%s': shutting down", m_name.c_str( ) );
            stop( );
         }
      );
   for( auto component : m_components )
      component->process_shutdown( blocker );
}
