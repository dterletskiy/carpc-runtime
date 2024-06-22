#include "carpc/runtime/comm/async/event/Event.hpp"
#include "carpc/runtime/application/Process.hpp"
#include "carpc/runtime/application/ThreadBase.hpp"
#include "SystemEventConsumer.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "Srv"



using namespace carpc::application;



ThreadBase::ThreadBase( const std::string& name, const std::size_t wd_timeout )
   : IThread( )
   , m_name( name )
   , m_wd_timeout( wd_timeout )
   , m_event_queue( configuration::current( ).max_priority, name )
   , m_consumers_map( name )
{
   SYS_VRB( "'%s': created", m_name.c_str( ) );
}

ThreadBase::~ThreadBase( )
{
   SYS_VRB( "'%s': destroyed", m_name.c_str( ) );
}

bool ThreadBase::insert_event( const carpc::async::IAsync::tSptr p_event )
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

carpc::async::IAsync::tSptr ThreadBase::get_event( )
{
   return m_event_queue.get( );
}

void ThreadBase::notify( const carpc::async::IAsync::tSptr p_event )
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
         m_consumers_map.process( p_event, m_process_started );

         break;
      }
      default: break;
   }
}

void ThreadBase::set_notification( const carpc::async::IAsync::ISignature::tSptr p_signature, carpc::async::IAsync::IConsumer* p_consumer )
{
   m_consumers_map.set_notification( p_signature, p_consumer );
}

void ThreadBase::clear_notification( const carpc::async::IAsync::ISignature::tSptr p_signature, carpc::async::IAsync::IConsumer* p_consumer )
{
   m_consumers_map.clear_notification( p_signature, p_consumer );
}

void ThreadBase::clear_all_notifications( const carpc::async::IAsync::ISignature::tSptr p_signature, carpc::async::IAsync::IConsumer* p_consumer )
{
   m_consumers_map.clear_all_notifications( p_signature, p_consumer );
}

bool ThreadBase::is_subscribed( const carpc::async::IAsync::tSptr p_event )
{
   switch( p_event->type( ) )
   {
      case async::eAsyncType::CALLABLE:
      case async::eAsyncType::RUNNABLE:   return true;
      case async::eAsyncType::EVENT:      return m_consumers_map.is_subscribed( p_event->signature( ) );
   }
   return false;
}

void ThreadBase::dump( ) const
{
   SYS_DUMP_START( );
   SYS_INF( "%s:", m_name.c_str( ) );
   m_event_queue.dump( );
   m_consumers_map.dump( );
   SYS_DUMP_END( );
}

bool ThreadBase::send( const carpc::async::IAsync::tSptr, const application::Context& )
{
   SYS_WRN( "not supported for not IPC thread" );
   return false;
}
