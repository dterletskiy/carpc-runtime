#include <map>
#include "carpc/oswrappers/linux/signals.hpp"
#include "carpc/oswrappers/Mutex.hpp"
#include "carpc/runtime/comm/timer/Timer.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "Timer"



using namespace carpc::timer;



static carpc::os::Mutex mutex_consumer_map;
static std::map< carpc::os::os_linux::timer::tID, Timer* > consumer_map;

namespace {

   void timer_processor( const carpc::os::os_linux::timer::tID timer_id )
   {
      SYS_VRB( "processing timer: %#lx", (long)timer_id );

      mutex_consumer_map.lock( );
      auto iterator = consumer_map.find( timer_id );
      if( consumer_map.end( ) == iterator )
      {
         mutex_consumer_map.unlock( );
         SYS_ERR( "Timer has not been found" );
         return;
      }
      Timer* p_timer = iterator->second;
      mutex_consumer_map.unlock( );

      p_timer->process( timer_id );
   }

   void timer_handler( union sigval sv )
   {
      carpc::os::os_linux::timer::tID* timer_id = static_cast< carpc::os::os_linux::timer::tID* >( sv.sival_ptr );
      SYS_VRB( "timer id: %#lx", (long)(*timer_id) );
      timer_processor( *timer_id );
   }

   void timer_handler_simple( union sigval sv )
   {
      if( nullptr == sv.sival_ptr )
         return;

      Timer* p_timer = static_cast< Timer* >( sv.sival_ptr );
      SYS_VRB( "Timer: %p", p_timer );
      p_timer->process( p_timer->timer_id( ) );
   }

   void signal_handler( int signal, siginfo_t* si, void* uc )
   {
      SYS_VRB( "signal: %d / si->si_signo: %d", signal, si->si_signo );
      SYS_VRB( "sival_ptr: %p(%#zx) / sival_int: %d ",
         si->si_value.sival_ptr,
         *static_cast< std::size_t* >( si->si_value.sival_ptr ),
         si->si_value.sival_int
      );

      carpc::os::os_linux::timer::tID* timer_id = static_cast< carpc::os::os_linux::timer::tID* >( si->si_value.sival_ptr );
      SYS_VRB( "timer id: %#lx", (long)(*timer_id) );
      timer_processor( *timer_id );
   }

}



Timer::Timer( ITimerConsumer* p_consumer, const std::string& name )
   : m_name( name )
   , mp_consumer( p_consumer )
{
   if( nullptr == mp_consumer )
   {
      SYS_ERR( "Timer consumer is null pointer" );
      return;
   }

   if( false == m_context.is_valid( ) )
   {
      SYS_ERR( "Creating timer not in application thread" );
      return;
   }

   // 1.
   // if( false == os::os_linux::timer::create( m_timer_id, SIGRTMIN, signal_handler ) )
   // 2.
   // if( false == os::os_linux::timer::create( m_timer_id, timer_handler, &m_timer_id ) )
   // 3.
   if( false == os::os_linux::timer::create( m_timer_id, timer_handler_simple, this ) )
   {
      SYS_ERR( "create timer error" );
      return;
   }

   mutex_consumer_map.lock( );
   const auto [iterator, success] = consumer_map.insert( { m_timer_id, this } );
   mutex_consumer_map.unlock( );
   if( false == success )
   {
      SYS_ERR( "insert error" );
      if( false == os::os_linux::timer::remove( m_timer_id ) )
      {
         SYS_ERR( "remove timer error" );
      }
      return;
   }

   SYS_VRB( "created timer: %s(%s -> %#lx)", m_name.c_str( ), m_id.dbg_name( ).c_str( ), (long) m_timer_id );
   TimerEvent::Event::set_notification( mp_consumer, { m_id.value( ) } );
}

Timer::~Timer( )
{
   TimerEvent::Event::clear_notification( mp_consumer, { m_id.value( ) } );

   mutex_consumer_map.lock( );
   const std::size_t result = consumer_map.erase( m_timer_id );
   mutex_consumer_map.unlock( );
   if( 0 == result )
   {
      SYS_WRN( "timer has not been found" );
   }
   else if( 1 < result )
   {
      SYS_WRN( "%zu timers have been founded", result );
   }

   if( false == os::os_linux::timer::remove( m_timer_id ) )
   {
      SYS_ERR( "remove timer error" );
   }
   else
   {
      SYS_VRB( "removed timer: %s(%s -> %#lx)", m_name.c_str( ), m_id.dbg_name( ).c_str( ), (long) m_timer_id );
   }
}

const bool Timer::operator<( const Timer& timer ) const
{
   return m_id < timer.m_id;
}

bool Timer::start( const std::size_t nanoseconds, const std::size_t count )
{
   if( true == m_is_running )
   {
      SYS_WRN( "Timer has been started already" );
      return false;
   }

   if( 0 == nanoseconds )
   {
      SYS_ERR( "Zero period value" );
      return false;
   }

   m_nanoseconds = nanoseconds;
   m_count = count;
   m_ticks = 0;
   os::os_linux::timer::eTimerType type = os::os_linux::timer::eTimerType::single;
   if( CONTINIOUS == m_count ) type = os::os_linux::timer::eTimerType::continious;
   if( false == os::os_linux::timer::start( m_timer_id, (long int)m_nanoseconds, type ) )
   {
      SYS_ERR( "start timer error" );
      return false;
   }

   SYS_VRB( "started timer: %s(%s -> %#lx)", m_name.c_str( ), m_id.dbg_name( ).c_str( ), (long) m_timer_id );
   m_is_running = true;
   return true;  
}

bool Timer::stop( )
{
   if( false == m_is_running )
   {
      SYS_WRN( "Timer has not been started" );
      return false;
   }

   m_is_running = false;
   m_nanoseconds = 0;
   m_count = 0;
   m_ticks = 0;

   if( false == os::os_linux::timer::stop( m_timer_id ) )
   {
      SYS_ERR( "stop timer error" );
      return false;
   }

   SYS_VRB( "stoped timer: %s(%s -> %#lx)", m_name.c_str( ), m_id.dbg_name( ).c_str( ), (long) m_timer_id );
   return true;  
}

void Timer::process( const carpc::os::os_linux::timer::tID id )
{
   if( id != m_timer_id )
   {
      SYS_ERR( "Timer id mismatch" );
      return;
   }

   ++m_ticks;
   if( m_ticks == m_count )
      stop( );
   else if( m_ticks > m_count )
      return;

   TimerEvent::Event::create( { m_id.value( ) } )->
      data( { m_id } )->priority( carpc::priority::TIMER )->send( m_context );
}



ITimerConsumer::ITimerConsumer( )
{
}

ITimerConsumer::~ITimerConsumer( )
{
}

void ITimerConsumer::process_event( const TimerEvent::Event& event )
{
   process_timer( event.data( )->id );
}
