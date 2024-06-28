#include "carpc/runtime/application/Process.hpp"
#include "carpc/runtime/comm/async/IAsync.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "IAsync"


using namespace carpc::async;



const bool IAsync::dispatch( const application::Context& to_context )
{
   const char* const async_name = name( type( ) );
   auto p_async = shared_from_this( );
   SYS_VRB( "%s: %s", async_name, p_async->signature( )->dbg_name( ).c_str( ) );

   if( to_context.is_external( ) )
   {
      if( eAsyncType::EVENT == type( ) )
      {
         SYS_INF( "sending IPC %s", async_name );
         application::IThread::tSptr p_thread_ipc = application::Process::instance( )->thread_ipc( );
         if( nullptr == p_thread_ipc )
         {
            SYS_ERR( "application IPC thread is not started" );
            return false;
         }

         return p_thread_ipc->send( p_async, to_context );
      }
      else
      {
         SYS_WRN( "%s can't be sent as external", async_name );
         return false;
      }
   }
   else if( application::thread::broadcast == to_context.tid( ) )
   {
      SYS_INF( "sending broadcast %s to all application threads", async_name );
      bool result = true;

      application::IThread::tSptr p_thread_ipc = application::Process::instance( )->thread_ipc( );
      if( nullptr != p_thread_ipc )
      {
         result &= p_thread_ipc->insert_async( p_async );
      }

      application::IThread::tSptrList thread_list = carpc::application::Process::instance( )->thread_list( );
      for( auto p_thread : thread_list )
         result &= p_thread->insert_async( p_async );

      return result;
   }
   else if( application::thread::local == to_context.tid( ) )
   {
      SYS_INF( "sending %s to current application thread: %s"
            , async_name
            , to_context.tid( ).dbg_name( ).c_str( )
         );
      application::IThread::tSptr p_thread = carpc::application::Process::instance( )->current_thread( );
      if( nullptr == p_thread )
      {
         SYS_ERR( "sending local %s not from application thread", async_name );
         return false;
      }

      return p_thread->insert_async( p_async );
   }
   else
   {
      SYS_INF( "sending %s to %s application thread"
            , async_name
            , to_context.tid( ).dbg_name( ).c_str( )
         );
      application::IThread::tSptr p_thread = carpc::application::Process::instance( )->thread( to_context.tid( ) );
      if( nullptr == p_thread )
      {
         SYS_ERR( "sending %s to unknown application thread", async_name );
         return false;
      }

      return p_thread->insert_async( p_async );
   }

   return true;
}
