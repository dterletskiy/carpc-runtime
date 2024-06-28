#include "carpc/runtime/application/Process.hpp"
#include "carpc/runtime/comm/async/runnable/IRunnable.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "IRunnable"


using namespace carpc::async;



const bool IRunnable::send( const application::Context& to_context, const bool is_block )
{
   // Sending non blocking Async object
   if( false == is_block )
      return dispatch( to_context );

   if( to_context == application::Context::current( ) )
   {
      SYS_ERR(
            "sending blocking runnable object to destination context '%s' = calling context '%s'",
            to_context.dbg_name( ).c_str( ),
            application::Context::current( ).dbg_name( ).c_str( )
         );
      return false;
   }

   // Sending blocking Async object
   std::shared_ptr<carpc::os::ConditionVariable> p_cond_var = std::make_shared<carpc::os::ConditionVariable>();


   auto operation_wrapper = [ operation = m_operation, p_cond_var ]( )
   {
      if( operation )
         operation( );

      p_cond_var->notify( );
   };

   m_operation = operation_wrapper;

   if( false == dispatch( to_context ) )
      return false;

   while ( true != p_cond_var->test( ) )
      p_cond_var->wait( );

   return true;
}

void IRunnable::process( IAsync::IConsumer* p_consumer ) const
{
   if( nullptr == m_operation ) return;
   m_operation( );
}
