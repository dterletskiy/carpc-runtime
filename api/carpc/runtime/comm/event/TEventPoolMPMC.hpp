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



   template< typename EventT >
   TEventPool< EventT >::~TEventPool( )
   {
      EventT* node = m_head.load( );
      while( node )
      {
         EventT* next = node->m_next;
         delete node;
         node = next;
      }
   }

   template< typename EventT >
   EventT* TEventPool< EventT >::acquire( )
   {
      EventT* ev = pop( m_head );
      if ( !ev )
         return nullptr;

      // Reset reference count for reuse
      ev->reset_refcount( );
      return ev;
   }

   template< typename EventT >
   void TEventPool< EventT >::release( EventT* event )
   {
      push( m_head, event );
   }

   // Multiple-consumer pop (CAS needed)
   template< typename EventT >
   EventT* TEventPool< EventT >::pop( std::atomic< EventT* >& head )
   {
      EventT* old_head = head.load( std::memory_order_acquire );
      while( old_head )
      {
         EventT* next = old_head->m_next;
         if( head.compare_exchange_weak(
               old_head, next,
               std::memory_order_acq_rel,
               std::memory_order_relaxed
            ) )
         {
            old_head->m_next = nullptr;
            return old_head;
         }
      }
      return nullptr;
   }

   // Multiple-producer push
   // Lock-free push (multiple producers)
   template< typename EventT >
   void TEventPool<EventT>::push( std::atomic< EventT* >& head, EventT* node )
   {
      EventT* old_head = head.load( std::memory_order_relaxed );
      do
      {
         node->m_next = old_head;
      } while( !head.compare_exchange_weak(
            old_head, node,
            std::memory_order_release,
            std::memory_order_relaxed
         ) );
   }

} // namespace carpc::runtime::comm::event
