#include <sys/types.h>
#include <unistd.h>
#include <filesystem>
#include <chrono>
#include <thread>
#include "carpc/oswrappers/linux/signals.hpp"
#include "carpc/oswrappers/Mutex.hpp"
#include "carpc/runtime/application/Thread.hpp"
#include "carpc/runtime/application/ThreadIPC.hpp"
#include "carpc/runtime/application/Process.hpp"
#include "carpc/runtime/events/Events.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "SrvcProc"



namespace {

   void process_watchdog( )
   {
      for( const auto& p_thread : carpc::application::Process::instance( )->thread_list( ) )
      {
         time_t time_stamp = p_thread->process_started( );
         if( 0 == time_stamp )
            continue;

         if( 0 >= p_thread->wd_timeout( ) )
            continue;
         if( p_thread->wd_timeout( ) > static_cast< std::size_t >( time( nullptr ) - time_stamp ) )
            continue;

         SYS_ERR( "WatchDog error: '%s'", p_thread->name( ).c_str( ) );
      }
   }

   void timer_handler( union sigval sv )
   {
      carpc::os::os_linux::timer::tID* timer_id = static_cast< carpc::os::os_linux::timer::tID* >( sv.sival_ptr );
      SYS_INF( "WatchDog timer: %#lx", (long) *timer_id );
      process_watchdog( );
   }

   void signal_handler( int signal, siginfo_t* si, void* uc )
   {
      carpc::os::os_linux::timer::tID* timer_id = static_cast< carpc::os::os_linux::timer::tID* >( si->si_value.sival_ptr );
      SYS_INF( "WatchDog timer: %#lx", (long) *timer_id );
      process_watchdog( );
   }

}



using namespace carpc::application;



Process::tSptr Process::mp_instance = nullptr;

Process::Process( int argc, char** argv, char** envp )
   : m_id( getpid( ) ) // @TDA: should be unique for all computers in the network
   , m_params( argc, argv, envp )
{
   SYS_DBG( "[runtime] creating..." );

   m_params.print( );

   m_configuration.ipc = false;

   if(
            m_params.exists( "ipc" )
         && m_params.exists( "ipc_servicebrocker_domain" )
         && m_params.exists( "ipc_servicebrocker_type" )
         && m_params.exists( "ipc_servicebrocker_protocole" )
         && m_params.exists( "ipc_servicebrocker_address" )
         && m_params.exists( "ipc_servicebrocker_port" )

         && m_params.exists( "ipc_application_domain" )
         && m_params.exists( "ipc_application_type" )
         && m_params.exists( "ipc_application_protocole" )
         && m_params.exists( "ipc_application_address" )
         && m_params.exists( "ipc_application_port" )
      )
   {
      m_configuration.ipc = true;

      m_configuration.ipc_sb.socket = os::os_linux::socket::configuration {
         carpc::os::os_linux::socket::socket_domain_from_string(
               m_params.value( "ipc_servicebrocker_domain" )
            ),
         carpc::os::os_linux::socket::socket_type_from_string(
               m_params.value( "ipc_servicebrocker_type" )
            ),
         static_cast< int >( std::stoll( m_params.value( "ipc_servicebrocker_protocole" ) ) ),
         m_params.value( "ipc_servicebrocker_address" ),
         static_cast< int >( std::stoll( m_params.value( "ipc_servicebrocker_port" ) ) )
      };
      m_configuration.ipc_sb.buffer_size = static_cast< std::size_t >( std::stoll(
            m_params.value_or( "ipc_servicebrocker_buffer_size", "4096" ) )
         );

      m_configuration.ipc_app.socket = os::os_linux::socket::configuration {
         carpc::os::os_linux::socket::socket_domain_from_string(
               m_params.value( "ipc_application_domain" )
            ),
         carpc::os::os_linux::socket::socket_type_from_string(
               m_params.value( "ipc_application_type" )
            ),
         static_cast< int >( std::stoll( m_params.value( "ipc_application_protocole" ) ) ),
         m_params.value( "ipc_application_address" ),
         static_cast< int >( std::stoll( m_params.value( "ipc_application_port" ) ) )
      };
      m_configuration.ipc_app.buffer_size = static_cast< std::size_t >(
            std::stoll( m_params.value_or( "ipc_application_buffer_size", "4096" ) )
         );
   }

   m_configuration.wd_timout = static_cast< std::size_t >(
         std::stoll( m_params.value_or( "application_wd_timout", "10" ) )
      );

   DUMP_IPC_EVENTS;

   SYS_DBG( "[runtime] created" );
}

Process::~Process( )
{
   SYS_DBG( "[runtime] destroyed" );
}

namespace { carpc::os::Mutex s_mutex; }
Process::tSptr Process::instance( int argc, char** argv, char** envp )
{
   // First call of this function must be done from main function => should not be race condition.
   // os::Mutex::AutoLocker locker( s_mutex );
   if( nullptr == mp_instance )
   {
      if( argc < 1 || nullptr == argv )
      {
         SYS_ERR( "called not from 'main( int argc, char** argv )' function" );
         return nullptr;
      }
      mp_instance.reset( new Process( argc, argv, envp ) );
   }

   return mp_instance;
}

IThread::tSptr Process::thread_ipc( ) const
{
   return mp_thread_ipc;
}

IThread::tSptr Process::thread( const thread::ID& id ) const
{
   for( auto& p_thread : m_thread_list )
   {
      if( id == p_thread->id( ) )
         return p_thread;
   }

   return nullptr;
}

IThread::tSptr Process::thread( const std::string& name ) const
{
   for( auto& p_thread : m_thread_list )
   {
      if( p_thread->name( ) == name )
         return p_thread;
   }

   return nullptr;
}

IThread::tSptr Process::current_thread( ) const
{
   for( auto& p_thread : m_thread_list )
   {
      if( p_thread && os::Thread::current_id( ) == p_thread->thread( ).id( ) )
         return p_thread;
   }

   if( mp_thread_ipc && os::Thread::current_id( ) == mp_thread_ipc->thread( ).id( ) )
      return mp_thread_ipc;

   return nullptr;
}

bool Process::start( const Thread::Configuration::tVector& thread_configs )
{
   SYS_DBG( "[runtime] starting..." );

   if( true == m_configuration.ipc )
   {
      // Creating IPC brocker thread
      mp_thread_ipc = std::make_shared< ThreadIPC >( );
      if( nullptr == mp_thread_ipc )
         return false;

      // Starting IPC brocker thread
      if( true == mp_thread_ipc->start( ) )
      {
         SYS_DBG( "[runtime] starting IPC thread" );
         while( false == mp_thread_ipc->started( ) )
         {
            std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
            SYS_WRN( "[runtime] waiting to strat IPC thread" );
         }
         SYS_DBG( "[runtime] IPC thread started" );
      }
      else
      {
         SYS_WRN( "[runtime] IPC thread can't be started" );
      }
   }
   else
   {
      SYS_DBG( "[runtime] starting without IPC thread" );
   }

   // Creating application threads
   for( const auto& thread_config : thread_configs )
   {
      IThread::tSptr p_thread = std::make_shared< Thread >( thread_config );
      if( nullptr == p_thread )
         return false;
      m_thread_list.emplace_back( p_thread );
   }

   // Starting application threads
   for( const auto& p_thread : m_thread_list )
   {
      SYS_DBG( "[runtime] starting '%s' thread", p_thread->name( ).c_str( ) );
      if( false == p_thread->start( ) )
         return false;
   }

   while( true )
   {
      bool threads_started = true;
      for( const auto& p_thread : m_thread_list )
      {
         bool thread_started = p_thread->started( );
         threads_started &= thread_started;
         if( false == thread_started )
         {
            SYS_WRN( "[runtime] waiting to strat '%s' thread", p_thread->name( ).c_str( ) );
         }
      }

      if( true == threads_started )
         break;

      std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
   }
   SYS_DBG( "[runtime] all application threads started" );

   // Watchdog timer
   if( 0 < m_configuration.wd_timout )
   {
      if( false == os::os_linux::timer::create( m_timer_id, timer_handler, &m_timer_id ) )
         return false;
      if( false == os::os_linux::timer::start( m_timer_id, m_configuration.wd_timout * 1000000000, os::os_linux::timer::eTimerType::continious ) )
         return false;
   }
   else
   {
      SYS_WRN( "[runtime] watchdog disabled" );
   }

   SYS_DBG( "[runtime] started" );
   return true;
}

bool Process::stop( )
{
   SYS_DBG( "[runtime] stopping" );

   for( auto& p_thread : m_thread_list )
      if( p_thread )
         p_thread->stop( );
   m_thread_list.clear( );
   if( mp_thread_ipc )
      mp_thread_ipc->stop( );

   os::os_linux::timer::remove( m_timer_id );

   SYS_DBG( "[runtime] stopped" );
   return true;
}

void Process::boot( )
{
   SYS_DBG( "[runtime] running..." );

   events::system::System::Event::create( { events::system::eID::boot } )->
      data( { "booting application" } )->send( );

   for( auto& p_thread : m_thread_list )
      if( p_thread )
         p_thread->wait( );
   SYS_DBG( "[runtime] all application threads are stopped" );

   if( mp_thread_ipc )
      mp_thread_ipc->wait( );
   SYS_DBG( "[runtime] IPC thread is stopped" );

   os::os_linux::timer::remove( m_timer_id );

   m_thread_list.clear( );
   mp_thread_ipc.reset( );

   SYS_DBG( "[runtime] stopped" );
}
