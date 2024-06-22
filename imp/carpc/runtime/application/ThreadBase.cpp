#include "carpc/runtime/application/ThreadBase.hpp"
#include "SystemEventConsumer.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "Srv"



using namespace carpc::application;



ThreadBase::ThreadBase( const std::string& name, const std::size_t wd_timeout )
   : IThread( )
   , m_name( name )
   , m_wd_timeout( wd_timeout )
   , m_async_processor( name )
{
   SYS_VRB( "'%s': created", m_name.c_str( ) );
}

ThreadBase::~ThreadBase( )
{
   SYS_VRB( "'%s': destroyed", m_name.c_str( ) );
}

bool ThreadBase::insert_event( const async::IAsync::tSptr p_event )
{
   if( false == m_started.load( ) )
   {
      SYS_WRN( "'%s': is not started", m_name.c_str( ) );
      return false;
   }

   return m_async_processor.insert_event( p_event );
}

carpc::async::IAsync::tSptr ThreadBase::get_event( )
{
   return m_async_processor.get_event( );
}

void ThreadBase::notify( const async::IAsync::tSptr p_event )
{
   m_async_processor.notify( p_event );
}

void ThreadBase::set_notification( const async::IAsync::ISignature::tSptr p_signature, async::IAsync::IConsumer* p_consumer )
{
   m_async_processor.set_notification( p_signature, p_consumer );
}

void ThreadBase::clear_notification( const async::IAsync::ISignature::tSptr p_signature, async::IAsync::IConsumer* p_consumer )
{
   m_async_processor.clear_notification( p_signature, p_consumer );
}

void ThreadBase::clear_all_notifications( const async::IAsync::ISignature::tSptr p_signature, async::IAsync::IConsumer* p_consumer )
{
   m_async_processor.clear_all_notifications( p_signature, p_consumer );
}

bool ThreadBase::is_subscribed( const async::IAsync::tSptr p_event )
{
   return m_async_processor.is_subscribed( p_event );
}

void ThreadBase::dump( ) const
{
   SYS_DUMP_START( );
   SYS_INF( "%s:", m_name.c_str( ) );
   m_async_processor.dump( );
   SYS_DUMP_END( );
}

bool ThreadBase::send( const async::IAsync::tSptr, const application::Context& )
{
   SYS_WRN( "not supported for not IPC thread" );
   return false;
}
