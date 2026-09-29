#pragma once

#include <atomic>



namespace carpc::runtime::comm::event {

   // MPMC
   template< typename EventT >
   class TEventPool
   {
      public:
         TEventPool( ) = default;
         ~TEventPool( );

         EventT* acquire( );

         void release( EventT* event );

      private:
         std::atomic< EventT* > m_head { nullptr };

         static EventT* pop( std::atomic< EventT* >& head );

         static void push( std::atomic< EventT* >& head, EventT* node );
   };

} // namespace carpc::runtime::comm::event
