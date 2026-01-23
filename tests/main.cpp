#include "carpc/trace/trace.h"



#include "carpc/runtime/core/Context.hpp"
#include "carpc/runtime/comm/event/Event.hpp"

namespace test::event {

   struct EventData
   {
      EventData( )
      {
         CARPC_TRACE_LOG_TRACE( );
      }
      EventData( const EventData& )
      {
         CARPC_TRACE_LOG_TRACE( );
      }
      EventData( EventData&& ) noexcept
      {
         CARPC_TRACE_LOG_TRACE( );
      }
      ~EventData( )
      {
         CARPC_TRACE_LOG_TRACE( );
      }
   };

   struct EventSignature
   {
      EventSignature( )
      {
         CARPC_TRACE_LOG_TRACE( );
      }
      EventSignature( const EventSignature& )
      {
         CARPC_TRACE_LOG_TRACE( );
      }
      EventSignature( EventSignature&& ) noexcept
      {
         CARPC_TRACE_LOG_TRACE( );
      }
      ~EventSignature( )
      {
         CARPC_TRACE_LOG_TRACE( );
      }
   };

   DEFINE_EVENT( System, EventData, EventSignature );

   void run_all( )
   {

      {
         System::Event::tEventPtr event =
            System::Event::create( EventSignature{ } )
               ->data.set( EventData{ } )
                  ->data.set( );
         System::Event::tDataPtr data = event->data.get( );

         CARPC_TRACE_LOG_TRACE( "%s",
               event->type_id( ).value( ).c_str( )
            );
      }

      {
         carpc::runtime::core::Context ctx =
               carpc::runtime::core::Context::current( );
      }

      CARPC_TRACE_LOG_TRACE( "[Event] All tests passed." );
   }

}



int main( )
{
   carpc::trace::StdoutSink sink;
   carpc::trace::Runtime::start( &sink );

   test::event::run_all( );

   carpc::trace::Runtime::stop( );

   return 0;
}
