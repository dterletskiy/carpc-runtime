#pragma once

#include "carpc/base/common/ID.hpp"
#include "carpc/oswrappers/Thread.hpp"
#include "carpc/runtime/comm/async/IAsync.hpp"
#include "carpc/runtime/comm/async/AsyncQueue.hpp"
#include "carpc/runtime/comm/async/AsyncPriorityQueue.hpp"
#include "carpc/runtime/comm/async/AsyncConsumerMap.hpp"
#include "carpc/runtime/application/Context.hpp"
#include "carpc/runtime/application/Types.hpp"



namespace carpc::application {

   class IThread
      : public std::enable_shared_from_this< IThread >
   {
      public:
         using tSptr = std::shared_ptr< IThread >;
         using tWptr = std::weak_ptr< IThread >;
         using tSptrList = std::list< tSptr >;

      public:
         IThread( ) = default;
         virtual ~IThread( ) = default;
         IThread( const IThread& ) = delete;
         IThread& operator=( const IThread& ) = delete;

      public:
         virtual bool start( ) = 0;
         virtual void stop( ) = 0;
         virtual bool started( ) const = 0;
         virtual bool wait( ) = 0;
         virtual void boot( const std::string& ) = 0;
         virtual void shutdown( const std::string& ) = 0;
         virtual const carpc::os::Thread& thread( ) const = 0;
         virtual void dump( ) const = 0;

      public:
         virtual void set_notification( const carpc::async::IAsync::ISignature::tSptr, carpc::async::IAsync::IConsumer* ) = 0;
         virtual void clear_notification( const carpc::async::IAsync::ISignature::tSptr, carpc::async::IAsync::IConsumer* ) = 0;
         virtual void clear_all_notifications( const carpc::async::IAsync::ISignature::tSptr, carpc::async::IAsync::IConsumer* ) = 0;
         virtual bool insert_event( const carpc::async::IAsync::tSptr ) = 0;
         virtual bool send( const carpc::async::IAsync::tSptr, const application::Context& ) = 0;
         virtual const std::size_t wd_timeout( ) const = 0;
         virtual const time_t process_started( ) const = 0;

      public:
         virtual const thread::ID& id( ) const = 0;
         virtual const thread::Name& name( ) const = 0;
   };

} // namespace carpc::application
