#pragma once

#include "carpc/runtime/application/ThreadBase.hpp"



namespace carpc::application {

   class Context;
   class SendReceive;



   class ThreadIPC : public ThreadBase
   {
      public:
         ThreadIPC( );
         ~ThreadIPC( );
         ThreadIPC( const ThreadIPC& ) = delete;
         ThreadIPC& operator=( const ThreadIPC& ) = delete;

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

      public:
         bool send( const carpc::async::IAsync::tSptr, const application::Context& ) override;
      private:
         SendReceive*                                 mp_send_receive;
   };



   inline
   const carpc::os::Thread& ThreadIPC::thread( ) const
   {
      return m_thread;
   }

} // namespace carpc::application
