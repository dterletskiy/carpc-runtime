#pragma once

#include <atomic>

#include "carpc/runtime/application/IComponent.hpp"
#include "carpc/runtime/application/IThread.hpp"



namespace carpc::application {

   class Thread : public IThread
   {
      public:
         struct Configuration
         {
            using tVector = std::vector< Configuration >;
            std::string                m_name;
            IComponent::tCreatorVector m_component_creators;
            std::size_t                m_wd_timeout;
         };

      public:
         Thread( const Configuration& );
         ~Thread( );
         Thread( const Thread& ) = delete;
         Thread& operator=( const Thread& ) = delete;

      private:
         void boot( const std::string& ) override;
         void shutdown( const std::string& ) override;

      private:
         bool start( ) override;
         void stop( ) override;
         bool started( ) const override;
         bool wait( ) override;

      private:
         const carpc::os::Thread& thread( ) const override;
         void thread_loop( );
         carpc::os::Thread                            m_thread;
         std::atomic< bool >                          m_started = false;

      private:
         bool insert_event( const carpc::async::IAsync::tSptr ) override;
         carpc::async::IAsync::tSptr get_event( );
         tEventCollection                             m_event_queue;

      private:
         void set_notification( const carpc::async::IAsync::ISignature::tSptr, carpc::async::IAsync::IConsumer* ) override;
         void clear_notification( const carpc::async::IAsync::ISignature::tSptr, carpc::async::IAsync::IConsumer* ) override;
         void clear_all_notifications( const carpc::async::IAsync::ISignature::tSptr, carpc::async::IAsync::IConsumer* ) override;
         bool is_subscribed( const carpc::async::IAsync::tSptr );
         void notify( const carpc::async::IAsync::tSptr );
         tConsumerMap                                 m_consumers_map;

      private:
         IComponent::tSptrList                        m_components;
         IComponent::tCreatorVector                   m_component_creators;

      private:
         void dump( ) const override;

      private:
         bool send( const carpc::async::IAsync::tSptr, const application::Context& ) override;
   };



   inline
   const carpc::os::Thread& Thread::thread( ) const
   {
      return m_thread;
   }

   inline
   bool Thread::started( ) const
   {
      return m_started.load( );
   }

   inline
   bool Thread::wait( )
   {
      const bool started = m_thread.join( );
      m_started.store( started );
      return !m_started.load( );
   }

} // namespace carpc::application
