#pragma once

#include <atomic>

#include "carpc/runtime/application/IThread.hpp"



namespace carpc::application {

   class ThreadBase : public IThread
   {
      protected:
         using tEventCollection = async::AsyncPriorityQueue;
         using tConsumerMap = async::AsyncConsumerMap;

      public:
         ThreadBase( const std::string&, const std::size_t );
         ~ThreadBase( );
         ThreadBase( const ThreadBase& ) = delete;
         ThreadBase& operator=( const ThreadBase& ) = delete;

      protected:
         std::atomic< bool >                          m_started = false;

      private:
         void dump( ) const override;

      private:
         bool send( const carpc::async::IAsync::tSptr, const application::Context& ) override;

      private:
         const thread::ID& id( ) const override final;
      protected:
         thread::ID                    m_id = thread::ID::generate( );

      private:
         const thread::Name& name( ) const override final;
      protected:
         thread::Name                  m_name{ "NoName_Thread" };

      private:
         const std::size_t wd_timeout( ) const override final;
      protected:
         std::size_t                   m_wd_timeout = 0;

      public:
         const time_t process_started( ) const override final;
      protected:
         void process_start( );
         void process_stop( );
      protected:
         std::atomic< time_t >         m_process_started = 0;

      protected:
         carpc::async::IAsync::tSptr get_event( );
      private:
         bool insert_event( const carpc::async::IAsync::tSptr ) override;
         tEventCollection                             m_event_queue;

      protected:
         void notify( const carpc::async::IAsync::tSptr );
      private:
         void set_notification( const carpc::async::IAsync::ISignature::tSptr, carpc::async::IAsync::IConsumer* ) override;
         void clear_notification( const carpc::async::IAsync::ISignature::tSptr, carpc::async::IAsync::IConsumer* ) override;
         void clear_all_notifications( const carpc::async::IAsync::ISignature::tSptr, carpc::async::IAsync::IConsumer* ) override;
         bool is_subscribed( const carpc::async::IAsync::tSptr );
         tConsumerMap                                 m_consumers_map;
   };



   inline
   const thread::Name& ThreadBase::name( ) const
   {
      return m_name;
   }

   inline
   const thread::ID& ThreadBase::id( ) const
   {
      return m_id;
   }

   inline
   const std::size_t ThreadBase::wd_timeout( ) const
   {
      return m_wd_timeout;
   }

   inline
   const time_t ThreadBase::process_started( ) const
   {
      return m_process_started.load( );
   }

   inline
   void ThreadBase::process_start( )
   {
      return m_process_started.store( time( nullptr ) );
   }

   inline
   void ThreadBase::process_stop( )
   {
      return m_process_started.store( 0 );
   }

} // namespace carpc::application
