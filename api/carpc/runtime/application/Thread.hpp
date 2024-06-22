#pragma once

#include "carpc/runtime/application/IComponent.hpp"
#include "carpc/runtime/application/ThreadBase.hpp"



namespace carpc::application {

   class Thread : public ThreadBase
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

      private:
         IComponent::tSptrList                        m_components;
         IComponent::tCreatorVector                   m_component_creators;
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
