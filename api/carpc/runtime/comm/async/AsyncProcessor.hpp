#pragma once

#include <atomic>

#include "carpc/runtime/comm/async/AsyncQueue.hpp"
#include "carpc/runtime/comm/async/AsyncPriorityQueue.hpp"
#include "carpc/runtime/comm/async/AsyncConsumerMap.hpp"



namespace carpc::async {

   /*************************
    * 
    * 'AsyncProcessor' - class for processing 'asyc' objects by dispatching them to
    * corresponding consumers what are subscribed to 'asyc' signature and stored
    * in the current class object.
    * The object of current class is the member of application::Thread what accepts
    * subscribers and dispatches 'async' objects from from its context.
    * Insertion 'async' object could be done from any other context "thread".
    * 
    * **********************/
   class AsyncProcessor
   {
      private:
         using tEventCollection = async::AsyncPriorityQueue;
         using tConsumerMap = async::AsyncConsumerMap;

      public:
         AsyncProcessor( const std::string& );
         ~AsyncProcessor( );
         AsyncProcessor( const AsyncProcessor& ) = delete;
         AsyncProcessor& operator=( const AsyncProcessor& ) = delete;

      public:
         const std::string& name( ) const;
      protected:
         std::string                   m_name{ "NoName_Thread" };

      public:
         const time_t process_started( ) const;
      protected:
         void process_start( );
         void process_stop( );
      protected:
         std::atomic< time_t >         m_process_started = 0;

      public:
         IAsync::tSptr get_event( );
         bool insert_event( const IAsync::tSptr );
      private:
         tEventCollection              m_event_queue;

      public:
         void notify( const IAsync::tSptr );
         void set_notification( const IAsync::ISignature::tSptr, IAsync::IConsumer* );
         void clear_notification( const IAsync::ISignature::tSptr, IAsync::IConsumer* );
         void clear_all_notifications( const IAsync::ISignature::tSptr, IAsync::IConsumer* );
         bool is_subscribed( const IAsync::tSptr );
      private:
         tConsumerMap                  m_consumers_map;

      public:
         void dump( ) const;
   };



   inline
   const std::string& AsyncProcessor::name( ) const
   {
      return m_name;
   }

   inline
   const time_t AsyncProcessor::process_started( ) const
   {
      return m_process_started.load( );
   }

   inline
   void AsyncProcessor::process_start( )
   {
      return m_process_started.store( time( nullptr ) );
   }

   inline
   void AsyncProcessor::process_stop( )
   {
      return m_process_started.store( 0 );
   }

} // namespace carpc::async
