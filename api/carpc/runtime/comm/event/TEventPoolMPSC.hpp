namespace carpc::runtime::comm::event {

   template< typename EventT >
   TEventPool< EventT >::~TEventPool( )
   {
      EventT* node = m_head.load( std::memory_order_relaxed );
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

   // Single-consumer pop (no CAS needed)
   template< typename EventT >
   EventT* TEventPool< EventT >::pop( std::atomic< EventT* >& head )
   {
      // Exchange head with nullptr to grab all nodes atomically
      EventT* node = head.exchange( nullptr, std::memory_order_acquire );
      if ( !node )
         return nullptr;

      // Take the first node from the grabbed list
      EventT* ev = node;
      EventT* next = ev->m_next;

      if ( next )
         head.store( next, std::memory_order_relaxed ); // put back remaining nodes

      ev->m_next = nullptr;
      return ev;
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
