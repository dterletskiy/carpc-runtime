#include "carpc/runtime/comm/async/event/Event.hpp"
#include "carpc/runtime/application/Process.hpp"
#include "carpc/runtime/application/Thread.hpp"
#include "SystemEventConsumer.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "Srv"



using namespace carpc::application;



Thread::Thread( const Configuration& config )
   : IThread( config.m_name, config.m_wd_timeout )
   , m_thread( std::bind( &Thread::thread_loop, this ) )
   , m_event_queue( configuration::current( ).max_priority, config.m_name )
   , m_consumers_map( config.m_name )
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
      carpc::async::IAsync::tSptr p_event = get_event( );
      SYS_VRB( "'%s': processing event (%s)", m_name.c_str( ), p_event->signature( )->dbg_name( ).c_str( ) );
      notify( p_event );
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

bool Thread::insert_event( const carpc::async::IAsync::tSptr p_event )
{
   if( false == m_started.load( ) )
   {
      SYS_WRN( "'%s': is not started", m_name.c_str( ) );
      return false;
   }

   if( false == is_subscribed( p_event ) )
   {
      SYS_INF( "'%s': there are no consumers for event '%s'", m_name.c_str( ), p_event->signature( )->dbg_name( ).c_str( ) );
      return false;
   }

   return m_event_queue.insert( p_event );
}

carpc::async::IAsync::tSptr Thread::get_event( )
{
   return m_event_queue.get( );
}

void Thread::notify( const carpc::async::IAsync::tSptr p_event )
{
   switch( p_event->type( ) )
   {
      case async::eAsyncType::CALLABLE:
      case async::eAsyncType::RUNNABLE:
      {
         process_start( );
         SYS_VRB( "'%s': start processing runnable at %ld (%s)",
               m_name.c_str( ),
               process_started( ),
               p_event->signature( )->dbg_name( ).c_str( )
            );
         p_event->process( );
         SYS_VRB( "'%s': finished processing runnable started at %ld (%s)",
               m_name.c_str( ),
               process_started( ),
               p_event->signature( )->dbg_name( ).c_str( )
            );
         process_stop( );

         break;
      }
      case async::eAsyncType::EVENT:
      {
         auto& consumers_set = m_consumers_map.start_process( p_event->signature( ) );
         SYS_VRB( "'%s': %zu consumers will be processed", m_name.c_str( ), consumers_set.size( ) );
         for( carpc::async::IAsync::IConsumer* p_consumer : consumers_set )
         {
            process_start( );
            SYS_VRB( "'%s': start processing event at %ld (%s)",
                  m_name.c_str( ),
                  process_started( ),
                  p_event->signature( )->dbg_name( ).c_str( )
               );
            p_event->process( p_consumer );
            SYS_VRB( "'%s': finished processing event started at %ld (%s)",
                  m_name.c_str( ),
                  process_started( ),
                  p_event->signature( )->dbg_name( ).c_str( )
               );
         }
         process_stop( );
         m_consumers_map.finish_process( );

         break;
      }
      default: break;
   }
}

void Thread::set_notification( const carpc::async::IAsync::ISignature::tSptr p_signature, carpc::async::IAsync::IConsumer* p_consumer )
{
   m_consumers_map.set_notification( p_signature, p_consumer );
}

void Thread::clear_notification( const carpc::async::IAsync::ISignature::tSptr p_signature, carpc::async::IAsync::IConsumer* p_consumer )
{
   m_consumers_map.clear_notification( p_signature, p_consumer );
}

void Thread::clear_all_notifications( const carpc::async::IAsync::ISignature::tSptr p_signature, carpc::async::IAsync::IConsumer* p_consumer )
{
   m_consumers_map.clear_all_notifications( p_signature, p_consumer );
}

bool Thread::is_subscribed( const carpc::async::IAsync::tSptr p_event )
{
   switch( p_event->type( ) )
   {
      case async::eAsyncType::CALLABLE:
      case async::eAsyncType::RUNNABLE:   return true;
      case async::eAsyncType::EVENT:      return m_consumers_map.is_subscribed( p_event->signature( ) );
   }
   return false;
}

void Thread::dump( ) const
{
   SYS_DUMP_START( );
   SYS_INF( "%s:", m_name.c_str( ) );
   m_event_queue.dump( );
   m_consumers_map.dump( );
   SYS_DUMP_END( );
}

bool Thread::send( const carpc::async::IAsync::tSptr, const application::Context& )
{
   SYS_WRN( "not supported for not IPC thread" );
   return false;
}
