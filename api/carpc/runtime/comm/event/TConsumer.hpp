#pragma once

#include "carpc/runtime/comm/event/IConsumer.hpp"



namespace carpc::runtime::comm::event {

   template< typename _Generator >
   struct TConsumer : public IConsumer
   {
      using tEvent = typename _Generator::Config::tEvent;

      TConsumer( ) = default;
      ~TConsumer( ) override
      {
         // tEvent::clear_all_notifications( this );
      }

      virtual void process_event( const tEvent& ) = 0;
   };

} // namespace carpc::runtime::comm::event
