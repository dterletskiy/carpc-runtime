#include "carpc/runtime/application/Context.hpp"
#include "carpc/runtime/application/ThreadIPC.hpp"
#include "SendReceive.hpp"
#include "SystemEventConsumer.hpp"
#include "ServiceEventConsumer.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "SrvIPC"



using namespace carpc::application;



ThreadIPC::ThreadIPC( )
   : ThreadBase( "IPC", 10 )
   , m_thread( std::bind( &ThreadIPC::thread_loop, this ) )
{
   SYS_VRB( "'%s': created", m_name.c_str( ) );
   mp_send_receive = new SendReceive;
}

ThreadIPC::~ThreadIPC( )
{
   SYS_VRB( "'%s': destroyed", m_name.c_str( ) );
   delete mp_send_receive;
}

bool ThreadIPC::started( ) const
{
   return m_started.load( ) && mp_send_receive->started( );
}

bool ThreadIPC::wait( )
{
   const bool started = m_thread.join( );
   m_started.store( started );
   const bool stopped = mp_send_receive->wait( );
   return !m_started.load( ) && !stopped;
}

void ThreadIPC::thread_loop( )
{
   SYS_INF( "'%s': enter", m_name.c_str( ) );
   m_started.store( true );

   SystemEventConsumer system_event_consumer( *this );
   ServiceEventConsumer service_event_consumer( mp_send_receive );

   while( m_started.load( ) )
   {
      carpc::async::IAsync::tSptr p_event = get_event( );
      SYS_VRB( "'%s': processing event (%s)", m_name.c_str( ), p_event->signature( )->dbg_name( ).c_str( ) );
      notify( p_event );
   }

   SYS_INF( "'%s': exit", m_name.c_str( ) );
}

bool ThreadIPC::start( )
{
   SYS_INF( "'%s': starting", m_name.c_str( ) );

   if( false == mp_send_receive->start( ) )
   {
      SYS_ERR( "'%s': can't be started", m_name.c_str( ) );
      return false;
   }

   if( false == m_thread.run( m_name.c_str( ) ) )
   {
      SYS_ERR( "'%s': can't be started", m_name.c_str( ) );
      return false;
   }

   return true;
}

void ThreadIPC::stop( )
{
   SYS_INF( "'%s': stopping", m_name.c_str( ) );
   m_started.store( false );
   mp_send_receive->stop( );
}

void ThreadIPC::boot( const std::string& message )
{
   SYS_INF( "'%s': booting", m_name.c_str( ) );
}

void ThreadIPC::shutdown( const std::string& message )
{
   SYS_INF( "'%s': shutting down", m_name.c_str( ) );
   stop( );
}

bool ThreadIPC::send( const carpc::async::IAsync::tSptr p_event, const application::Context& to_context )
{
   return mp_send_receive->send( std::static_pointer_cast< async::IEvent >( p_event ), to_context );
}
