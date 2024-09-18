#include "carpc/runtime/comm/async/AsyncProcessor.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "AsyncProc"



using namespace carpc::async;



AsyncProcessor::AsyncProcessor( const std::string& name )
   : m_name( name )
   , m_async_queue( name )
   , m_consumers_map( name )
{
   SYS_VRB( "'%s': created", m_name.c_str( ) );
}

AsyncProcessor::~AsyncProcessor( )
{
   SYS_VRB( "'%s': destroyed", m_name.c_str( ) );
}

bool AsyncProcessor::insert_async( const IAsync::tSptr p_async )
{
   if( false == is_subscribed( p_async ) )
   {
      SYS_INF( "'%s': there are no consumers for async object '%s'",
            m_name.c_str( ),
            p_async->signature( )->dbg_name( ).c_str( )
         );
      return false;
   }

   return m_async_queue.insert( p_async );
}

IAsync::tSptr AsyncProcessor::get_async( )
{
   return m_async_queue.get( );
}

void AsyncProcessor::notify_consumers( const IAsync::tSptr p_async )
{
   switch( p_async->type( ) )
   {
      case async::eAsyncType::CALLABLE:
      case async::eAsyncType::RUNNABLE:
      {
         process_start( );
         SYS_VRB( "'%s': start processing runnable at %ld (%s)",
               m_name.c_str( ),
               process_started( ),
               p_async->signature( )->dbg_name( ).c_str( )
            );
         p_async->process( );
         SYS_VRB( "'%s': finished processing runnable started at %ld (%s)",
               m_name.c_str( ),
               process_started( ),
               p_async->signature( )->dbg_name( ).c_str( )
            );
         process_stop( );

         break;
      }
      case async::eAsyncType::EVENT:
      {
         m_consumers_map.process( p_async, m_process_started );

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

bool AsyncProcessor::is_subscribed( const IAsync::tSptr p_async )
{
   switch( p_async->type( ) )
   {
      case async::eAsyncType::CALLABLE:
      case async::eAsyncType::RUNNABLE:
      {
         return true;
      }
      case async::eAsyncType::EVENT:
      {
         return m_consumers_map.is_subscribed( p_async->signature( ) );
      }
   }
   return false;
}

void AsyncProcessor::dump( ) const
{
   SYS_DUMP_START( );
   SYS_INF( "%s:", m_name.c_str( ) );
   m_async_queue.dump( );
   m_consumers_map.dump( );
   SYS_DUMP_END( );
}
