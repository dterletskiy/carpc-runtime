#pragma once

#include "carpc/oswrappers/ConditionVariable.hpp"
#include "carpc/runtime/comm/async/IAsync.hpp"



namespace carpc::async {

   class AsyncQueue
   {
      public:
         using tSptr = std::shared_ptr< AsyncQueue >;
         using tWptr = std::weak_ptr< AsyncQueue >;
         using tCollection = std::deque< IAsync::tSptr >;

      public:
         AsyncQueue( const std::string& name = "NoName" );
         ~AsyncQueue( );
         AsyncQueue( const AsyncQueue& ) = delete;
         AsyncQueue& operator=( const AsyncQueue& ) = delete;

      private:
         std::string                m_name;

      public:
         bool insert( const IAsync::tSptr );
         IAsync::tSptr get( );
         void clear( );
      private:
         tCollection                m_collection;
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
   void AsyncQueue::freeze( )
   {
      m_freezed.store( true );
   }

   inline
   void AsyncQueue::unfreeze( )
   {
      m_freezed.store( false );
   }

   inline
   bool AsyncQueue::is_freezed( ) const
   {
      return m_freezed.load( );
   }

} // namespace carpc::async
