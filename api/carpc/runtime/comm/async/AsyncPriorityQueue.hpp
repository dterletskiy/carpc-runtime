#pragma once

#include <atomic>

#include "carpc/oswrappers/ConditionVariable.hpp"
#include "carpc/runtime/comm/async/IAsync.hpp"



namespace carpc::async {

   class AsyncPriorityQueue
   {
      public:
         using tSptr = std::shared_ptr< AsyncPriorityQueue >;
         using tWptr = std::weak_ptr< AsyncPriorityQueue >;
         using tCollection = std::vector< std::deque< IAsync::tSptr > >;

      public:
         /***************
          *
          * max_priority - max number of priorities.
          *    Container with prioritised object will containe indexes [0; max_priority),
          *    but real iteration will be performed from (0; max_priority).
          *    This is because of method carpc::Tpriority::check( )
          *
          **************/
         AsyncPriorityQueue( const std::string& name = "NoName", const tPriority& max_priority = tPriority::max );
         ~AsyncPriorityQueue( );
         AsyncPriorityQueue( const AsyncPriorityQueue& ) = delete;
         AsyncPriorityQueue& operator=( const AsyncPriorityQueue& ) = delete;

      private:
         std::string                m_name;

      public:
         bool insert( const IAsync::tSptr );
         IAsync::tSptr get( );
         void clear( );
      private:
         tCollection                m_collections;
         os::ConditionVariable      m_buffer_cond_var;

      public:
         void freeze( );
         void unfreeze( );
         bool is_freezed( ) const;
      private:
         std::atomic< bool >        m_freezed = false;

      public:
         void dump( ) const;
   };



   inline
   void AsyncPriorityQueue::freeze( )
   {
      m_freezed.store( true );
   }

   inline
   void AsyncPriorityQueue::unfreeze( )
   {
      m_freezed.store( false );
   }

   inline
   bool AsyncPriorityQueue::is_freezed( ) const
   {
      return m_freezed.load( );
   }

} // namespace carpc::async
