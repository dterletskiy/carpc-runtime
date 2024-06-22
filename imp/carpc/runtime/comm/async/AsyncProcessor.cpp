#include "carpc/runtime/comm/async/AsyncProcessor.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "AsyncProc"



using namespace carpc::async;



AsyncProcessor::AsyncProcessor( const std::string& name )
   : m_name( name )
   , m_event_queue( tPriority::max, name )
   , m_consumers_map( name )
{
   SYS_VRB( "'%s': created", m_name.c_str( ) );
}

AsyncProcessor::~AsyncProcessor( )
{
   SYS_VRB( "'%s': destroyed", m_name.c_str( ) );
}

bool AsyncProcessor::insert_event( const IAsync::tSptr p_event )
{
   if( false == is_subscribed( p_event ) )
   {
      SYS_INF( "'%s': there are no consumers for event '%s'", m_name.c_str( ), p_event->signature( )->dbg_name( ).c_str( ) );
      return false;
   }

   return m_event_queue.insert( p_event );
}

IAsync::tSptr AsyncProcessor::get_event( )
{
   return m_event_queue.get( );
}

void AsyncProcessor::notify( const IAsync::tSptr p_event )
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

void AsyncProcessor::set_notification( const IAsync::ISignature::tSptr p_signature, IAsync::IConsumer* p_consumer )
{
   m_consumers_map.set_notification( p_signature, p_consumer );
}

void AsyncProcessor::clear_notification( const IAsync::ISignature::tSptr p_signature, IAsync::IConsumer* p_consumer )
{
   m_consumers_map.clear_notification( p_signature, p_consumer );
}

void AsyncProcessor::clear_all_notifications( const IAsync::ISignature::tSptr p_signature, IAsync::IConsumer* p_consumer )
{
   m_consumers_map.clear_all_notifications( p_signature, p_consumer );
}

bool AsyncProcessor::is_subscribed( const IAsync::tSptr p_event )
{
   switch( p_event->type( ) )
   {
      case async::eAsyncType::CALLABLE:
      case async::eAsyncType::RUNNABLE:   return true;
      case async::eAsyncType::EVENT:      return m_consumers_map.is_subscribed( p_event->signature( ) );
   }
   return false;
}

void AsyncProcessor::dump( ) const
{
   SYS_DUMP_START( );
   SYS_INF( "%s:", m_name.c_str( ) );
   m_event_queue.dump( );
   m_consumers_map.dump( );
   SYS_DUMP_END( );
}
