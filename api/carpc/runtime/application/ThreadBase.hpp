#pragma once

#include <atomic>

#include "carpc/runtime/comm/async/AsyncProcessor.hpp"
#include "carpc/runtime/application/IThread.hpp"



namespace carpc::application {

   class ThreadBase : public IThread
   {
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
         bool send( const async::IAsync::tSptr, const application::Context& ) override;

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

      protected:
         void notify_consumers( const async::IAsync::tSptr );
         async::IAsync::tSptr get_async( );
      private:
         void set_notification( const async::IAsync::ISignature::tSptr, async::IAsync::IConsumer* ) override final;
         void clear_notification( const async::IAsync::ISignature::tSptr, async::IAsync::IConsumer* ) override final;
         void clear_all_notifications( const async::IAsync::ISignature::tSptr, async::IAsync::IConsumer* ) override final;
         bool is_subscribed( const async::IAsync::tSptr );
         bool insert_async( const async::IAsync::tSptr ) override final;
         const time_t process_started( ) const override final;
         async::AsyncProcessor         m_async_processor;
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
      return m_async_processor.process_started( );
   }

} // namespace carpc::application
