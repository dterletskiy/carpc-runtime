#include "carpc/runtime/application/Process.hpp"
#include "carpc/runtime/comm/async/event/IEvent.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "IEvent"


using namespace carpc::async;



using tRegistry = std::map< tAsyncTypeID, IEvent::tCreator >;
tRegistry s_registry;



bool IEvent::check_in( const tAsyncTypeID& event_type, tCreator p_creator )
{
   if( nullptr == p_creator )
      return false;

   s_registry.emplace( std::make_pair( event_type, p_creator ) );
   return true;
}

void IEvent::dump( )
{
   for( auto& pair : s_registry )
   {
      SYS_VRB( "name: %s / creator: %p", pair.first.c_str( ), pair.second );
   }
}

bool IEvent::serialize( ipc::tStream& stream, IEvent::tSptr p_event )
{
   return IEvent::serialize( stream, *p_event );
}

bool IEvent::serialize( ipc::tStream& stream, const IEvent& event )
{
   if( s_registry.end( ) == s_registry.find( event.signature( )->type_id( ) ) )
   {
      SYS_ERR( "event '%s' is not registered", event.signature( )->dbg_name( ).c_str( ) );
      return false;
   }

   if( false == ipc::serialize( stream, event.signature( )->type_id( ) ) )
   {
      SYS_ERR( "meta data serialization error" );
      return false;
   }

   if( false == event.to_stream_t( stream ) )
   {
      SYS_ERR( "event '%s' serialization error", event.signature( )->dbg_name( ).c_str( ) );
      return false;
   }

   return true;
}

IEvent::tSptr IEvent::deserialize( ipc::tStream& stream )
{
   tAsyncTypeID event_type_id;
   if( false == ipc::deserialize( stream, event_type_id ) )
   {
      SYS_ERR( "meta data deserialization error" );
      return nullptr;
   }

   auto iterator = s_registry.find( event_type_id );
   if( s_registry.end( ) == iterator )
   {
      SYS_ERR( "event '%s' is not registered", event_type_id.c_str( ) );
      return nullptr;
   }

   IEvent::tSptr p_event = iterator->second( );
   if( false == p_event->from_stream_t( stream ) )
   {
      SYS_ERR( "event '%s' deserialization error", event_type_id.c_str( ) );
      return nullptr;
   }

   return p_event;
}



const bool IEvent::to_stream( ipc::tStream& stream ) const
{
   return serialize( stream, *this );
}

const bool IEvent::from_stream( ipc::tStream& stream )
{
   return false;
}

const bool IEvent::set_notification( IAsync::IConsumer* p_consumer, const ISignature::tSptr p_signature )
{
   application::IThread::tSptr p_thread = application::Process::instance( )->current_thread( );
   if( nullptr == p_thread )
   {
      SYS_ERR( "subscribe on event not from application thread" );
      return false;
   }

   SYS_INF( "event: %s / consumer: %p / application thread: %s", p_signature->dbg_name( ).c_str( ), p_consumer, p_thread->name( ).c_str( ) );
   p_thread->set_notification( p_signature, p_consumer );

   return true;
}

const bool IEvent::clear_notification( IAsync::IConsumer* p_consumer, const ISignature::tSptr p_signature )
{
   application::IThread::tSptr p_thread = application::Process::instance( )->current_thread( );
   if( nullptr == p_thread )
   {
      SYS_ERR( "unsubscribe from event not from application thread" );
      return false;
   }

   SYS_INF( "event: %s / consumer: %p / application thread: %s", p_signature->dbg_name( ).c_str( ), p_consumer, p_thread->name( ).c_str( ) );
   p_thread->clear_notification( p_signature, p_consumer );

   return true;
}

const bool IEvent::clear_all_notifications( IAsync::IConsumer* p_consumer, const ISignature::tSptr p_signature )
{
   application::IThread::tSptr p_thread = application::Process::instance( )->current_thread( );
   if( nullptr == p_thread )
   {
      SYS_ERR( "unsubscribe from event not from application thread" );
      return false;
   }

   SYS_INF( "event: %s / consumer: %p / application thread: %s", p_signature->dbg_name( ).c_str( ), p_consumer, p_thread->name( ).c_str( ) );
   p_thread->clear_all_notifications( p_signature, p_consumer );

   return true;
}

const bool IEvent::send( const application::Context& to_context )
{
   return dispatch( to_context );
}

void IEvent::process( IAsync::IConsumer* p_consumer ) const
{
   if( nullptr == p_consumer )
   {
      SYS_ERR( "consumer: %p", p_consumer );
      return;
   }

   SYS_INF( "consumer: %p", p_consumer );
   process_event( p_consumer );
}
