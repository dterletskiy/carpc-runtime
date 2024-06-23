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
         void thread_loop( ) override;

      public:
         bool send( const async::IAsync::tSptr, const application::Context& ) override;
      private:
         SendReceive*                                 mp_send_receive;
   };

} // namespace carpc::application
